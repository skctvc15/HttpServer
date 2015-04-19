/*************************************************************************
	> File Name: threadpool.h
	> Author: skctvc
	> Mail: skctvc15@163.com 
	> Created Time: 2014年06月11日 星期三 16时10分29秒
 ************************************************************************/

#ifndef THREADPOOL_H
#define THREADPOOL_H

#include<list>
#include<cstdio>
#include<stdexcept>
#include<pthread.h>
#include"threadtools.h"

/*线程池类，将其定义为模板，T是任务类,任务类须定义process函数来执行具体任务*/
template < typename T >
class threadpool
{
public:
    /* 参数1是线程池中线程的数量，参数2是请求队列中最多允许的、等待处理的请求的数量 */
    threadpool(int thread_number = 8, int max_requests = 10000);
    ~threadpool();

    /* 往请求队列中添加任务*/
    bool append(T* request);

private:
    /* 工作线程运行的函数，它不断从工作队列中取出任务并执行之 */
    static void *worker( void *arg );
    void run();
private:
    int m_thread_number;        /* 线程池中的线程数 */
    int m_max_requests;         /* 请求队列最大请求数 */
    pthread_t* m_threads;       /* 描述线程池的数组，其大小为m_thread_number */
    std::list< T* > m_workqueue;/* 请求队列 */
    Mutex m_queuelocker;        /* 请求队列锁 */ 
    Condition m_queuestat;      /* 是否有线程需要处理指示条件变量 */
    bool m_stop;                /* 是否结束线程 */
};

template < typename T >
threadpool < T >::threadpool( int thread_number , int max_requests ) :
    m_thread_number( thread_number ),m_max_requests(max_requests),m_threads( NULL ),m_stop( false )
    ,m_queuelocker(),m_queuestat(m_queuelocker)
{
    if(( thread_number <= 0 ) || max_requests <= 0 )
    {
        throw std::invalid_argument("thread_number or max_requests must > 0");
    }
    
    m_threads = new pthread_t[ m_thread_number ];

    /*创建thread_number个线程，并将他们都设置为脱离线程*/
    for( int i = 0; i < thread_number; i++ )
    {
        if( pthread_create( m_threads + i,NULL, worker, this ) != 0 )    //C++中通过静态函数调用类的动态成员这里通过传递this指针的方式
        {
            delete [] m_threads;
            throw std::exception();
        }
        if( pthread_detach( m_threads[i] ) )
        {
            delete [] m_threads;
            throw std::exception();
        }
    }
}

template < typename T >
threadpool< T >::~threadpool()
{
    delete [] m_threads;
    m_stop = true;
}
 
template < typename T >
bool threadpool< T >::append(T * request)
{
    if( m_workqueue.size() > m_max_requests )
    {
        return false;
    }

    {
        MutexGuard Lock(m_queuelocker);
        m_workqueue.push_back( request );
        m_queuestat.notify();
    }
    return true;
}

template< typename T >
void* threadpool< T >::worker( void *arg )
{
    threadpool* pool = (threadpool *)arg;
    pool->run();
    return pool;
}

template< typename T >
void threadpool< T >::run()
{
    while( !m_stop )
    {

        T* request = NULL;
        {
            MutexGuard Lock(m_queuelocker);
            while( m_workqueue.empty() )
            {
                m_queuestat.wait();
            }
            request = m_workqueue.front();
            m_workqueue.pop_front();
            
        }

        if( !request )
        {
            continue;
        }
        request->start();
    }
}

#endif
