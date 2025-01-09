#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <cassert>
#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <unordered_map>

namespace HG {
class ThreadPool
{
public:
    ThreadPool()
        : ThreadPool(std::thread::hardware_concurrency())
    {
    }

    // explicit 
    ThreadPool(size_t nMaxThreads)
        : m_bQuit(false),
            m_nCurrentThreads(0),
            m_nIdleThreads(0),
            m_nMaxThreads(nMaxThreads)
    {
    }

    // disable the copy operations
    // ThreadPool(const ThreadPool &) = delete;
    // ThreadPool &operator=(const ThreadPool &) = delete;

    ~ThreadPool()
    {
        {
            std::lock_guard<std::mutex> guard(m_mutex);
            m_bQuit = true;
        }
        m_condition.notify_all();

        for (auto &elem : m_mapThreads)
        {
            assert(elem.second.joinable());
            elem.second.join();
        }
    }

    template <typename Func, typename... Ts>
    auto submit(Func &&func, Ts &&...params)
        -> std::future<typename std::result_of<Func(Ts...)>::type>
    {
        auto execute = std::bind(std::forward<Func>(func), std::forward<Ts>(params)...);

        using ReturnType = typename std::result_of<Func(Ts...)>::type;
        using PackagedTask = std::packaged_task<ReturnType()>;

        auto task = std::make_shared<PackagedTask>(std::move(execute));
        auto result = task->get_future();

        std::lock_guard<std::mutex> guard(m_mutex);
        assert(!m_bQuit);

        m_quTasks.emplace([task]()
                        { (*task)(); });
        if (m_nIdleThreads > 0)
        {
            m_condition.notify_one();
        }
        else if (m_nCurrentThreads < m_nMaxThreads)
        {
            std::thread t(&ThreadPool::worker, this);
            assert(m_mapThreads.find(t.get_id()) == m_mapThreads.end());
            m_mapThreads[t.get_id()] = std::move(t);
            ++m_nCurrentThreads;
        }

        return result;
    }


    size_t threadsNum() const
    {
        std::lock_guard<std::mutex> guard(m_mutex);
        return m_nCurrentThreads;
    }

private:
    void worker()
    {
        while (true)
        {
            std::function<void()> task;
            {
                std::unique_lock<std::mutex> uniqueLock(m_mutex);
                ++m_nIdleThreads;
                auto hasTimedout = !m_condition.wait_for(uniqueLock,
                                                    std::chrono::seconds(WAIT_SECONDS),
                                                    [this]()
                                                    {
                                                        return m_bQuit || !m_quTasks.empty();
                                                    });
                --m_nIdleThreads;
                if (m_quTasks.empty())
                {
                    if (m_bQuit)
                    {
                        --m_nCurrentThreads;
                        return;
                    }
                    if (hasTimedout)
                    {
                        --m_nCurrentThreads;
                        joinFinishedThreads();
                        m_quFinishedThreadIDs.emplace(std::this_thread::get_id());
                        return;
                    }
                }
                task = std::move(m_quTasks.front());
                m_quTasks.pop();
            }
            task();
        }
    }

    void joinFinishedThreads()
    {
        while (!m_quFinishedThreadIDs.empty())
        {
            auto id = std::move(m_quFinishedThreadIDs.front());
            m_quFinishedThreadIDs.pop();
            auto iter = m_mapThreads.find(id);

            assert(iter != m_mapThreads.end());
            assert(iter->second.joinable());

            iter->second.join();
            m_mapThreads.erase(iter);
        }
    }

    static constexpr size_t WAIT_SECONDS = 2;

    bool m_bQuit = false;
    size_t m_nCurrentThreads;
    size_t m_nIdleThreads;
    size_t m_nMaxThreads;

    mutable std::mutex m_mutex;
    std::condition_variable m_condition;
    std::queue<std::function<void()>> m_quTasks;
    std::queue<std::thread::id> m_quFinishedThreadIDs;
    std::unordered_map<std::thread::id, std::thread> m_mapThreads;
};

constexpr size_t ThreadPool::WAIT_SECONDS;
}

#endif /* THREADPOOL_H */