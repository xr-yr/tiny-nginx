#ifndef NGX_C_THREADPOOL_H
#define NGX_C_THREADPOOL_H

#include <pthread.h>
#include <stdint.h>
#include <atomic>
#include <vector>
#include <list>

namespace nginx {

    class CThreadPool {
    public:
        CThreadPool();

        ~CThreadPool();

        // 禁止拷贝构造
        CThreadPool(const CThreadPool &other) = delete;

        // 禁止拷贝赋值
        CThreadPool &operator=(const CThreadPool &other) = delete;

        // 禁止移动构造
        CThreadPool(const CThreadPool &&other) = delete;

        // 禁用移动赋值
        CThreadPool &operator=(const CThreadPool &&other) = delete;

        // 创建线程池
        bool Create(int threadNum);

        // 释放线程池
        void StopAll();

        // 添加消息
        void AddMsgRecvQueue(char *buf);

        // 通知线程工作
        void Call();

        // 获取消息队列大小
        uint32_t GetRecvMsgQueueCount() {
            return m_recvMsgQueueCount;
        }

        // 加锁
        void Pthread_mutex_lock();

        // 解锁
        void Pthread_mutex_unlock();

        //
        void Pthread_cond_signal();

        void Pthread_cond_wait();

        void Pthread_cond_broadcast();

    private:
        // 线程回调函数
        static void *ThreadFunc(void *arg);

        // 清理消息队列
        void ClearMsgRecvQueue();

    private:
        struct ThreadItem {
            pthread_t m_thread;         // 线程ID
            CThreadPool *m_pThis;       // 线程池指针
            bool isRunning;             // 线程状态
            // 构造函数
            ThreadItem(CThreadPool *pool) : m_pThis(pool) {}

            // 析构函数
            ~ThreadItem() { isRunning = false; }

            // 禁止拷贝构造
            ThreadItem(const ThreadItem &other) = delete;

            // 禁止拷贝赋值
            ThreadItem &operator=(const ThreadItem &other) = delete;

            // 禁止移动构造
            ThreadItem(const ThreadItem &&other) = delete;

            // 禁用移动赋值
            ThreadItem &operator=(const ThreadItem &&other) = delete;
        };

    private:
        static pthread_mutex_t m_pthreadMutex;      // 线程同步互斥锁
        static pthread_cond_t m_pthreadCond;        // 线程同步条件变量
        static std::atomic<bool> m_shutDown;        // 线程池释放标志，true:释放

        uint32_t m_threadCap;                       // 线程容量
        std::atomic<uint32_t> m_runThreadCount;     // 运行中线程数量
        time_t m_lastEmgTime;                       // 线程不够用时间

        std::vector<ThreadItem *> m_threadPool;     // 线程池
        std::list<char *> m_msgRecvQueue;           // 接收数据消息队列
        uint32_t m_recvMsgQueueCount;               // 消息队列大小
    };

} // namespace nginx

#endif // NGX_C_THREADPOOL_H
