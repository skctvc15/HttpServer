/*************************************************************************
	> File Name: threadpool.h
	> Author: skctvc
	> Mail: skctvc15@163.com
	> Created Time: 2014年06月11日 星期三 16时10分29秒
    > 此线程池只适用于服务器这种长时间运行的程序
 ************************************************************************/

#ifndef THREADPOOL_H
#define THREADPOOL_H

#include<queue>
#include<assert.h>
#include"threadtools.h"

template <typename T>
class ThreadPool
{
public:
    ThreadPool(int thread_number = 8, int max_requests = 10000);
    ~ThreadPool();

    void pushToWorkQueue(T* request);
    void stop();

private:
    static void *worker(void *arg);
    void run();
    void errQuit();

    int m_thread_number;        /* 线程池中的线程数 */
    int m_max_requests;         /* 请求队列最大请求数 */
    pthread_t* m_threads;       /* 描述线程池的数组，其大小为m_thread_number */
    std::queue<T*> m_workqueue;  /* 请求队列 */
    Mutex m_queuelocker;        /* 请求队列锁 */
    Condition m_not_empty;      /* 队列非空条件变量 */
    Condition m_not_full;      /* 队列未满条件变量 */
    bool m_stop;                /* 是否结束线程 */
};

template <typename T>
ThreadPool <T>::ThreadPool(int thread_number , int max_requests) :
    m_thread_number(thread_number), m_max_requests(max_requests),
    m_threads(NULL) , m_queuelocker(), m_not_empty(m_queuelocker), m_not_full(m_queuelocker), m_stop(false)
{
    assert(thread_number > 0 && max_requests > 0);

    m_threads = new pthread_t[m_thread_number]();

    for (int i = 0; i < m_thread_number; i++)
    {
        if (pthread_create(m_threads + i, NULL, worker, this) != 0)
        {
            errQuit();
        }

        if (pthread_detach(m_threads[i]))
        {
            errQuit();
        }
    }
}

template <typename T>
ThreadPool<T>::~ThreadPool()
{
    stop();
}

template <typename T>
void ThreadPool<T>::pushToWorkQueue(T* request)
{
    while (m_workqueue.size() > m_max_requests)
    {
        m_not_full.wait();
    }

    MutexGuard lock(m_queuelocker);
    m_workqueue.push(request);
    m_not_empty.notify();
}

template<typename T>
void* ThreadPool<T>::worker(void *arg)
{
    ThreadPool* pool = static_cast<ThreadPool*>(arg);
    pool->run();
    return pool;
}

template<typename T>
void ThreadPool<T>::run()
{
    while (!m_stop)
    {
        T* request = NULL;
        {
            MutexGuard lock(m_queuelocker);
            while (m_workqueue.empty())
            {
                m_not_empty.wait();
            }

            request = m_workqueue.front();
            m_workqueue.pop();
            m_not_full.notify();
        }

        if (!request)
        {
            continue;
        }

        request->process();
    }
}

template<typename T>
void ThreadPool<T>::stop()
{
    m_stop = true;
    delete []m_threads;
}

template<typename T>
void ThreadPool<T>::errQuit()
{
    fprintf(stderr, "Init threadpool failed, quit....\n");
    delete [] m_threads;
    exit(EXIT_FAILURE);
}

#endif
