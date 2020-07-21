#ifndef _AZURE_SINK_BLOCK_AZURE_UPLOADER_HPP_
#define _AZURE_SINK_BLOCK_AZURE_UPLOADER_HPP_

#include <map>
#include <utility>
#include <memory>
#include <thread>
#include <vector>
#include <mutex>
#include <atomic>
#include <sstream>
#include <condition_variable>
#include "util/utils.hpp"

#include "storage_credential.h"
#include "storage_account.h"
#include "blob/blob_client.h"
#include "blob/append_block_request.h"

#include "simpleazureuploader.hpp"

namespace gst {
namespace azure {
namespace storage {

// only supports one location at a time.
class BlockUploadWorker {
  std::shared_ptr<AzureUploadLocation> loc;
  std::mutex stream_lock, finish_lock;
  std::unique_ptr<std::stringstream> stream;
  std::condition_variable stream_cond, finish_cond;
  std::atomic_bool finished;
  std::atomic_bool stopped;
  std::shared_ptr<::azure::storage_lite::blob_client> client;
  std::thread worker;
public:
  BlockUploadWorker(std::shared_ptr<AzureUploadLocation> loc,
    std::shared_ptr<::azure::storage_lite::blob_client> client):
    loc(loc), stopped(false), finished(true),
    client(client), worker([this] { this->run(); }) {}
  bool append(UploadBuffer buffer);
  void run();
  void flush();
  void stop();

private:
  std::ostream &log();
  static long long int getBlockId();
  static std::atomic_llong counter;
};

// const int AZURE_CLIENT_CONCCURRENCY = 8;

class BlockAzureUploader {
private:
  std::shared_ptr<::azure::storage_lite::blob_client> client;
  std::shared_ptr<AzureUploadLocation> loc;
  std::vector<BlockUploadWorker> workers;
public:
  BlockAzureUploader(const char *account_name, const char *account_key, bool use_https);
  std::shared_ptr<AzureUploadLocation> init(const char *container_name, const char *blob_name);
  bool upload(std::shared_ptr<AzureUploadLocation> loc, const char *data, size_t size);
  bool flush(std::shared_ptr<AzureUploadLocation> loc);
  bool destroy(std::shared_ptr<AzureUploadLocation> loc);
private:
  BlockUploadWorker &getWorker();
};

}
}
}

#endif