#include "blockazureuploader.hpp"
#include "util/base64.hpp"
#include "util/utils.hpp"

namespace gst {
namespace azure {
namespace storage {

std::atomic_llong BlockUploadWorker::counter;
inline long long int BlockUploadWorker::getBlockId()
{
  return BlockUploadWorker::counter.fetch_add(1, std::memory_order_relaxed);
}

std::ostream &BlockUploadWorker::log()
{
  return std::cerr << '[' << std::hex << std::this_thread::get_id << ']';
}

void BlockUploadWorker::run()
{
  std::unique_ptr<std::stringstream> s = nullptr;
  while(!stopped.load())
  {
    // wait for new content
    if(stream->rdbuf()->in_avail() == 0)
    {
      finished.store(true);
      finish_cond.notify_all();
      // wait for new content while not stopped
      std::unique_lock<std::mutex> lk(stream_lock);
      stream_cond.wait(lk);
      if(stopped) break;
      s = std::move(stream);
      stream = std::make_unique<std::stringstream>();
    }

    auto len = getStreamLen(*s);
    log() << "Uploading content, length = " << len << std::endl;
    // get new block id in base64 format
    std::string b64_block_id = base64_encode(std::to_string(getBlockId()));
    log() << "Got new block id " << b64_block_id << std::endl;
    auto metadata = std::vector<std::pair<std::string, std::string>>();
    metadata.emplace_back("blockid", b64_block_id);
    // keep trying until success
    
    auto fut = client->upload_block_blob_from_stream(loc->first, loc->second, *s, metadata);
    auto result = fut.get();
    handle(result, log());
  }
}


}
}
}