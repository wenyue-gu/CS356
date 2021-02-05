#pragma once

#include <cstdlib>
#include <queue>
#include <errno.h>
#include <pthread.h>
#include <sys/time.h>

template <typename T>
class SafeQueue
{
    std::queue<T> queue;
    pthread_mutex_t mtx;
    pthread_cond_t empty;

public:
    SafeQueue()
    {
        pthread_mutex_init(&mtx, NULL);
        pthread_cond_init(&empty, NULL);
    };

    ~SafeQueue()
    {
        pthread_mutex_lock(&mtx);
        while (!queue.empty())
            queue.pop();
        pthread_mutex_unlock(&mtx);
    }

    void lock()
    {
        pthread_mutex_lock(&mtx);
    }

    void unlock()
    {
        pthread_mutex_unlock(&mtx);
    }

    T &front()
    {
        return queue.front();
    }

    void pop()
    {
        queue.pop();
    }

    bool Empty()
    {
        return queue.empty();
    }

    size_t size()
    {
        return queue.size();
    }

    T get(int timeout = 0)
    {
        T res;
        pthread_mutex_lock(&mtx);
        if (timeout > 0)
        {
            struct timespec outtime;
            struct timeval now;
            gettimeofday(&now, NULL);
            outtime.tv_sec = now.tv_sec + timeout;
            outtime.tv_nsec = now.tv_usec * 1000;

            int err = 0;
            while (queue.size() == 0)
            {
                err = pthread_cond_timedwait(&empty, &mtx, &outtime);
                if (err == ETIMEDOUT)
                {
                    pthread_mutex_unlock(&mtx);
                    return res;
                }
            }
        }
        else
        {
            while (queue.size() == 0)
                pthread_cond_wait(&empty, &mtx);
        }
        res = queue.front();
        queue.pop();
        pthread_mutex_unlock(&mtx);
        return res;
    }

    int put(const T &e)
    {
        pthread_mutex_lock(&mtx);
        queue.push(e);
        pthread_cond_signal(&empty);
        pthread_mutex_unlock(&mtx);
        return 0;
    }
};