#include "azuredownloader.hpp"
#include "azuredownloader.h"
#include "gstazuredownloader.h"

#include <string>
#include <gst/gst.h>

#include "utils/common.hpp"

namespace gst {
namespace azure {
namespace storage {

using namespace ::azure::storage_lite;

AzureDownloader::AzureDownloader(
  const std::string &account_name, const std::string &account_key,
  bool use_https, size_t worker_count, size_t block_size, size_t prefetch_size)
  :loc(nullptr), worker_count(worker_count), block_size(block_size), prefetch_size(prefetch_size)
{
  auto credential = std::make_shared<shared_key_credential>(account_name, account_key);
  auto account = std::make_shared<storage_account>(account_name, credential, use_https);
  client = std::make_shared<blob_client>(account, worker_count);
  window_start = 0;
  prefetch_cursor = 0;
  prefetch_buffer = new char[prefetch_size];
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

size_t AzureDownloader::read(char* buffer, size_t size)
{
  if (loc == nullptr)
    return 0;
  size_t off = 0;
  // concat to blob end
  size = std::max(blob_size - next_cursor, size);
  // split it according to block size
  do {
    size_t cur = std::min(size - off, block_size);
    reqs.push(ReadBlock{ next_cursor, cur, buffer + off });
    off += cur;
    next_cursor += cur;
  } while (off < size);
  // prefetch
  size_t prefetch_size = std::max(blob_size - next_cursor, prefetch_size);

  // wait for buffer filling to complete
  std::unique_lock<std::mutex> lk(comp_lock);
  comp_cond.wait(lk, [=]() { return this->window_start >= this->next_cursor; });
  return size;
}

// change cursor to offset
bool AzureDownloader::seek(size_t offset)
{
  // clear window
  std::unique_lock<std::mutex> lk(comp_lock);
  window_start = next_cursor = offset;
  window.clear();
  return true;
}

bool AzureDownloader::destroy()
{
  // close queue, wait for all workers to finish
  reqs.close();
  for (auto& fut : workers)
    fut.wait();
}

void AzureDownloader::push_window(const ReadBlock blk)
{
  const auto cmp = [](ReadBlock& a, ReadBlock& b) { return a.offset > b.offset; };
  // update read window
  std::unique_lock<std::mutex> lk(comp_lock);
  window.push_back(blk);
  std::push_heap(window.begin(), window.end(), cmp);
  // it is possible that window front is less than window_start
  // if a seek operation is performed and previous download jobs are not completed.
  while (window_start <= window.front().offset)
  {
    std::pop_heap(window.begin(), window.end(), cmp);
    window_start = std::max(window_start, window.back().offset + window.back().size);
    window.pop_back();
    comp_cond.notify_all();
  }
}

void AzureDownloader::run()
{
  while (1)
  {
    ReadBlock req;
    try {
      req = reqs.pop();
    } catch (ClosedException e) {
      break;
    }
    do {
      auto fut = client->download_blob_to_buffer(
        loc->first, loc->second, req.offset, req.size, req.buffer, 1);
      auto result = fut.get();
      handle(result, log());
      if (result.success())
        break;
      log() << "Retrying..." << std::endl;
    } while (1);
    push_window(req);
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