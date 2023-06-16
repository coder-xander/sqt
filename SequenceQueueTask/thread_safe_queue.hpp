#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>
#include <stdexcept>

template<typename T>
class ThreadSafeQueue {
public:
    ThreadSafeQueue() = default;

    // ʹ��std::lock_guard�����������캯��
    ThreadSafeQueue(const ThreadSafeQueue& other) {
        std::lock_guard<std::mutex> lock(other.m_mutex);
        m_queue = other.m_queue;
    }

    // ɾ��������ֵ�����
    ThreadSafeQueue& operator=(const ThreadSafeQueue&) = delete;

    // ����ƶ����캯��
    ThreadSafeQueue(ThreadSafeQueue&& other) noexcept {
        std::lock_guard<std::mutex> lock(other.m_mutex);
        m_queue = std::move(other.m_queue);
    }

    // ����ƶ���ֵ�����
    ThreadSafeQueue& operator=(ThreadSafeQueue&& other) noexcept {
        if (this != &other) {
            std::lock(m_mutex, other.m_mutex);
            std::lock_guard<std::mutex> lock1(m_mutex, std::adopt_lock);
            std::lock_guard<std::mutex> lock2(other.m_mutex, std::adopt_lock);
            m_queue = std::move(other.m_queue);
        }
        return *this;
    }

    // ��Ӵ���������������Ĺ��캯��
    explicit ThreadSafeQueue(size_t max_size) : maxSize(max_size) {}

    void push(T value) {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_condition_not_full.wait(lock, [this] { return m_queue.size() < maxSize; });
        m_queue.push(std::move(value));
        lock.unlock();
        m_condition_not_empty.notify_one();
    }

    void wait_and_pop(T& value) {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_condition_not_empty.wait(lock, [this] { return !m_queue.empty(); });
        value = std::move(m_queue.front());
        m_queue.pop();
        lock.unlock();
        m_condition_not_full.notify_one();
    }

    // �Ľ�Ϊ��������pop����
    bool try_pop(T& value) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_queue.empty()) {
            return false;
        }
        value = std::move(m_queue.front());
        m_queue.pop();
        m_condition_not_full.notify_one();
        return true;
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
    size_t maxSize{ 500000 };
    mutable std::mutex m_mutex;
    std::queue<T> m_queue;
    std::condition_variable m_condition_not_full;
    std::condition_variable m_condition_not_empty;
};

