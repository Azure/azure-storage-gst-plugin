#include "blockazureuploader.hpp"
#include "blockazureuploader.h"

#include <chrono>
#include <algorithm>
#include <gst/gst.h>

#include "utils/base64.hpp"
#include "utils/common.hpp"


using namespace std::chrono_literals;
namespace gst {
namespace azure {
namespace storage {

BlockAzureUploader::BlockAzureUploader(
  const char *account_name, const char *account_key, bool use_https,
  unsigned long block_size, unsigned long worker_count,
  unsigned long commit_block_count, unsigned long commit_interval_ms)
  : block_size(block_size), worker_count(worker_count),
    commit_block_count(commit_block_count), commit_interval_ms(commit_interval_ms)
{
  std::string name(account_name);
  std::string key(account_key);
  auto credential = std::make_shared<::azure::storage_lite::shared_key_credential>(name, key);
  auto storage_account = std::make_shared<::azure::storage_lite::storage_account>(name, credential, use_https);
  client = std::make_shared<::azure::storage_lite::blob_client>(storage_account, worker_count);
}

// Check if the location is correct.
// BlockAzureUploader only support one location at a time,
// and cannot be modified once specified
bool BlockAzureUploader::checkLoc(std::shared_ptr<AzureUploadLocation> loc)
{
  return this->loc != nullptr && this->loc == loc;
}

// Initialize the uploader by specifying upload location, which cannot be
// modified later on.
std::shared_ptr<AzureUploadLocation> BlockAzureUploader::init(const char *container_name, const char *blob_name)
{
  if(loc != nullptr && destroy(loc) == false)
    log() << "Warning: failed to destroy previous workers." << std::endl;
  loc = std::make_shared<AzureUploadLocation>(std::string(container_name), std::string(blob_name));
  // create the blob first
  auto ss = std::stringstream();
  log() << "Creating block blob..." << std::endl;
  auto fut = client->upload_block_blob_from_stream(loc->first, loc->second, ss, std::vector<std::pair<std::string, std::string>>());
  auto result = fut.get();
  handle(result);
  // generate stream object
  stream = std::make_unique<std::stringstream>();
  // configure block id
  completedId = nextId = committedId = 0;
  // spawn all workers
  workers = std::vector<std::future<void>>();
  workers.reserve(worker_count);
  for(unsigned i = 0; i < worker_count; i++)
    workers.emplace_back(std::async(&BlockAzureUploader::run, this));
  // spawn the comitter
  commitWorker = std::async(&BlockAzureUploader::runCommit, this);
  return loc;
}

// Receive bunch of data. Send it once it exceeds configured block size.
bool BlockAzureUploader::upload(std::shared_ptr<AzureUploadLocation> loc, const char *data, size_t size)
{
  if(!checkLoc(loc))
    return false;
  stream->write(data, size);
  // push the stream to upload if it exceeds the block size
  // we do not make perfect cut here, since we don't want to split single frame
  // into different blocks
  if(getStreamLen(*stream) >= block_size)
  {
    reqs.push(UploadJob{nextId++, std::move(stream)});
    stream = std::make_unique<std::stringstream>();
  }
  return true;
}

// Flush remaining data, blocks until all requests are being processed.
bool BlockAzureUploader::flush(std::shared_ptr<AzureUploadLocation> loc)
{
  if(!checkLoc(loc))
    return false;
  log() << "Flushing..." << std::endl;
  // commit remaining data
  if(getStreamLen(*stream) > 0) {
    log() << "Pushing uncommitted data..." << std::endl;
    reqs.push(UploadJob{ nextId++, std::move(stream) });
    stream = std::make_unique<std::stringstream>();
  }
  // wait for the request queue to become empty
  reqs.wait_empty();
  return true;
}

// Flush & commit all remaining blocks.
bool BlockAzureUploader::destroy(std::shared_ptr<AzureUploadLocation> loc)
{
  if(!checkLoc(loc))
    return false;
  reqs.close();
  reqs.wait_empty();
  // wait for all current workers to finish their jobs
  for(auto &fut: workers)
    fut.wait();
  // wait for the committer to commit all blocks
  resps.close();
  if(commitWorker.valid())
    commitWorker.wait();
  return true;
}

// uploader thread's routine
void BlockAzureUploader::run()
{
  log() << "Initialized new upload worker." << std::endl;
  while(1)
  {
    // wait for new content
    UploadJob job;
    try {
      job = std::move(reqs.pop());
    } catch (ClosedException &e) {
      log() << e.what() << std::endl;
      break;
    }
    auto len = getStreamLen(*job.stream);
    // get new block id in base64 format
    std::string b64_block_id = base64_encode(job.id);
    log() << "Uploading content, length = " << len << " id = " << b64_block_id << std::endl;
    // keep trying until success
    do {
      auto fut = client->upload_block_from_stream(loc->first, loc->second, b64_block_id, *job.stream);
      auto result = fut.get();
      handle(result, log());
      if(result.success())
        break;
      log() << "Retrying " << b64_block_id << "..." << std::endl;
    } while(1);
    // success, send back response
    resps.push(UploadResponse{UploadResponse::OK, job.id});
  }
  log() << "Worker is exiting." << std::endl;
}

// add one block to block list(the next block)
void BlockAzureUploader::commitBlock(blockid_t id)
{
  window.push_back(id);
  std::push_heap(window.begin(), window.end(), [](blockid_t a, blockid_t b) { return a > b; });      
  while(completedId == window.front())
  {
    block_list.push_back(::azure::storage_lite::put_block_list_request_base::block_item{
      base64_encode(completedId), put_block_list_request_base::block_type::latest
    });
    std::pop_heap(window.begin(), window.end(), [](blockid_t a, blockid_t b) { return a > b; });
    window.pop_back();
    completedId += 1;
  }
}

// upload put block list
void BlockAzureUploader::doCommit()
{
  // no metadata needed
  auto metadata = std::vector<std::pair<std::string, std::string>>();
  auto fut = client->put_block_list(loc->first, loc->second, block_list, metadata);
  auto result = fut.get();
  handle(result);
  if(result.success()) {
    lastCommit = std::chrono::steady_clock::now();
    committedId = completedId;
    log() << "Committed, last block id = " << committedId << std::endl;
  }
}

// commit thread's routine
void BlockAzureUploader::runCommit()
{
  log() << "Initializing committer." << std::endl;
  lastCommit = std::chrono::steady_clock::now();
  auto interval = static_cast<long>(commit_interval_ms) * 1ms;
  while(1)
  {
    // fetch all responses already enqueued
    try {
      UploadResponse resp;
      do {
        auto timeout = lastCommit + interval - std::chrono::steady_clock::now();
        resp = resps.pop_for(timeout);
        if(resp.code == UploadResponse::OK)
          commitBlock(resp.id);
      } while(!resps.empty());
      if(completedId - committedId >= commit_block_count)
        doCommit();
    } catch (ClosedException &e) {
      log() << "Response queue is closed." << std::endl;
      break;
    } catch (TimeoutException &e) {
      doCommit();
    }
  }
  // do commit on exit
  doCommit();
  log() << "Comitter is exiting." << std::endl;
}

}
}
}

