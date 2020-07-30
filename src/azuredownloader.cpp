#include "azuredownloader.hpp"
#include "azuredownloader.h"
#include "gstazuredownloader.h"

#include <string>
#include <cassert>
#include <gst/gst.h>

#include "utils/common.hpp"

namespace gst {
namespace azure {
namespace storage {

using namespace ::azure::storage_lite;

AzureDownloader::AzureDownloader(
  const std::string &account_name, const std::string &account_key,
  bool use_https, size_t worker_count, size_t block_size, size_t prefetch_block_count)
  :loc(nullptr), worker_count(worker_count), block_size(block_size), prefetch_block_count(prefetch_block_count)
{
  auto credential = std::make_shared<shared_key_credential>(account_name, account_key);
  auto account = std::make_shared<storage_account>(account_name, credential, use_https);
  client = std::make_shared<blob_client>(account, worker_count);
  read_cursor = write_cursor = 0;
}

std::shared_ptr<AzureLocation>
AzureDownloader::init(const std::string &container_name, const std::string &blob_name)
{
  // check if the blob exist and get its size
  auto fut = client->get_blob_properties(container_name, blob_name);
  auto result = fut.get();
  handle(result, log());
  if (result.success())
  {
    auto resp = result.response();
    blob_size = resp.size;
    loc = std::make_shared<AzureLocation>(container_name, blob_name);
  } else {
    return nullptr; // failed
  }
  // initialize worker threads
  workers.clear();
  for (unsigned i = 0; i < worker_count; i++)
    workers.emplace_back(std::async(&AzureDownloader::run, this));
  return loc;
}

// this function is NOT re-entrant
size_t AzureDownloader::read(char* buffer, size_t size)
{
  std::lock_guard<std::mutex> guard(read_lock);
  if (loc == nullptr)
    return 0;
  
  std::unique_lock<std::mutex> lk(comp_lock);

  // push jobs first, including prefetch blocks
  size_t target = read_cursor + size;
  size_t req_target = std::min(blob_size, target + prefetch_block_count * block_size);
  while (write_cursor < req_target)
  {
    size_t blk_size = std::min(block_size, blob_size - write_cursor);
    reqs.push(ReadRequest{ write_cursor, blk_size });
    write_cursor += blk_size;
  }

  while (read_cursor < target)
  {
    comp_cond.wait(lk, [=] { return !this->window.empty() && this->window.front().req.offset <= this->read_cursor;  });
    ReadResponse &resp = window.front();
    size_t new_read_cursor = std::min(target, resp.req.offset + resp.req.buf_size);
    log() << "Read directly " << (new_read_cursor - read_cursor) / 1024 << "KiB." << std::endl;
    memcpy(buffer, resp.buf + read_cursor - resp.req.offset, new_read_cursor - read_cursor);
    buffer += new_read_cursor - read_cursor;
    if (new_read_cursor == resp.req.offset + resp.req.buf_size)
    {
      delete[] resp.buf;
      std::pop_heap(window.begin(), window.end(), std::greater<ReadResponse>());
      window.pop_back();
    }
    read_cursor = new_read_cursor;
  }
  return size;
}

// change cursor to offset
bool AzureDownloader::seek(size_t offset)
{
  // clear window
  std::unique_lock<std::mutex> lk(comp_lock);
  read_cursor = write_cursor = offset;
  window.clear();
  return true;
}

bool AzureDownloader::destroy()
{
  // close queue, wait for all workers to finish
  reqs.close();
  for (auto& fut : workers)
    fut.wait();
  return true;
}

void AzureDownloader::run()
{
  while (1)
  {
    ReadRequest req;
    try {
      req = reqs.pop();
    } catch (ClosedException &e) {
      break;
    }
    do {
      char* buf = new char[req.buf_size];
      if (buf == nullptr)
        log() << "Failed to allocate buffer." << std::endl;
      auto fut = client->download_blob_to_buffer(
        loc->first, loc->second, req.offset, req.buf_size, buf, 1);
      auto result = fut.get();
      handle(result, log()); 
      if (result.success()) {
        std::unique_lock<std::mutex> lk(comp_lock);
        window.push_back(ReadResponse{req, buf});
        std::push_heap(window.begin(), window.end(), std::greater<ReadResponse>());
        comp_cond.notify_one();
        break;
      }
      log() << "Retrying..." << std::endl;
    } while (1);
  }
  log() << "Worker exitting..." << std::endl;
}

}
}
}


static inline gst::azure::storage::AzureDownloader* dl(GstAzureDownloader* d) {
  return static_cast<gst::azure::storage::AzureDownloader*>(d->impl);
}

static inline std::shared_ptr<gst::azure::storage::AzureLocation>& location(GstAzureDownloader* d) {
  return *(static_cast<std::shared_ptr<gst::azure::storage::AzureLocation>*>(d->data));
}

// c interfaces
G_BEGIN_DECLS


gboolean downloader_init(GstAzureDownloader *downloader, const gchar *container_name, const gchar *blob_name);
gboolean downloader_read(GstAzureDownloader *downloader, gchar *buffer, gsize size);
gboolean downloader_seek(GstAzureDownloader *downloader, goffset offset);
gboolean downloader_destroy(GstAzureDownloader *downloader);

static const GstAzureDownloaderClass downloader_class = {
  .init = downloader_init,
  .read = downloader_read,
  .seek = downloader_seek,
  .destroy = downloader_destroy
};

GstAzureDownloader* gst_azure_src_downloader_new(const GstAzureSrcConfig* config)
{
  if (config == NULL)
    return NULL;
  GstAzureDownloader* downloader = new GstAzureDownloader();
  if (downloader == NULL)
    return NULL;
  size_t read_ahead = (config->read_ahead_size < 0) ? (config->block_size) : (size_t)(config->read_ahead_size);
  downloader->klass = &downloader_class;
  downloader->impl = (void*)(new gst::azure::storage::AzureDownloader(
    config->account_name, config->account_key, config->use_https,
    config->worker_count, config->block_size, read_ahead
  ));
  downloader->data = (void*)(new std::shared_ptr <gst::azure::storage::AzureLocation>(nullptr));
  return downloader;
}

gboolean downloader_init(GstAzureDownloader* downloader, const gchar* container_name, const gchar* blob_name)
{
  location(downloader) = dl(downloader)->init(container_name, blob_name);
  return TRUE;
}
gboolean downloader_read(GstAzureDownloader* downloader, gchar* buffer, gsize size)
{
  return dl(downloader)->read(buffer, size);
}
gboolean downloader_seek(GstAzureDownloader* downloader, goffset offset)
{
  return dl(downloader)->seek(offset);
}
gboolean downloader_destroy(GstAzureDownloader* downloader)
{
  return dl(downloader)->destroy();
}

G_END_DECLS