#ifndef _AZURE_ELEMENTS_BLOCK_AZURE_UPLOADER_HPP_
#define _AZURE_ELEMENTS_BLOCK_AZURE_UPLOADER_HPP_

#include <map>
#include <utility>
#include <memory>
#include <thread>
#include <mutex>
#include <sstream>
#include <condition_variable>

#include "azurecommon.hpp"
#include "utils/common.hpp"
#include "utils/blockingqueue.hpp"

#include "storage_credential.h"
#include "storage_account.h"
#include "blob/blob_client.h"
#include "blob/append_block_request.h"

namespace gst
{
  namespace azure
  {
    namespace storage
    {

      const unsigned int BLOCK_SIZE = 1048576 * 2;
      const unsigned int WORKER_COUNT = 4;
      class BlockAzureUploader
      {
      private:
        typedef long long unsigned blockid_t;
        struct UploadJob
        {
          blockid_t id;
          std::unique_ptr<std::stringstream> stream;
        };
        struct UploadResponse
        {
          enum
          {
            OK,
            FAIL
          } code;
          blockid_t id;
        };
        // configurations
        unsigned block_size;
        unsigned worker_count;
        unsigned commit_block_count;
        unsigned commit_interval_ms;

        // members
        std::shared_ptr<::azure::storage_lite::blob_client> client;
        std::shared_ptr<AzureLocation> loc;
        std::unique_ptr<std::stringstream> stream;

        // worker and comitter
        std::vector<std::future<void>> workers;
        std::future<void> commitWorker;

        // message channels
        BlockingQueue<UploadJob> reqs;
        BlockingQueue<UploadResponse> resps;
        blockid_t nextCommitId;
        blockid_t nextId;
        blockid_t committedId;
        std::vector<blockid_t> window;
        std::chrono::_V2::steady_clock::time_point lastCommit;
        std::vector<::azure::storage_lite::put_block_list_request_base::block_item>
            block_list;

        // flush blocker
        std::mutex flush_lock;
        bool flushing;
        unsigned sem;
        std::condition_variable flush_cond;
        std::condition_variable complete_cond;
        void enterJob()
        {
          std::unique_lock<std::mutex> lk(flush_lock);
          flush_cond.wait(lk, [this] { return !this->flushing; });
          sem--;
        }
        void leaveJob()
        {
          std::unique_lock<std::mutex> lk(flush_lock);
          sem++;
          // log() << "sem=" << sem << std::endl;
          complete_cond.notify_one();
        }
        // wait for worker threads to complete current upload and start waiting
        void waitFlush()
        {
          std::unique_lock<std::mutex> lk(flush_lock);
          flushing = true;
          complete_cond.wait(lk, [this] { return this->sem == this->worker_count; });
        }
        // disable flush so that worker threads can continue to work
        void disableFlush()
        {
          std::lock_guard<std::mutex> guard(flush_lock);
          flushing = false;
          flush_cond.notify_all();
        }

      public:
        BlockAzureUploader(
            const std::string &account_name, const std::string &account_key,
            unsigned long block_size, unsigned long worker_count,
            unsigned long commit_block_count, unsigned long commit_interval_ms,
            bool use_https, const std::string &blob_endpoint);
        std::shared_ptr<AzureLocation> init(const char *container_name, const char *blob_name);
        bool upload(std::shared_ptr<AzureLocation> loc, const char *data, size_t size);
        bool flush(std::shared_ptr<AzureLocation> loc);
        bool destroy(std::shared_ptr<AzureLocation> loc);

      private:
        void waitCommit();
        void doFlush();
        void commitBlock(blockid_t id);
        void doCommit();
        bool checkLoc(std::shared_ptr<AzureLocation> loc);
        void run();
        void runCommit();
      };

    } // namespace storage
  }   // namespace azure
} // namespace gst

#endif