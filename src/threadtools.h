#ifndef _LOCKER_H
#define _LOCKER_H
#include <pthread.h>
#include <semaphore.h>
#include <boost/utility.hpp>

class Mutex : boost::noncopyable{
    friend class Condition;
public:
    Mutex() {
        pthread_mutex_init(&m_mutex,NULL);
    }

    ~Mutex() {
        pthread_mutex_destroy(&m_mutex);
    }

    bool lock(){
        return pthread_mutex_lock(&m_mutex) == 0;
    }

    bool unlock() {
        return pthread_mutex_unlock(&m_mutex) == 0;
    }

private:
    pthread_mutex_t m_mutex;
};

class MutexGuard : boost::noncopyable{
public:
    explicit MutexGuard(Mutex & mutex):m_mutex(mutex) {
        m_mutex.lock();
    }

    ~MutexGuard() {
        m_mutex.unlock();
    }

private:
    Mutex& m_mutex;
};

#define MutexGuard(x) static_assert(false,"missing mutex guard variable name")

class Condition : boost::noncopyable{
public:
    Condition(Mutex& mutex) : m_mutex(mutex) {
        pthread_cond_init(&m_cond,NULL);
    }

    ~Condition() {
        pthread_cond_destroy(&m_cond);
    }

    bool wait() {
        return  pthread_cond_wait(&m_cond,&m_mutex.m_mutex) == 0;
    }

    bool notify() {
        return pthread_cond_signal(&m_cond) == 0;
    }

    bool notifyAll() {
        return pthread_cond_broadcast(&m_cond) == 0;
    }

private:
    pthread_cond_t m_cond;
    Mutex& m_mutex;
};

class Sem : boost::noncopyable{
public:
    Sem() {
        sem_init(&m_sem,0,0);
    }

    ~Sem() {
        sem_destroy(&m_sem);
    }

    bool wait() {
        return sem_wait(&m_sem) == 0;
    }

    bool post() {
        return sem_post(&m_sem) == 0;
    }

private:
    sem_t m_sem;
};

#endif

