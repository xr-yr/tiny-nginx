#include "ngx_c_threadpool.h"

#include <unistd.h>

namespace nginx {

    // 静态成员初始化
    pthread_mutex_t CThreadPool::m_pthreadMutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t CThreadPool::m_pthreadCond = PTHREAD_COND_INITIALIZER;
    std::atomic<bool> CThreadPool::m_shutDown = false;

    CThreadPool::CThreadPool() {
        m_runThreadCount = 0;
        m_lastEmgTime = 0;
        m_recvMsgQueueCount = 0;
    }

    CThreadPool::~CThreadPool() {
        if (!m_shutDown) {
            StopAll();
        }
        // 清除消息队列
        ClearMsgRecvQueue();
    }

    bool CThreadPool::Create(int threadNum) {
        int32_t err = 0;
        ThreadItem *pThreadItem;
        m_threadCap = threadNum;

        for (int i = 0; i < threadNum; ++i) {
            m_threadPool.push_back(pThreadItem = new ThreadItem(this));
            err = pthread_create(&pThreadItem->m_thread, nullptr,
                                 ThreadFunc, pThreadItem);
            if (err != 0) {
                // 创建线程错误
                return false;
            } else {
                // log
            }
        }
        // 必须保证每个线程都启动并运行到pthread_cond_wait()，本函数才返回，线程才能进行后续的正常工作
        for (auto &iter: m_threadPool) {
            if (iter->isRunning) {
                continue;
            }
            // 等待 100ms     100 个 1000us
            usleep(100 * 1000);
        }
        return true;
    }

    void CThreadPool::StopAll() {
        if (m_shutDown) {
            return;
        }
        Pthread_mutex_lock();
        // 销毁标志设为 true，原子变量，可以不加锁
        m_shutDown = true;
        // 唤醒等待条件的所有线程，一定要在改变条件状态以后再给线程发信号
        Pthread_cond_broadcast();
        Pthread_mutex_unlock();
        // 等待线程，释放线程 ThreadItem
        for (auto &iter: m_threadPool) {
            pthread_join(iter->m_thread, nullptr);
            delete iter;
        }
        m_threadPool.clear();
        return;
    }

    void CThreadPool::AddMsgRecvQueue(char *buf) {
        Pthread_mutex_lock();
        // 添加消息
        m_msgRecvQueue.push_back(buf);
        ++m_recvMsgQueueCount;

        Pthread_mutex_unlock();
        // 通知线程工作
        Call();
        return;
    }

    void CThreadPool::Call() {
        // 唤醒一个等待条件的线程，及被 pthread_cond_wait 阻塞的线程
        Pthread_cond_signal();

        if (m_threadCap == m_runThreadCount) {
            time_t currTime = time(nullptr);
            if (currTime - m_lastEmgTime > 10) {
                // 更新时间，打印无可用线程
                m_lastEmgTime = currTime;
                // todo
            }
        }
        return;
    }

    void CThreadPool::Pthread_mutex_lock() {
        int32_t err = pthread_mutex_lock(&m_pthreadMutex);
        if (err != 0) {
            // todo
        }
        return;
    }

    void CThreadPool::Pthread_mutex_unlock() {
        int32_t err = pthread_mutex_unlock(&m_pthreadMutex);
        if (err != 0) {
            // todo
        }
        return;
    }

    void CThreadPool::Pthread_cond_signal() {
        int32_t err = pthread_cond_signal(&m_pthreadCond);
        if (err != 0) {
            // todo
        }
    }

    void CThreadPool::Pthread_cond_wait() {
        int32_t err = pthread_cond_wait(&m_pthreadCond, &m_pthreadMutex);
        if (err != 0) {
            // todo
        }
    }

    void CThreadPool::Pthread_cond_broadcast() {
        int32_t err = pthread_cond_broadcast(&m_pthreadCond);
        if (err != 0) {
            // todo
        }
    }

    void *CThreadPool::ThreadFunc(void *arg) {
        return nullptr;
    }

    void CThreadPool::ClearMsgRecvQueue() {

    }

} // namespace nginx
