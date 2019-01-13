#ifndef SAFE_QUEUE
#define SAFE_QUEUE

#include <queue>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <future>

template <class T>
class SafeQueue
{
    public:
        SafeQueue(void) : 
            m_vQueue(), 
            m_mutex()
        {}

        ~SafeQueue(void)
        {}

        void clear()
        {
            m_vQueue.clear();
        }

        void push(T t)
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_vQueue.push_back(t);
        }

        void pushMany(std::vector<T> vT)
        {
            std::lock_guard<std::mutex> lock(m_mutex);

            for (size_t i = 0; i < vT.size(); ++i)
                m_vQueue.push_back(vT[i]);
        }
        
        // Parameter expected to be empty.
        bool popAll(std::vector<T>& result)
        {
            std::lock_guard<std::mutex> lock(m_mutex);

            if (m_vQueue.empty())
                return false;

            result = m_vQueue;
            m_vQueue.clear();
            return true;
        }

    private:
        std::mutex m_mutex;
        std::vector<T> m_vQueue;
};

#endif