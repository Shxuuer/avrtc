#include "base/thread.h"

namespace avrtc {

Thread::Thread() {
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_create(&thread_, &attr, Thread::PreRun, this);
}

Thread::Thread(std::shared_ptr<void> thread_local_data) : Thread() {
    thread_local_data_ = thread_local_data;
}

Thread::~Thread() {
    pthread_detach(thread_);
}

void Thread::Stop() {
    running_ = false;
}

void Thread::Join() {
    pthread_join(thread_, nullptr);
}

void* Thread::PreRun(void* pv) {
    Thread* thread = static_cast<Thread*>(pv);
    thread->Run();
    return nullptr;
}

void Thread::Run() {
    ThreadManager::Instance()->Add(this);
    ThreadManager::Instance()->SetCurrentThread(this);

    while (running_) {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            cond_var_.wait(lock, [this]() { return !tasks_.empty(); });
            task = tasks_.front();
            tasks_.erase(tasks_.begin());
        }
        if (task) {
            task();
        }
    }
}

void Thread::AddTask(const std::function<void()>& task) {
    std::lock_guard<std::mutex> lock(mutex_);
    tasks_.push_back(task);
    cond_var_.notify_one();
}

ThreadManager::ThreadManager() {
    pthread_key_create(&thread_key_, nullptr);
}

ThreadManager::~ThreadManager() {
    pthread_key_delete(thread_key_);
}

void ThreadManager::Add(Thread* thread) {
    std::lock_guard<std::mutex> lock(mutex_);
    threads_.push_back(thread);
}

ThreadManager* ThreadManager::Instance() {
    static ThreadManager* const instance = new ThreadManager();
    return instance;
}

Thread* ThreadManager::CurrentThread() {
    Thread* current_thread =
        static_cast<Thread*>(pthread_getspecific(thread_key_));
    CHECK(current_thread != nullptr);
    return current_thread;
}

void ThreadManager::SetCurrentThread(Thread* thread) {
    pthread_setspecific(thread_key_, thread);
}

}  // namespace avrtc