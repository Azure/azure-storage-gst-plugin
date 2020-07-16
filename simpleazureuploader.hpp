#ifndef _SIMPLE_AZURE_UPLOADER_CPP_H_
#define _SIMPLE_AZURE_UPLOADER_CPP_H_

#include <map>
#include <utility>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include "utils.h"

#include "storage_credential.h"
#include "storage_account.h"
#include "blob/blob_client.h"
#include "blob/append_block_request.h"

namespace gst {
namespace azure {
namespace storage {

typedef std::pair<std::string, std::string> AzureUploadLocation;
typedef std::pair<const char *, size_t> UploadBuffer;
class UploadWorker {
  std::shared_ptr<AzureUploadLocation> loc;
  std::mutex new_lock, finish_lock;
  std::condition_variable new_cond, finish_cond;
  std::stringstream stream;
  bool finished;
  bool stopped;
  std::shared_ptr<::azure::storage_lite::blob_client> client;
  std::thread worker;
public:
  UploadWorker(std::shared_ptr<AzureUploadLocation> loc,
    std::shared_ptr<::azure::storage_lite::blob_client> client):
    loc(loc), client(client), worker([this] { this->run(); }) {}
  bool append(UploadBuffer buffer);
  void run();
  void flush();
  void stop();
private:
  std::ostream &log();
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