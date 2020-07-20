#include "blockazureuploader.hpp"

namespace gst {
namespace azure {
namespace storage {

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
  while(!stopped)
  {
    // wait for new content
    if(stream->rdbuf()->in_avail() == 0)
    {
      finish_lock.lock();
      finished = true;
      finish_lock.unlock();
      finish_cond.notify_all();
      // wait for new content while not stopped
      std::unique_lock<std::mutex> lk(stream_lock);
      new_cond.wait(lk);
      lk.unlock();
      if(stopped) break;
    }
    stream_lock.lock();
    auto saved_stream = std::move(stream);
    stream = std::make_unique<std::stringstream>();
    stream_lock.unlock();
    // get content length for debugging
    auto cur = saved_stream->tellg();
    saved_stream->seekg(0, std::ios_base::end);
    auto end = saved_stream->tellg();
    saved_stream->seekg(cur);

    log() << "Uploading content, length = " << static_cast<unsigned int>(end - cur) << std::endl;
    // get new block id
    long long block_id = getBlockId();
    
    auto fut = this->client->append_block_from_stream(loc->first, loc->second, *saved_stream);
    auto result = fut.get();
    handle(result, log());
  }
}


}
}
}