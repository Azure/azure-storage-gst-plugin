// async azure uploader using azure-storage-cpplite
#include "simpleazureuploader.h"
#include "simpleazureuploader.hpp"
#include "utils/common.hpp"
#include <gst/gst.h>
#include <iostream>
#include <iomanip>

// FIXME use a specific logger

namespace gst {
namespace azure {
namespace storage {

// individual uploader logics
bool UploadWorker::append(UploadBuffer buffer)
{
  const std::lock_guard<std::mutex> lock(stream_lock);
  log() << "Appending, length = " << buffer.second << std::endl;
  stream->write(buffer.first, buffer.second);
  finish_lock.lock();
  finished = false;
  finish_lock.unlock();
  new_cond.notify_all();
  return true;
}

// FIXME find ways to avoid memory copy
void UploadWorker::run()
{
  log() << "Worker is running." << std::endl;
  // first create the blob
  auto fut = client->create_append_blob(loc->first, loc->second);
  auto result = fut.get();
  handle(result, log());
  
  while(!stopped)
  {
    if(stream->rdbuf()->in_avail() == 0)
    {
      // notify all that stream is flushed
      finish_lock.lock();
      finished = true;
      finish_lock.unlock();
      finish_cond.notify_all();
      // wait for new content while not stopped
      std::unique_lock<std::mutex> lk(stream_lock);
      new_cond.wait(lk);
      if(stopped) break;
      lk.unlock();
    }
    stream_lock.lock();
    auto saved_stream = std::move(stream);
    stream = std::make_unique<std::stringstream>();
    stream_lock.unlock();

    // get content length for debugging
    auto cur = saved_stream->tellg();
    saved_stream->seekg(0, std::ios_base::end);
    auto end = saved_stream->tellg();
    saved_stream->seekg(cur);

    log() << "Uploading content, length = " << static_cast<unsigned int>(end - cur) << std::endl;
    // re-commit the stream object and transfer it
    // do not need explicit retry mechanism here, as unsent stream
    // will be automatically recommitted here
    
    auto fut = this->client->append_block_from_stream(loc->first, loc->second, *saved_stream);
    auto result = fut.get();
    handle(result, log());
  }
  // exit if stopped
  log() << "Worker is exiting." << std::endl;
}

void UploadWorker::flush()
{
  // wait for the whole stream to complete uploading
  log() << "Flushing." << std::endl;
  std::unique_lock<std::mutex> lk(finish_lock);
  finish_cond.wait(lk, [this] { return this->finished; });
}

void UploadWorker::stop()
{
  log() << "Stopping." << std::endl;
  stopped = true;
  new_cond.notify_all();
  worker.join();
}

SimpleAzureUploader::SimpleAzureUploader(const char *account_name, const char *account_key, bool use_https)
{
  std::string name(account_name);
  std::string key(account_key);
  auto credential = std::make_shared<::azure::storage_lite::shared_key_credential>(name, key);
  auto storage_account = std::make_shared<::azure::storage_lite::storage_account>(name, credential, use_https);
  this->client = std::make_shared<::azure::storage_lite::blob_client>(storage_account, AZURE_CLIENT_CONCCURRENCY);
}

// create a new stream.
std::shared_ptr<AzureUploadLocation>
SimpleAzureUploader::init(const char *container_name, const char *blob_name)
{
  auto ret = std::make_shared<AzureUploadLocation>(std::string(container_name), std::string(blob_name));
  // build a new upload worker in place
  auto worker = std::make_unique<UploadWorker>(ret, this->client);
  this->uploads.emplace(ret, move(worker));
  return std::move(ret);
}

// append block of data. Push data to queue and return immediately.
bool
SimpleAzureUploader::upload(std::shared_ptr<AzureUploadLocation> loc, const char *data, size_t size)
{
  if(!size) {
    return true;
  }
  // append it
  auto worker_iter = uploads.find(loc);
  if(worker_iter == uploads.end()) {
    // not found
    return false;
  } else {
    return worker_iter->second->append(UploadBuffer(data, size));
  }
}

bool
SimpleAzureUploader::flush(std::shared_ptr<AzureUploadLocation> loc)
{
  auto worker_iter = uploads.find(loc);
  // wait for specific location to flush
  if(worker_iter == uploads.end()) {
    std::cerr << "Warning: flushing a non-existent location."
      << (loc->first) << ' ' << (loc->second) << std::endl;
    return true;
  }
  worker_iter->second->flush();
  return true;
}

bool
SimpleAzureUploader::destroy(std::shared_ptr<AzureUploadLocation> loc)
{
  // wait for specific location to flush
  auto worker_iter = uploads.find(loc);
  if(worker_iter == uploads.end()) {
    std::cerr << "Flushing a non-existent location."
      << (loc->first) << ' ' << (loc->second) << std::endl;
    return true;
  }
  worker_iter->second->flush();
  // FIXME possible race condition, new data might be injected in between
  // possibility is scarce though
  worker_iter->second->stop();
  uploads.erase(worker_iter);
  return true;
}

}
}
}

