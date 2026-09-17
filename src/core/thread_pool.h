#ifndef RENDERER_SRC_CORE_THREAD_POOL_H_
#define RENDERER_SRC_CORE_THREAD_POOL_H_

#include <condition_variable>
#include <exception>
#include <functional>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <thread>
#include <utility>
#include <vector>

class ThreadPool {
 public:
  explicit ThreadPool(int num_threads) {
    if (num_threads <= 0) {
      throw std::invalid_argument("ThreadPool requires at least one worker");
    }
    workers_.reserve(num_threads);
    try {
      for (int i = 0; i < num_threads; ++i) {
        workers_.emplace_back([this] { WorkerLoop(); });
      }
    } catch (...) {
      Shutdown();
      throw;
    }
  }

  ~ThreadPool() { Shutdown(); }

  ThreadPool(const ThreadPool&) = delete;
  ThreadPool& operator=(const ThreadPool&) = delete;

  void Submit(std::function<void()> task) {
    if (!task) {
      throw std::invalid_argument("Cannot submit an empty task");
    }
    {
      std::lock_guard<std::mutex> lock(mutex_);
      tasks_.push(std::move(task));
      ++pending_;
    }
    cv_.notify_one();
  }

  void Wait() {
    std::unique_lock<std::mutex> lock(mutex_);
    done_cv_.wait(lock, [this] { return pending_ == 0; });
    auto failure = std::exchange(error_, nullptr);
    lock.unlock();
    if (failure) {
      std::rethrow_exception(failure);
    }
  }

  int Size() const { return static_cast<int>(workers_.size()); }

 private:
  void Shutdown() noexcept {
    {
      std::lock_guard lock(mutex_);
      stop_ = true;
    }
    cv_.notify_all();
    for (auto& worker : workers_) {
      worker.join();
    }
  }

  void WorkerLoop() {
    while (true) {
      std::function<void()> task;
      {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [this] { return stop_ || !tasks_.empty(); });
        if (stop_ && tasks_.empty()) {
          return;
        }
        task = std::move(tasks_.front());
        tasks_.pop();
      }

      try {
        task();
      } catch (...) {
        std::lock_guard lock(mutex_);
        if (!error_) {
          error_ = std::current_exception();
        }
      }

      {
        std::lock_guard<std::mutex> lock(mutex_);
        if (--pending_ == 0) {
          done_cv_.notify_all();
        }
      }
    }
  }

  std::vector<std::thread> workers_;
  std::queue<std::function<void()>> tasks_;
  std::mutex mutex_;
  std::condition_variable cv_;
  std::condition_variable done_cv_;
  size_t pending_ = 0;
  std::exception_ptr error_;
  bool stop_ = false;
};

#endif  // RENDERER_SRC_CORE_THREAD_POOL_H_
