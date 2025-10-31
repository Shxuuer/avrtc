#ifndef BASE_THREAD_H
#define BASE_THREAD_H

#include <glog/logging.h>
#include <pthread.h>

#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <vector>

namespace avrtc {

class Thread {
  static const int kForever = -1;

 public:
  Thread();
  Thread(std::shared_ptr<void> thread_local_data);
  ~Thread();
  void Join();
  void AddTask(const std::function<void()>& task);
  void Stop();

 private:
  static void* PreRun(void* pv);
  void Run();
  pthread_t thread_;

  std::mutex mutex_;
  std::condition_variable cond_var_;
  std::vector<std::function<void()>> tasks_;

  bool running_ = true;
  std::shared_ptr<void> thread_local_data_;
};

class ThreadManager {
 public:
  static ThreadManager* Instance();
  Thread* CurrentThread();
  void SetCurrentThread(Thread* thread);
  void Add(Thread* thread);

 private:
  ThreadManager();
  ~ThreadManager();

  std::vector<Thread*> threads_;
  pthread_key_t thread_key_;
  std::mutex mutex_;
};

}  // namespace avrtc

#endif  // BASE_THREAD_H