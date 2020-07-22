#ifndef _AZURE_SINK_UTIL_CONCURRENT_QUEUE_HPP_
#define _AZURE_SINK_UTIL_CONCURRENT_QUEUE_HPP_

#include <queue>
#include <mutex>
#include <exception>
#include <condition_variable>
#include <iostream>

struct ClosedException: public std::exception
{
  const char *what() const throw()
  {
    return "Queue closed.";
  }
};

// a thread-safe, blocking queue
template <class T>
class BlockingQueue
{
  std::queue<T> q;
  std::mutex lock;
  std::condition_variable cond;
  bool closed;

public:
  BlockingQueue(): q(), lock(), cond(), closed(false) {}
  // front & back is not allowed here, since we're proposing a thread-safe container
  void push(const T& value)
  {
    std::lock_guard<std::mutex> guard(lock);
    if(closed)
      throw ClosedException();
    std::cerr << std::hex << std::this_thread::get_id() << "Pushing new content..." << std::endl;
    q.push(value);
    if(q.size() == 1) {
      log() << "notifying..." << std::endl;
      cond.notify_one();
    }
  }
  void push(T&& value)
  {
    std::lock_guard<std::mutex> guard(lock);
    if(closed)
      throw ClosedException();
    log() << "Pushing new content..." << std::endl;
    q.push(std::move(value));
    if(q.size() == 1) {
      log() << "notifying..." << std::endl;
      cond.notify_one();
    }
  }

  // Pop the first element and return it atomically.
  // If the queue is empty, wait for the first element to be inserted and return it then.
  // If the queue is closed, an exception will be thrown.
  T pop()
  {
    log() << "Entered pop!" << std::endl;
    std::unique_lock<std::mutex> lk(lock);  // locked implicitly here
    log() << "waiting..." << std::endl;
    // wait on condition variable
    cond.wait(lk, [this] { return this->closed || !this->q.empty(); });
    log() << "notified." << std::endl;
    if(closed) {
      log() << "Oh shit it is closed!" << std::endl;
      lk.unlock();
      throw ClosedException();
    }
    log() << "Moving new content..." << std::endl;
    T ret = std::move(q.front());
    q.pop();
    lk.unlock();
    return ret;
  }

  bool empty()
  {
    std::lock_guard<std::mutex> guard(lock);
    return q.empty();
  }
  // close current concurrent queue, return nullptr on all threads waiting on new content.
  void close()
  {
    closed = true;
    cond.notify_all();
  }
  void clear()
  {
    std::lock_guard<std::mutex> guard(lock);
    q = std::queue<T>();
  }
};

#endif