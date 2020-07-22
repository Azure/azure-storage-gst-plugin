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
#include <condition_variable>

#include "azureuploadercommon.hpp"
#include "util/utils.hpp"
#include "util/blockingqueue.hpp"

#include "storage_credential.h"
#include "storage_account.h"
#include "blob/blob_client.h"
#include "blob/append_block_request.h"

namespace gst {
namespace azure {
namespace storage {

const unsigned int BLOCK_SIZE = 1048576;
const unsigned int WORKER_COUNT = 8;
class BlockAzureUploader {
private:
  typedef long long unsigned blockid_t;
  struct UploadJob
  {
    blockid_t id;
    std::unique_ptr<std::stringstream> stream;
  };
  struct UploadResponse
  {
    enum {
      OK,
      FAIL
    } code;
    blockid_t id;
  };
  std::shared_ptr<::azure::storage_lite::blob_client> client;
  std::shared_ptr<AzureUploadLocation> loc;
  std::unique_ptr<std::stringstream> stream;
  std::array<std::future<void>, WORKER_COUNT> workers;
  std::future<void> commitWorker;
  BlockingQueue<UploadJob> reqs;
  BlockingQueue<UploadResponse> resps;
  blockid_t window_start;
  std::vector<blockid_t> window;
  
  std::ostream &log();
  blockid_t blockId;

public:
  BlockAzureUploader(const char *account_name, const char *account_key, bool use_https);
  std::shared_ptr<AzureUploadLocation> init(const char *container_name, const char *blob_name);
  bool upload(std::shared_ptr<AzureUploadLocation> loc, const char *data, size_t size);
  bool flush(std::shared_ptr<AzureUploadLocation> loc);
  bool destroy(std::shared_ptr<AzureUploadLocation> loc);
private:
  bool checkLoc(std::shared_ptr<AzureUploadLocation> loc);
  void run();
  void runCommit();
};

}
}
}

#endif