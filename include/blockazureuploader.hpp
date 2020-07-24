#ifndef _AZURE_SINK_BLOCK_AZURE_UPLOADER_HPP_
#define _AZURE_SINK_BLOCK_AZURE_UPLOADER_HPP_

#include <map>
#include <utility>
#include <memory>
#include <thread>
#include <array>
#include <mutex>
#include <atomic>
#include <sstream>
#include <queue>
#include <chrono>
#include <condition_variable>

#include "azureuploadercommon.hpp"
#include "utils/common.hpp"
#include "utils/blockingqueue.hpp"

#include "storage_credential.h"
#include "storage_account.h"
#include "blob/blob_client.h"
#include "blob/put_block_list_request.h"

namespace gst {
namespace azure {
namespace storage {

class BlockAzureUploader {
private:
  typedef long long unsigned blockid_t;
  struct UploadJob {
    blockid_t id;
    std::unique_ptr<std::stringstream> stream;
  };
  struct UploadResponse {
    enum {
      OK,
      FAIL
    } code;
    blockid_t id;
  };
  // configurations
  unsigned long block_size;
  unsigned long worker_count;
  unsigned long commit_block_count;
  unsigned long commit_interval_ms;

  // members
  std::shared_ptr<::azure::storage_lite::blob_client> client;
  std::shared_ptr<AzureUploadLocation> loc;
  std::unique_ptr<std::stringstream> stream;
  std::vector<std::future<void>> workers;
  std::future<void> commitWorker;
  BlockingQueue<UploadJob> reqs;
  BlockingQueue<UploadResponse> resps;
  blockid_t completedId;
  blockid_t nextId;
  blockid_t committedId;
  std::vector<blockid_t> window;
  std::chrono::_V2::steady_clock::time_point lastCommit;
  std::vector<::azure::storage_lite::put_block_list_request_base::block_item>
    block_list;

public:
  BlockAzureUploader(
    const char *account_name, const char *account_key, bool use_https,
    unsigned long block_size, unsigned long worker_count,
    unsigned long commit_block_count, unsigned long commit_interval_ms);
  std::shared_ptr<AzureUploadLocation> init(const char *container_name, const char *blob_name);
  bool upload(std::shared_ptr<AzureUploadLocation> loc, const char *data, size_t size);
  bool flush(std::shared_ptr<AzureUploadLocation> loc);
  bool destroy(std::shared_ptr<AzureUploadLocation> loc);
private:
  void commitBlock(blockid_t id);
  void doCommit();
  bool checkLoc(std::shared_ptr<AzureUploadLocation> loc);
  void run();
  void runCommit();
};

}
}
}

#endif