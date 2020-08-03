// async azure uploader using azure-storage-cpplite
#include "simpleazureuploader.h"
#include "simpleazureuploader.hpp"
#include "utils/common.hpp"
#include <gst/gst.h>
#include <iostream>
#include <iomanip>

namespace gst {
namespace azure {
namespace storage {

#define APPEND_BLOCK_MAX_BLOCK_SIZE (2*1024*1024)

// individual uploader logics
bool UploadWorker::append(const char *buf, size_t size)
{
  const std::lock_guard<std::mutex> lock(stream_lock);
  log() << "Appending, length = " << size << std::endl;
  stream->write(buf, size);
  finish_lock.lock();
  finished = false;
  finish_lock.unlock();
  new_cond.notify_all();
  return true;
}

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
      finish_cond.notify_all();
      finish_lock.unlock();
      // wait for new content while not stopped
      std::unique_lock<std::mutex> lk(stream_lock);
      new_cond.wait(lk, [this] {
        return this->stopped || this->stream->rdbuf()->in_avail() > 0;
        });
      if(stopped) break;
    }
    stream_lock.lock();
    // get 2MiB-max trunks from stream
    std::unique_ptr<std::stringstream> saved_stream = nullptr;
    if(getStreamLen(*stream) <= APPEND_BLOCK_MAX_BLOCK_SIZE)
    {
      saved_stream = std::move(stream);
      stream = std::make_unique<std::stringstream>();
    } else {
      // read from stream
      std::string buf;
      buf.reserve(APPEND_BLOCK_MAX_BLOCK_SIZE);
      std::copy_n(std::istreambuf_iterator<char>(*stream), APPEND_BLOCK_MAX_BLOCK_SIZE, std::back_inserter(buf));
      saved_stream = std::make_unique<std::stringstream>(move(buf));
    }
    stream_lock.unlock();

    // get content length for debugging
    auto cur = saved_stream->tellg();
    saved_stream->seekg(0, std::ios_base::end);
    auto end = saved_stream->tellg();
    saved_stream->seekg(cur);

    log() << "Uploading content, length = " << static_cast<unsigned int>(end - cur) << std::endl;

    // do not need explicit retry mechanism here, as unsent stream
    // will be automatically recommitted
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
  new_cond.notify_one();
  worker.wait();
}

SimpleAzureUploader::SimpleAzureUploader(std::string account_name, std::string account_key, bool use_https)
{
  auto credential = std::make_shared<::azure::storage_lite::shared_key_credential>(account_name, account_key);
  auto storage_account = std::make_shared<::azure::storage_lite::storage_account>(account_name, credential, use_https);
  client = std::make_shared<::azure::storage_lite::blob_client>(storage_account, AZURE_CLIENT_CONCCURRENCY);
}

// create a new stream.
std::shared_ptr<AzureLocation>
SimpleAzureUploader::init(std::string container_name, std::string blob_name)
{
  loc = std::make_shared<AzureLocation>(container_name, blob_name);
  // build a new upload worker in place
  worker = std::make_unique<UploadWorker>(loc, this->client);
  return loc;
}

// Append block of data in a non-blocking manner.
bool
SimpleAzureUploader::upload(const char *data, size_t size)
{
  if (loc == nullptr)
  {
    log() << "Warning: Upload called before uploader is initialized." << std::endl;
    return true;
  }
  if (size == 0)
    return true;
  return worker->append(data, size);
}

bool
SimpleAzureUploader::flush()
{
  // wait for specific location to flush
  if(loc == nullptr)
  {
    log() << "Warning: Flush called before uploader is initialized." << std::endl;
    return true;
  }
  worker->flush();
  return true;
}

bool
SimpleAzureUploader::destroy()
{
  // wait for specific location to flush
  if(loc == nullptr)
  {
    log() << "Warning: Flush called before uploader is initialized." << std::endl;
    return true;
  }
  // possible race condition between flush & stop
  // does not matter though
  worker->flush();
  worker->stop();
  loc = nullptr;
  worker = nullptr;
  return true;
}

}
}
}

static inline gst::azure::storage::SimpleAzureUploader *simple_uploader(GstAzureUploader *uploader)
{
  return static_cast<gst::azure::storage::SimpleAzureUploader *>(uploader->impl);
}

static inline std::shared_ptr<gst::azure::storage::AzureLocation> &location(GstAzureUploader *uploader)
{
  return *(static_cast<std::shared_ptr<gst::azure::storage::AzureLocation> *>(uploader->data));
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
  uploader->data = (void *)(new std::shared_ptr<gst::azure::storage::AzureLocation>(nullptr));
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
  return simple_uploader(uploader)->flush();
}

gboolean gst_simple_azure_uploader_destroy(GstAzureUploader *uploader)
{
  if(location(uploader) == nullptr)
    return FALSE;
  return simple_uploader(uploader)->destroy();
}

gboolean gst_simple_azure_uploader_upload(GstAzureUploader *uploader, const gchar *data, const gsize size)
{
  if(location(uploader) == nullptr)
    return FALSE;
  return simple_uploader(uploader)->upload((const char *)data, (size_t)size);
}

G_END_DECLS