static inline gst::azure::storage::BlockAzureUploader *block_uploader(GstAzureUploader *uploader)
{
  return static_cast<gst::azure::storage::BlockAzureUploader *>(uploader->impl);
}

static inline std::shared_ptr<gst::azure::storage::AzureUploadLocation> &location(GstAzureUploader *uploader)
{
  return *(static_cast<std::shared_ptr<gst::azure::storage::AzureUploadLocation> *>(uploader->data));
}

// c interfaces
G_BEGIN_DECLS

gboolean gst_block_azure_uploader_init(GstAzureUploader *uploader, const gchar *container_name, const gchar *blob_name);
gboolean gst_block_azure_uploader_flush(GstAzureUploader *uploader);
gboolean gst_block_azure_uploader_destroy(GstAzureUploader *uploader);
gboolean gst_block_azure_uploader_upload(GstAzureUploader *uploader, const gchar *data, const gsize size);

GstAzureUploaderClass *getBlockUploaderClass()
{
  GstAzureUploaderClass *ret = new GstAzureUploaderClass();
  ret->init = gst_block_azure_uploader_init;
  ret->flush = gst_block_azure_uploader_flush;
  ret->destroy = gst_block_azure_uploader_destroy;
  ret->upload = gst_block_azure_uploader_upload;
  return ret;
}


GstAzureUploader *gst_azure_sink_block_uploader_new(const GstAzureSinkConfig *config) {
  static GstAzureUploaderClass *defaultClass = getBlockUploaderClass();
  if(config == NULL)
    return NULL;
  GstAzureUploader *uploader = new GstAzureUploader();
  if(uploader == NULL)
    return NULL;
  uploader->klass = defaultClass;
  uploader->impl = (void *)(new gst::azure::storage::BlockAzureUploader(
    config->account_name, config->account_key, (bool)config->use_https,
    config->block_size, config->worker_count, config->commit_block_count, config->commit_interval_ms));
  uploader->data = (void *)(new std::shared_ptr<gst::azure::storage::AzureUploadLocation>(nullptr));
  return uploader;
}

gboolean gst_block_azure_uploader_init(GstAzureUploader *uploader, const gchar *container_name, const gchar *blob_name)
{
  location(uploader) = block_uploader(uploader)->init(container_name, blob_name);
  return TRUE;
}

gboolean gst_block_azure_uploader_flush(GstAzureUploader *uploader)
{
  if(location(uploader) == nullptr)
    return FALSE;
  return block_uploader(uploader)->flush(location(uploader));
}

gboolean gst_block_azure_uploader_destroy(GstAzureUploader *uploader)
{
  if(location(uploader) == nullptr)
    return FALSE;
  return block_uploader(uploader)->destroy(location(uploader));
}

gboolean gst_block_azure_uploader_upload(GstAzureUploader *uploader, const gchar *data, const gsize size)
{
  if(location(uploader) == nullptr)
    return FALSE;
  return block_uploader(uploader)->upload(location(uploader), (const char *)data, (size_t)size);
}

G_END_DECLS