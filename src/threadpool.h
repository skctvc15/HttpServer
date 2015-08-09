/*************************************************************************
	> File Name: threadpool.h
	> Author: skctvc
	> Mail: skctvc15@163.com
	> Created Time: 2014年06月11日 星期三 16时10分29秒
    > 此线程池只适用于服务器这种长时间运行的程序
 ************************************************************************/

#ifndef THREADPOOL_H
#define THREADPOOL_H

#include<list>
#include<assert.h>
#include"threadtools.h"

template <typename T>
class ThreadPool
{
public:
    ThreadPool(int thread_number = 8, int max_requests = 10000);
    ~ThreadPool();

    bool pushToWorkQueue(T* request);

private:
    static void *worker(void *arg);
    void run();
    void errQuit();

    int m_thread_number;        /* 线程池中的线程数 */
    int m_max_requests;         /* 请求队列最大请求数 */
    pthread_t* m_threads;       /* 描述线程池的数组，其大小为m_thread_number */
    std::list<T*> m_workqueue;  /* 请求队列 */
    Mutex m_queuelocker;        /* 请求队列锁 */
    Condition m_queuestat;      /* 是否有线程需要处理指示条件变量 */
    bool m_stop;                /* 是否结束线程 */
};

template <typename T>
ThreadPool <T>::ThreadPool(int thread_number , int max_requests) :
    m_thread_number(thread_number), m_max_requests(max_requests),
    m_threads(NULL) , m_queuelocker(), m_queuestat(m_queuelocker), m_stop(false)
{
    assert(thread_number > 0 && max_requests > 0);

    m_threads = new pthread_t[m_thread_number];

    /*创建thread_number个线程，并将他们都设置为脱离线程*/
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
    delete [] m_threads;
    m_stop = true;
}

template <typename T>
bool ThreadPool<T>::pushToWorkQueue(T* request)
{
    if (m_workqueue.size() > m_max_requests)
    {
        return false;
    }

    MutexGuard Lock(m_queuelocker);
    m_workqueue.push_back(request);
    m_queuestat.notify();

    return true;
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
            MutexGuard Lock(m_queuelocker);
            while (m_workqueue.empty())
            {
                m_queuestat.wait();
            }

            request = m_workqueue.front();
            m_workqueue.pop_front();
        }

        if (!request)
        {
            continue;
        }

        request->process();
    }
}

template<typename T>
void ThreadPool<T>::errQuit()
{
    fprintf(stderr, "Init threadpool failed, quit....\n");
    delete [] m_threads;
    exit(EXIT_FAILURE);
}

#endif
