#ifndef THREADQUEUE_H
#define THREADQUEUE_H

#pragma once
#include <deque>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <utility>

template <typename T>
class ThreadQueue
{
public:
    explicit ThreadQueue(size_t maxSize = 8)
        : m_maxSize(maxSize)
    {}

    void clear()
    {
        std::lock_guard<std::mutex> lk(m_mtx);
        m_q.clear();
    }

    void notifyAll()
    {
        m_cvNotEmpty.notify_all();
        m_cvNotFull.notify_all();
    }

    bool push(T item, std::atomic_bool& stopFlag)
    {
        std::unique_lock<std::mutex> lk(m_mtx);
        m_cvNotFull.wait(lk, [&]{
            return stopFlag.load() || m_q.size() < m_maxSize;
        });

        if (stopFlag.load())
            return false;

        m_q.emplace_back(std::move(item));
        lk.unlock();
        m_cvNotEmpty.notify_one();
        return true;
    }

    bool pop(T& out, std::atomic_bool& stopFlag)
    {
        std::unique_lock<std::mutex> lk(m_mtx);
        m_cvNotEmpty.wait(lk, [&]{
            return stopFlag.load() || !m_q.empty();
        });

        if (m_q.empty())
            return false;

        out = std::move(m_q.front());
        m_q.pop_front();

        lk.unlock();
        m_cvNotFull.notify_one();
        return true;
    }

private:
    std::mutex m_mtx;
    std::condition_variable m_cvNotEmpty;
    std::condition_variable m_cvNotFull;
    std::deque<T> m_q;
    size_t m_maxSize = 8;
};


#endif // THREADQUEUE_H
