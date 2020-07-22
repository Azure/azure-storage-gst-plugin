#include "blockazureuploader.hpp"
#include "utils/base64.hpp"
#include "utils/common.hpp"
#include <chrono>
#include <algorithm>

using namespace std::chrono_literals;
namespace gst {
namespace azure {
namespace storage {

BlockAzureUploader::BlockAzureUploader(const char *account_name, const char *account_key, bool use_https)
{
  std::string name(account_name);
  std::string key(account_key);
  auto credential = std::make_shared<::azure::storage_lite::shared_key_credential>(name, key);
  auto storage_account = std::make_shared<::azure::storage_lite::storage_account>(name, credential, use_https);
  client = std::make_shared<::azure::storage_lite::blob_client>(storage_account, WORKER_COUNT);
}

// Check if the location is correct.
// BlockAzureUploader only support one location at a time,
// and cannot be modified once specified
bool BlockAzureUploader::checkLoc(std::shared_ptr<AzureUploadLocation> loc)
{
  return this->loc != nullptr && this->loc == loc;
}

std::shared_ptr<AzureUploadLocation> BlockAzureUploader::init(const char *container_name, const char *blob_name)
{
  if(loc != nullptr && destroy(loc) == false)
    log() << "Warning: failed to destroy previous workers." << std::endl;
  loc = std::make_shared<AzureUploadLocation>(std::string(container_name), std::string(blob_name));
  // create the blob first
  auto ss = std::stringstream();
  auto result = client->upload_block_from_stream(loc->first, loc->second, base64_encode("0"), ss);
  for(unsigned i = 0; i < WORKER_COUNT; i++)
    workers[i] = std::async(&BlockAzureUploader::run, this);
  stream = std::make_unique<std::stringstream>();
  window_start = 0;
  blockId = 1;  // block id starts from one, first block is zero-size
  commitWorker = std::async(&BlockAzureUploader::runCommit, this);
  return loc;
}

bool BlockAzureUploader::upload(std::shared_ptr<AzureUploadLocation> loc, const char *data, size_t size)
{
  if(!checkLoc(loc))
    return false;
  // Blocks are uploaded only if buffer(1MiB by default) overflows
  stream->write(data, size);
  if(getStreamLen(*stream) > BLOCK_SIZE)
  {
    reqs.push(UploadJob{blockId++, move(stream)});
    stream = std::make_unique<std::stringstream>();
  }
  return true;
}

bool BlockAzureUploader::flush(std::shared_ptr<AzureUploadLocation> loc)
{
  if(!checkLoc(loc))
    return false;
  log() << "Flushing..." << std::endl;
  // commit remaining data
  if(getStreamLen(*stream) > 0) {
    log() << "Pushing uncommitted data..." << std::endl;
    reqs.push(UploadJob{ blockId++, std::move(stream)});
  }
  std::this_thread::sleep_for(500ms);
  for(auto &fut: workers)
    fut.wait();
  return true;
}


bool BlockAzureUploader::destroy(std::shared_ptr<AzureUploadLocation> loc)
{
  if(!checkLoc(loc))
    return false;
  reqs.close();
  // wait for all current workers to finish their jobs
  for(auto &fut: workers)
    fut.wait();
  resps.close();
  if(commitWorker.valid())
    commitWorker.wait();
  return true;
}

// multithreaded worker job
void BlockAzureUploader::run()
{
  log() << "Initialized new upload worker." << std::endl;
  while(1)
  {
    // wait for new content
    UploadJob job;
    try {
      job = std::move(reqs.pop());
    } catch (ClosedException e) {
      log() << e.what() << std::endl;
      break;
    }
    auto len = getStreamLen(*stream);
    // get new block id in base64 format
    std::string b64_block_id = base64_encode(std::to_string(job.id));
    log() << "Uploading content, length = " << len << " id = " << b64_block_id << std::endl;
    // keep trying until success
    do {
      auto fut = client->upload_block_from_stream(loc->first, loc->second, b64_block_id, *stream);
      auto result = fut.get();
      handle(result, log());
      if(result.success())
        break;
      log() << "Retrying " << b64_block_id << "..." << std::endl;
    } while(1);
    // success, send back response
    resps.push(UploadResponse{UploadResponse::OK, job.id});
  }
  log() << "Worker is exiting." << std::endl;
}

void BlockAzureUploader::runCommit()
{
  using ::azure::storage_lite::put_block_list_request_base;
  log() << "Initalizing committer." << std::endl;
  while(1)
  {
    // fetch all responses already enqueued
    try {
      UploadResponse resp;
      do {
        resp = resps.pop();
        if(resp.code == UploadResponse::OK)
        {
          window.push_back(resp.id);
          std::push_heap(window.begin(), window.end());
          log() << "Committer received response of id " << base64_encode(std::to_string(resp.id)) << std::endl;
        }
      } while(!resps.empty());
    } catch (ClosedException e) {
      break;
    }
    blockid_t new_start = window_start;
    while(new_start == window.front())
    {
      std::pop_heap(window.begin(), window.end());
      new_start += 1;
      window.pop_back();
    }
    if(new_start > window_start)
    {
      log() << "Pushing window start to " << new_start << std::endl;
      // do commit
      std::vector<put_block_list_request_base::block_item> block_list;
      for(blockid_t id = window_start; id < new_start; id++)
        block_list.push_back(put_block_list_request_base::block_item{
          base64_encode(std::to_string(id)), 
          put_block_list_request_base::block_type::uncommitted
        });
      // currently no metadata
      auto metadata = std::vector<std::pair<std::string, std::string>>();
      auto fut = client->put_block_list(loc->first, loc->second, block_list, metadata);
      auto result = fut.get();
      handle(result);
      if(result.success()) {
        window_start = new_start;
        log() << "Committed, now window start is " << window_start << std::endl;
      }
    }
  }
  log() << "Comitter is exiting." << std::endl;
}

}
}
}