#ifndef _AZURE_ELEMENTS_UTIL_BLOCKING_QUEUE_HPP_
#define _AZURE_ELEMENTS_UTIL_BLOCKING_QUEUE_HPP_

#include <queue>
#include <mutex>
#include <exception>
#include <condition_variable>
#include <iostream>
#include <chrono>

struct ClosedException: public std::exception
{
  const char *what() const throw()
  {
    return "Queue closed.";
  }
};

struct TimeoutException: public std::exception
{
  const char *what() const throw()
  {
    return "Timeout exceeded.";
  }
};

// a thread-safe, blocking queue
template <class T>
class BlockingQueue
{
  std::queue<T> q;
  std::mutex lock;
  std::condition_variable cond;
  std::condition_variable emp;
  bool closed;

public:
  BlockingQueue(): q(), lock(), cond(), closed(false) {}
  // front & back are not allowed here, since we're proposing a thread-safe container
  void push(const T& value)
  {
    std::lock_guard<std::mutex> guard(lock);
    if(closed)
      throw ClosedException();
    q.push(value);
    cond.notify_one();
  }
  void push(T&& value)
  {
    std::lock_guard<std::mutex> guard(lock);
    if(closed)
      throw ClosedException();
    q.push(std::move(value));
    cond.notify_one();
  }

  // Pop the first element and return it atomically.
  // If the queue is empty, wait for the first element to be inserted and return it then.
  // If the queue is closed, a closed exception will be thrown.
  T pop()
  {
    std::unique_lock<std::mutex> lk(lock);
    // wait on condition variable
    cond.wait(lk, [this] { return this->closed || !this->q.empty(); });
    if(closed) {
      throw ClosedException();
    }
    T ret = std::move(q.front());
    q.pop();
    if(q.empty())
      emp.notify_all();
    return ret;
  }

  // Pop the first element and return it atomically.
  // Other than the functionalities of pop, a timeout duration can be specified.
  // If the timeout exceeds and there's still no element, a timeout exception is thrown.
  template <class Rep, class Period>
  T pop_for(const std::chrono::duration<Rep, Period> &timeout)
  {
    std::unique_lock<std::mutex> lk(lock);  // locked implicitly here
    // wait on condition variable
    cond.wait_for(lk, timeout, [this] { return this->closed || !this->q.empty(); });
    if(closed) {
      throw ClosedException();
    } else if(q.empty()) {
      throw TimeoutException();
    }
    T ret = std::move(q.front());
    q.pop();
    if(q.empty())
      emp.notify_all();
    return ret;
  }
  
  // non-blockingly return if the queue is empty
  bool empty()
  {
    std::lock_guard<std::mutex> guard(lock);
    return q.empty();
  }

  // wait for a queue to become empty
  void wait_empty()
  {
    std::unique_lock<std::mutex> lk(lock);
    emp.wait(lk, [this] { return this->q.empty(); });
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