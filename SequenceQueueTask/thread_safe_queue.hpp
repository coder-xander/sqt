#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>
template<typename T>
class ThreadSafeQueue {
public:
    ThreadSafeQueue() = default;
    ThreadSafeQueue(const ThreadSafeQueue& other) {
        std::lock_guard<std::mutex> lock(other.m_mutex);
        m_queue = other.m_queue;
    }

    ThreadSafeQueue& operator=(const ThreadSafeQueue&) = delete;

    void push(T value) {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_condition.wait(lock, [this] { return m_queue.size() < maxSize; }); // 等待队列未满
        m_queue.push(value);
        lock.unlock();
        m_condition.notify_one(); // 通知其他线程
    }
    void wait_and_pop(T& value) {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_condition.wait(lock, [this] { return !m_queue.empty(); }); // 等待队列非空
        value = m_queue.front();
        m_queue.pop();
        lock.unlock();
        m_condition.notify_one(); // 通知其他线程
    }

    bool empty() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_queue.empty();
    }

    size_t size() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_queue.size();
    }


private:
    size_t maxSize{ 300000 }; // 最大容量
    mutable std::mutex m_mutex;
    std::queue<T> m_queue;
    std::condition_variable m_condition;
};


