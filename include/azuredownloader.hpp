#ifndef _AZURE_ELEMENTS_AZURE_DOWNLOADER_HPP_
#define _AZURE_ELEMENTS_AZURE_DOWNLOADER_HPP_

#include <gst/gst.h>
#include "azurecommon.hpp"
#include "utils/blockingqueue.hpp"
#include "storage_credential.h"
#include "storage_account.h"
#include "blob/blob_client.h"

namespace gst {
namespace azure {
namespace storage {

class AzureDownloader
{
  struct ReadBlock
  {
    size_t offset;
    size_t size;
    char* buffer;
  };
  
  // configurations
  std::shared_ptr<AzureLocation> loc;
  size_t blob_size;
  unsigned worker_count;
  size_t block_size;
  size_t prefetch_size;

  // members
  std::shared_ptr<::azure::storage_lite::blob_client> client;
  BlockingQueue<ReadBlock> reqs;
  std::vector<std::future<void>> workers;
  std::vector<ReadBlock> window;
  size_t window_start;
   
  // circular prefetch buffer
  size_t prefetch_offset;
  char* prefetch_buffer;

  size_t next_cursor;
  std::mutex comp_lock;
  std::condition_variable comp_cond;
public:
  AzureDownloader(const std::string &account_name, const std::string &account_key,
    bool use_https, size_t worker_count, size_t block_size, size_t prefetch_size);
  std::shared_ptr<AzureLocation> init(const std::string &container_name, const std::string &blob_name);
  size_t read(char* buffer, size_t size);
  bool seek(size_t offset);
  bool destroy();
private:
  // worker routines
  void run();
  void push_window(const ReadBlock blk);
};

}
}
}

#endif