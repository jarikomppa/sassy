#ifndef CONCURRENT_THREADPOOL_H
#define CONCURRENT_THREADPOOL_H

#include <thread>
#include <mutex>
#include <functional>
#include <condition_variable>
#include <queue>
#include <vector>

/**
 *  Simple ThreadPool that creates `threadCount` threads upon its creation,
 *  and pulls from a queue to get new jobs.
 *
 *  This class requires a number of c++11 features be present in your compiler.
 */
class ThreadPool final
{
public:
    explicit ThreadPool(int threadCount) :
        _jobsLeft(0),
        _isRunning(true)
    {
        _threads.reserve(threadCount);
        for (int index = 0; index < threadCount; ++index)
        {
            _threads.emplace_back([&]
                {
                    /**
                    *  Take the next job in the queue and run it.
                    *  Notify the main thread that a job has completed.
                    */
                    do
                    {
                        std::function<void()> job;

                        // scoped lock
                        {
                            std::unique_lock<std::mutex> lock(_queueMutex);

                            // Wait for a job if we don't have any.
                            _jobsQueuedCondition.wait(lock, [&]
                                {
                                    return !_queue.empty();
                                });

                            // Get job from the queue
                            job = _queue.front();
                            _queue.pop();
                        }

                        job();

                        // scoped lock
                        {
                            std::lock_guard<std::mutex> lock(_jobsLeftMutex);
                            --_jobsLeft;
                        }

                        _jobsDoneCondition.notify_one();
                    } while (_isRunning);
                });
        }
    }

    /**
     *  JoinAll on deconstruction
     */
    ~ThreadPool()
    {
        JoinAll();
    }

    /**
     *  Add a new job to the pool. If there are no jobs in the queue,
     *  a thread is woken up to take the job. If all threads are busy,
     *  the job is added to the end of the queue.
     */
    void AddJob(const std::function<void()>& job)
    {
        // scoped lock
        {
            std::lock_guard<std::mutex> lock(_queueMutex);
            _queue.push(job);
        }
        // scoped lock
        {
            std::lock_guard<std::mutex> lock(_jobsLeftMutex);
            ++_jobsLeft;
        }
        _jobsQueuedCondition.notify_one();
    }

    /**
     *  Join with all threads. Block until all threads have completed.
     *  The queue may be filled after this call, but the threads will
     *  be done. After invoking `ThreadPool::JoinAll`, the pool can no
     *  longer be used.
     */
    void JoinAll()
    {
        if (_isRunning)
        {
            _isRunning = false;

            // add empty jobs to wake up threads
            const int threadCount = _threads.size();
            for (int index = 0; index < threadCount; ++index)
            {
                AddJob([] {});
            }

            // note that we're done, and wake up any thread that's
            // waiting for a new job
            _jobsQueuedCondition.notify_all();

            for (std::thread& thread : _threads)
            {
                if (thread.joinable())
                {
                    thread.join();
                }
            }
        }
    }

    /**
     *  Wait for the pool to empty before continuing.
     *  This does not call `std::thread::join`, it only waits until
     *  all jobs have finished executing.
     */
    void WaitAll()
    {
        std::unique_lock<std::mutex> lock(_jobsLeftMutex);
        _jobsDoneCondition.wait(lock, [&]
            {
                return _jobsLeft == 0;
            });
    }

    /**
     *  Get the vector of threads themselves, in order to set the
     *  affinity, or anything else you might want to do
     */
    std::vector<std::thread>& GetThreads()
    {
        return _threads;
    }

    /**
     *  Process the next job in the queue to run it in the calling thread
     */
    bool ExecuteNextJob()
    {
        std::function<void()> job;

        // scoped lock
        {
            std::lock_guard<std::mutex> lock(_queueMutex);

            if (_queue.empty())
            {
                return false;
            }

            // Get job from the queue
            job = _queue.front();
            _queue.pop();
        }

        job();

        // scoped lock
        {
            std::lock_guard<std::mutex> lock(_jobsLeftMutex);
            --_jobsLeft;
        }

        _jobsDoneCondition.notify_one();

        return true;
    }

private:
    std::vector<std::thread> _threads;
    std::queue<std::function<void()>> _queue;

    int _jobsLeft;
    bool _isRunning;
    std::condition_variable _jobsQueuedCondition;
    std::condition_variable _jobsDoneCondition;
    std::mutex _jobsLeftMutex;
    std::mutex _queueMutex;
};

#endif //CONCURRENT_THREADPOOL_H
