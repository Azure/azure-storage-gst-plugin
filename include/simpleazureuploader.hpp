#ifndef _AZURE_ELEMENTS_SIMPLE_AZURE_UPLOADER_HPP_
#define _AZURE_ELEMENTS_SIMPLE_AZURE_UPLOADER_HPP_

#include <map>
#include <utility>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include "utils/common.hpp"
#include "azurecommon.hpp"

#include "storage_credential.h"
#include "storage_account.h"
#include "blob/blob_client.h"
#include "blob/append_block_request.h"

namespace gst {
namespace azure {
namespace storage {

class UploadWorker {
  std::shared_ptr<AzureLocation> loc;
  std::mutex stream_lock, finish_lock;
  std::condition_variable new_cond, finish_cond;
  std::unique_ptr<std::stringstream> stream;
  bool finished;
  bool stopped;
  std::shared_ptr<::azure::storage_lite::blob_client> client;
  std::future<void> bg_worker;
public:
  UploadWorker(std::shared_ptr<AzureLocation> loc,
    std::shared_ptr<::azure::storage_lite::blob_client> client) :
    loc(loc), stream(std::move(std::make_unique<std::stringstream>())), finished(true), stopped(false),
    client(client), bg_worker(std::async(&UploadWorker::run, this)) {}
  ~UploadWorker() { stop(); }
  bool append(const char *, size_t);
  void run();
  void flush();
  void stop();
};

class SimpleAzureUploader {
private:
  std::shared_ptr<::azure::storage_lite::blob_client> client;
  std::shared_ptr<AzureLocation> loc;
  std::unique_ptr<UploadWorker> worker;
public:
  const int AZURE_CLIENT_CONCCURRENCY = 4;
  SimpleAzureUploader(std::string account_name, std::string account_key, bool use_https);
  std::shared_ptr<AzureLocation> init(std::string container_name, std::string blob_name);
  bool upload(const char *data, size_t size);
  bool flush();
  bool destroy();
};

}
}
}

#endif