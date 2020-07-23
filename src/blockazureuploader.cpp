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

BlockAzureUploader::BlockAzureUploader(const char *account_name, const char *account_key, bool use_https)
{
  std::string name(account_name);
  std::string key(account_key);
  auto credential = std::make_shared<::azure::storage_lite::shared_key_credential>(name, key);
  auto storage_account = std::make_shared<::azure::storage_lite::storage_account>(name, credential, use_https);
  client = std::make_shared<::azure::storage_lite::blob_client>(storage_account, WORKER_COUNT);
}

// Check if the location is correct.
// BlockAzureUploader only support one location at a time,
// and cannot be modified once specified
bool BlockAzureUploader::checkLoc(std::shared_ptr<AzureUploadLocation> loc)
{
  return this->loc != nullptr && this->loc == loc;
}

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
  for(unsigned i = 0; i < WORKER_COUNT; i++)
    workers[i] = std::async(&BlockAzureUploader::run, this);
  stream = std::make_unique<std::stringstream>();
  window_start = blockId = 0;
  commitWorker = std::async(&BlockAzureUploader::runCommit, this);
  return loc;
}

bool BlockAzureUploader::upload(std::shared_ptr<AzureUploadLocation> loc, const char *data, size_t size)
{
  if(!checkLoc(loc))
    return false;
  // Blocks are uploaded only if buffer(1MiB by default) overflows
  stream->write(data, size);
  if(getStreamLen(*stream) >= BLOCK_SIZE)
  {
    reqs.push(UploadJob{blockId++, std::move(stream)});
    stream = std::make_unique<std::stringstream>();
  }
  return true;
}

bool BlockAzureUploader::flush(std::shared_ptr<AzureUploadLocation> loc)
{
  if(!checkLoc(loc))
    return false;
  log() << "Flushing..." << std::endl;
  // commit remaining data
  if(getStreamLen(*stream) > 0) {
    log() << "Pushing uncommitted data..." << std::endl;
    reqs.push(UploadJob{ blockId++, std::move(stream) });
    stream = std::make_unique<std::stringstream>();
  }
  // wait for the request queue to become empty
  reqs.wait_empty();
  return true;
}


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

// multithreaded worker job
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

void BlockAzureUploader::runCommit()
{
  using ::azure::storage_lite::put_block_list_request_base;
  log() << "Initializing committer." << std::endl;
  while(1)
  {
    // fetch all responses already enqueued
    try {
      UploadResponse resp;
      do {
        resp = resps.pop();
        if(resp.code == UploadResponse::OK)
        {
          window.push_back(resp.id);
          std::push_heap(window.begin(), window.end(), [](blockid_t a, blockid_t b) { return a > b; });
          log() << "Committer received response of id " << resp.id << std::endl;
        }
      } while(!resps.empty());
    } catch (ClosedException &e) {
      log() << "Response queue is closed." << std::endl;
      break;
    }
    while(window_start == window.front())
    {
      std::pop_heap(window.begin(), window.end(), [](blockid_t a, blockid_t b) { return a > b; });
      window.pop_back();
      window_start += 1;
      // log() << "Pushing window start to " << window_start << std::endl;
    }
  }
  // finally do commit
  std::vector<put_block_list_request_base::block_item> block_list;
  for(blockid_t id = 0; id < window_start; id++)
    block_list.push_back(put_block_list_request_base::block_item{
      base64_encode(id),
      put_block_list_request_base::block_type::latest
    });
  // no metadata needed
  auto metadata = std::vector<std::pair<std::string, std::string>>();
  auto fut = client->put_block_list(loc->first, loc->second, block_list, metadata);
  auto result = fut.get();
  handle(result);
  if(result.success()) {
    log() << "Committed, last block id = " << window_start << std::endl;
  }
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
    config->account_name, config->account_key, (bool)config->use_https));
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