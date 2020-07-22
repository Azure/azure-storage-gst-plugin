#ifndef _AZURE_SINK_UTIL_CONCURRENT_QUEUE_HPP_
#define _AZURE_SINK_UTIL_CONCURRENT_QUEUE_HPP_

#include <queue>
#include <mutex>
#include <exception>
#include <condition_variable>

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
    q.push(value);
    if(q.size() == 1)
      cond.notify_one();
  }
  void push(T&& value)
  {
    std::lock_guard<std::mutex> guard(lock);
    if(closed)
      throw ClosedException();
    q.push(std::move(value));
    if(q.size() == 1)
      cond.notify_one();
  }

  // Pop the first element and return it atomically.
  // If the queue is empty, wait for the first element to be inserted and return it then.
  // If the queue is closed, an exception will be thrown.
  T &pop()
  {
    std::unique_lock<std::mutex> lk(lock);
    lk.lock();
    if(q.empty())
      // wait on condition variable
      cond.wait(lk, [this] { return !this->q.empty(); });
    if(closed)
      throw ClosedException();
    T &ret = q.front();
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