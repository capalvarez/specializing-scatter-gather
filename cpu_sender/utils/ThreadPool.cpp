#include "ThreadPool.h"


ThreadPool::ThreadPool(size_t threads)
        : stop(false) {
    for (size_t i = 0; i < threads; ++i)
        workers.emplace_back(
                [this] {
                    for (;;) {
                        std::function<void()> task;

                        {
                            std::unique_lock<std::mutex> lock(this->queue_mutex);
                            this->condition.wait(lock,
                                                 [this] { return this->stop || !this->tasks.empty(); });
                            if (this->stop && this->tasks.empty())
                                return;
                            task = std::move(this->tasks.front());
                            this->tasks.pop();
                        }

                        task();
                    }
                }
        );
}

// add new work item to the pool

void ThreadPool::wait_for_finish() {
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        stop = true;
    }

    condition.notify_all();
    for (std::thread &worker: workers)
        worker.join();
}

void ThreadPool::set_affinity() {
    long number_of_processors = sysconf(_SC_NPROCESSORS_ONLN);

    for (size_t i = 0; i < workers.size(); ++i) {
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(i % (number_of_processors-1), &cpuset);
        pthread_setaffinity_np(workers[i].native_handle(),
                               sizeof(cpu_set_t), &cpuset);

    }

}