static inline gst::azure::storage::SimpleAzureUploader *simple_uploader(GstAzureUploader *uploader)
{
  return static_cast<gst::azure::storage::SimpleAzureUploader *>(uploader->impl);
}

static inline std::shared_ptr<gst::azure::storage::AzureUploadLocation> &location(GstAzureUploader *uploader)
{
  return *(static_cast<std::shared_ptr<gst::azure::storage::AzureUploadLocation> *>(uploader->data));
}

// c interfaces
G_BEGIN_DECLS

gboolean gst_simple_azure_uploader_init(GstAzureUploader *uploader, const gchar *container_name, const gchar *blob_name);
gboolean gst_simple_azure_uploader_flush(GstAzureUploader *uploader);
gboolean gst_simple_azure_uploader_destroy(GstAzureUploader *uploader);
gboolean gst_simple_azure_uploader_upload(GstAzureUploader *uploader, const gchar *data, const gsize size);

GstAzureUploaderClass *getSimpleUploaderClass()
{
  GstAzureUploaderClass *ret = new GstAzureUploaderClass();
  ret->init = gst_simple_azure_uploader_init;
  ret->flush = gst_simple_azure_uploader_flush;
  ret->destroy = gst_simple_azure_uploader_destroy;
  ret->upload = gst_simple_azure_uploader_upload;
  return ret;
}


GstAzureUploader *gst_azure_sink_uploader_new(const GstAzureSinkConfig *config) {
  static GstAzureUploaderClass *defaultClass = getSimpleUploaderClass();
  if(config == NULL)
    return NULL;
  GstAzureUploader *uploader = new GstAzureUploader();
  if(uploader == NULL)
    return NULL;
  uploader->klass = defaultClass;
  uploader->impl = (void *)(new gst::azure::storage::SimpleAzureUploader(
    config->account_name, config->account_key, (bool)config->use_https));
  uploader->data = (void *)(new std::shared_ptr<gst::azure::storage::AzureUploadLocation>(nullptr));
  return uploader;
}

gboolean gst_simple_azure_uploader_init(GstAzureUploader *uploader, const gchar *container_name, const gchar *blob_name)
{
  location(uploader) = simple_uploader(uploader)->init(container_name, blob_name);
  return TRUE;
}

gboolean gst_simple_azure_uploader_flush(GstAzureUploader *uploader)
{
  if(location(uploader) == nullptr)
    return FALSE;
  return simple_uploader(uploader)->flush(location(uploader));
}

gboolean gst_simple_azure_uploader_destroy(GstAzureUploader *uploader)
{
  if(location(uploader) == nullptr)
    return FALSE;
  return simple_uploader(uploader)->destroy(location(uploader));
}

gboolean gst_simple_azure_uploader_upload(GstAzureUploader *uploader, const gchar *data, const gsize size)
{
  if(location(uploader) == nullptr)
    return FALSE;
  return simple_uploader(uploader)->upload(location(uploader), (const char *)data, (size_t)size);
}

G_END_DECLS