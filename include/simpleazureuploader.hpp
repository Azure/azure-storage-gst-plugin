#ifndef _AZURE_SINK_SIMPLE_AZURE_UPLOADER_HPP_
#define _AZURE_SINK_SIMPLE_AZURE_UPLOADER_HPP_

#include <map>
#include <utility>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include "utils/common.hpp"
#include "azureuploadercommon.hpp"

#include "storage_credential.h"
#include "storage_account.h"
#include "blob/blob_client.h"
#include "blob/append_block_request.h"

namespace gst {
namespace azure {
namespace storage {

class UploadWorker {
  std::shared_ptr<AzureUploadLocation> loc;
  std::mutex stream_lock, finish_lock;
  std::condition_variable new_cond, finish_cond;
  std::unique_ptr<std::stringstream> stream;
  bool finished;
  bool stopped;
  std::shared_ptr<::azure::storage_lite::blob_client> client;
  std::thread worker;
public:
  UploadWorker(std::shared_ptr<AzureUploadLocation> loc,
    std::shared_ptr<::azure::storage_lite::blob_client> client):
    loc(loc), stream(std::move(std::make_unique<std::stringstream>())), stopped(false), finished(true),
    client(client), worker([this] { this->run(); }) {}
  bool append(UploadBuffer buffer);
  void run();
  void flush();
  void stop();
};

const int AZURE_CLIENT_CONCCURRENCY = 8;

class SimpleAzureUploader {
private:
  std::shared_ptr<::azure::storage_lite::blob_client> client;
  std::map<std::shared_ptr<AzureUploadLocation>, std::unique_ptr<UploadWorker>> uploads;
public:
  SimpleAzureUploader(const char *account_name, const char *account_key, bool use_https);
  std::shared_ptr<AzureUploadLocation> init(const char *container_name, const char *blob_name);
  bool upload(std::shared_ptr<AzureUploadLocation> loc, const char *data, size_t size);
  bool flush(std::shared_ptr<AzureUploadLocation> loc);
  bool destroy(std::shared_ptr<AzureUploadLocation> loc);
};

}
}
}

#endif