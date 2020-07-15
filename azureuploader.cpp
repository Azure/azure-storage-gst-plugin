// async azure uploader using azure-storage-cpplite

// FIXME use a specific logger
#include <iostream>

#include "azureuploader.hpp"
#include "utils.h"

namespace gst {
namespace azure {
namespace storage {

// individual uploader logics
bool UploadWorker::append(UploadBuffer buffer)
{
  std::cerr << "Thread id " << worker.get_id() << " appending, length = " << buffer.second << std::endl;
  const std::lock_guard<std::mutex> lock(new_lock);
  stream.write(buffer.first, buffer.second);
  new_cond.notify_all();
}

// FIXME avoid memory copy
void UploadWorker::run()
{
  std::cerr << "Worker is running, thread id is " << worker.get_id() << std::endl;
  // first create the blob
  auto fut = client->create_append_blob(loc->first, loc->second);
  auto result = fut.get();
  handle(result);
  
  while(!stopped)
  {
    if(stream.rdbuf()->in_avail() == 0)
    {
      // notify all that stream is flushed
      finish_lock.lock();
      finished = true;
      finish_lock.unlock();
      finish_cond.notify_all();
      // wait for new content while not stopped
      std::unique_lock<std::mutex> lk(new_lock);
      new_cond.wait(lk);
      if(stopped) break;
      lk.unlock();
    }
    // get content length for debugging
    auto cur = stream.tellg();
    stream.seekg(0, std::ios_base::end);
    auto end = stream.tellg();
    stream.seekg(cur);
    std::cerr << "Uploading content, length = " << static_cast<unsigned int>(end - cur) << std::endl;
    // re-commit the stream object and transfer it
    auto fut = this->client->append_block_from_stream(loc->first, loc->second, stream);
    fut.wait();
    auto result = fut.get();
    if(!result.success())
    {
      std::cerr << "Warning result failed." << std::endl;
      std::cerr << result.error().code << "(" << result.error().code_name << std::endl;
      std::cerr << result.error().message << std::endl;
      // we don't have to handle it as it will remain in the buffer
    }
  }
  // exit if stopped
  std::cerr << "Worker is exiting, thread id is " << worker.get_id() << std::endl;
}

void UploadWorker::flush()
{
  // wait for the whole stream to complete uploading
  std::cerr << "Flushing thread id" << worker.get_id() << std::endl;
  std::unique_lock<std::mutex> lk(finish_lock);
  finish_cond.wait(lk, [this] { return this->finished; });
  lk.unlock();
}

void UploadWorker::stop()
{
  std::cerr << "Stopping worker id " << worker.get_id() << std::endl;
  stopped = true;
  new_cond.notify_all();
  worker.join();
}

AzureUploader::AzureUploader(const char *account_name, const char *account_key, bool use_https)
{
  std::string name(account_name);
  std::string key(account_key);
  auto credential = std::make_shared<::azure::storage_lite::shared_key_credential>(name, key);
  auto storage_account = std::make_shared<::azure::storage_lite::storage_account>(name, credential, use_https);
  // TODO parameterize the number of threads.
  this->client = std::make_shared<::azure::storage_lite::blob_client>(storage_account, AZURE_CLIENT_CONCCURRENCY);
}

// create a new stream.
std::shared_ptr<AzureUploadLocation>
AzureUploader::init(const char *container_name, const char *blob_name)
{
  auto ret = std::make_shared<AzureUploadLocation>(std::string(container_name), std::string(blob_name));
  // build a new upload worker in place
  auto worker = std::make_unique<UploadWorker>(ret, this->client);
  this->uploads.emplace(ret, move(worker));
  return std::move(ret);
}

// append block of data. Push data to queue and return immediately.
bool
AzureUploader::upload(std::shared_ptr<AzureUploadLocation> loc, const char *data, size_t size)
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
AzureUploader::flush(std::shared_ptr<AzureUploadLocation> loc)
{
  auto worker_iter = uploads.find(loc);
  // wait for specific location to flush
  if(worker_iter == uploads.end()) {
    std::cerr << "Flushing a non-existent location."
      << (loc->first) << ' ' << (loc->second) << std::endl;
    return true;
  }
  worker_iter->second->flush();
  return true;
}

bool
AzureUploader::destroy(std::shared_ptr<AzureUploadLocation> loc)
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
  worker_iter->second->stop();
  uploads.erase(worker_iter);
  return true;
}

}
}
}