#ifndef AZURE_SINK_UPLOADER_H
#define AZURE_SINK_UPLAODER_H

#include <map>
#include <utility>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>

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
  std::thread worker;
  std::mutex new_lock, finish_lock;
  std::condition_variable new_cond, finish_cond;
  std::stringstream stream;
  bool finished;
  bool stopped;
  std::shared_ptr<::azure::storage_lite::blob_client> client;
public:
  UploadWorker(std::shared_ptr<AzureUploadLocation> loc,
    std::shared_ptr<::azure::storage_lite::blob_client> client):
    loc(loc), client(client), worker([this] { this->run(); }) {}
  bool append(UploadBuffer buffer);
  void run();
  void flush();
  void stop();
};

const int AZURE_CLIENT_CONCCURRENCY = 8;

class AzureUploader {
private:
  std::shared_ptr<::azure::storage_lite::blob_client> client;
  std::map<std::shared_ptr<AzureUploadLocation>, std::unique_ptr<UploadWorker>> uploads;
public:
  AzureUploader(const char *account_name, const char *account_key, bool use_https);
  std::shared_ptr<AzureUploadLocation> init(const char *container_name, const char *blob_name);
  bool upload(std::shared_ptr<AzureUploadLocation> loc, const char *data, size_t size);
  bool flush(std::shared_ptr<AzureUploadLocation> loc);
  bool destroy(std::shared_ptr<AzureUploadLocation> loc);
};
}
}
}
#endif