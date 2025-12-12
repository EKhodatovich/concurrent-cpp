#include "thread_pool.h"
#include <condition_variable>
#include <memory>
#include <mutex>
#include <utility>

void ThreadPool::worker_loop()
{
    while (true) {
        std::unique_ptr<Task> task;
        {
            std::unique_lock<std::mutex> lock(_task_queue_mutex);
            _task_cv.wait(lock, [this]{ 
                return !_task_queue.empty() || _shutdown.load(); 
            });
            
            if (_shutdown.load() && _task_queue.empty()) {
                break;
            }
            
            if (!_task_queue.empty()) {
                task = std::move(_task_queue.front());
                _task_queue.pop();
                _active_tasks++;
            }
        }
        
        if (task) {
            (*task)();
            _active_tasks--;
            _empty_cv.notify_one();
        }
    }
}

ThreadPool::ThreadPool(int num_of_threads)
{
    for (int i = 0; i < num_of_threads; ++i)
    {
        _threads.push_back(std::thread(
            [this](){worker_loop();}
        ));
    }
}

ThreadPool::~ThreadPool()
{
    _shutdown.store(true);
    _task_cv.notify_all();
    for (auto& t: _threads) {
        if (t.joinable()) {
            t.join();
        }
    }
}

void ThreadPool::submit_task(std::unique_ptr<Task> task)
{
    {
        std::lock_guard<std::mutex> lock(_task_queue_mutex);
        _task_queue.push(std::move(task));
    }
    _task_cv.notify_one();
}

void ThreadPool::wait_until_empty()
{
    std::unique_lock<std::mutex> lock(_task_queue_mutex);
    _empty_cv.wait(lock, [this]{
        return _task_queue.empty() && _active_tasks.load() == 0;
    });
}