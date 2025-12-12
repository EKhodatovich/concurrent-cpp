#pragma once

#include <memory>
#include <mutex>
#include <queue>
#include <vector>
#include <thread>
#include <condition_variable>
#include <atomic>

#include "task.h"

class ThreadPool {
public:
    ThreadPool(int num_of_threads);
    ~ThreadPool();
    void submit_task(std::unique_ptr<Task> task);
    void wait_until_empty();  // Wait until all tasks are processed
    
private:
    void worker_loop();

    std::vector<std::thread> _threads;
    std::queue<std::unique_ptr<Task>> _task_queue;
    std::mutex _task_queue_mutex;
    std::condition_variable _task_cv;
    std::condition_variable _empty_cv;
    std::atomic<bool> _shutdown{false};
    std::atomic<int> _active_tasks{0};
};

