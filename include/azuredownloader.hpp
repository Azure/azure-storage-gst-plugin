#ifndef _AZURE_ELEMENTS_AZURE_DOWNLOADER_HPP_
#define _AZURE_ELEMENTS_AZURE_DOWNLOADER_HPP_

#include <gst/gst.h>
#include "azurecommon.hpp"
#include "utils/blockingqueue.hpp"
#include "storage_credential.h"
#include "storage_account.h"
#include "blob/blob_client.h"

namespace gst
{
  namespace azure
  {
    namespace storage
    {

      class AzureDownloader
      {
        struct ReadRequest
        {
          size_t offset;
          size_t buf_size;
          bool operator>(const ReadRequest &rhs) const
          {
            return offset > rhs.offset;
          }
        };

        struct ReadResponse
        {
          ReadRequest req;
          char *buf;
          bool operator>(const ReadResponse &rhs) const
          {
            return req > rhs.req;
          }
        };

        // configurations
        std::shared_ptr<AzureLocation> loc;
        size_t blob_size;
        unsigned worker_count;
        size_t block_size;
        size_t prefetch_block_count;

        // members
        std::shared_ptr<::azure::storage_lite::blob_client> client;
        BlockingQueue<ReadRequest> reqs;
        std::vector<std::future<void>> workers;
        std::vector<ReadResponse> window;
        // cursor for read() function
        size_t read_cursor;
        // cursor for downloader
        size_t write_cursor;
        std::mutex comp_lock;
        std::condition_variable comp_cond;
        std::mutex read_lock;

      public:
        AzureDownloader(const std::string &account_name, const std::string &account_key,
                        size_t worker_count, size_t block_size, size_t prefetch_block_count,
                        bool use_https, const std::string &blob_endpoint);
        std::shared_ptr<AzureLocation> init(const std::string &container_name, const std::string &blob_name);
        size_t read(char *buffer, size_t size);
        bool seek(size_t offset);
        size_t get_size();
        bool destroy();

      private:
        // worker routines
        void run();
      };

    } // namespace storage
  }   // namespace azure
} // namespace gst

#endif