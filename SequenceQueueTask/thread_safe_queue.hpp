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
        m_condition.wait(lock, [this] { return m_queue.size() < maxSize; }); // �ȴ�����δ��
        m_queue.push(value);
        lock.unlock();
        m_condition.notify_one(); // ֪ͨ�����߳�
    }
    void wait_and_pop(T& value) {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_condition.wait(lock, [this] { return !m_queue.empty(); }); // �ȴ����зǿ�
        value = m_queue.front();
        m_queue.pop();
        lock.unlock();
        m_condition.notify_one(); // ֪ͨ�����߳�
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
    size_t maxSize{ 300000 }; // �������
    mutable std::mutex m_mutex;
    std::queue<T> m_queue;
    std::condition_variable m_condition;
};


