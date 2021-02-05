#pragma once

#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/select.h>
#include <signal.h>
#include <queue>
#include "Util.h"

class Reliable;
#include "ReliableImpl.h"

#define SYNSENT 0
#define CONNECTED 1
#define FINWAIT 2
#define CLOSED 3

class Reliable
{
    int skt;
    sockaddr_in srvaddr;
    socklen_t srvlen;

    short status;
    uint32_t bytesInFly, rwnd, cwnd;
    SafeQueue<Task> buffer;

    char *pkt;
    ReliableImpl *reliImpl;
    pthread_t thHandler;

    class Timercmp
    {
    public:
        bool operator()(Timer *lhs, Timer *rhs)
        {
            return (*rhs) < (*lhs);
        }
    };
    std::priority_queue<Timer *, std::vector<Timer *>, Timercmp> timerHeap;

    void setSktTimeout(int timesec);

public:
    Reliable(int hport = 10000, int rport = 50001);
    ~Reliable();

    int connect();
    void Close();
    void Free();
    void handler();
    static void *handlerCaller(void *argv)
    {
        ((Reliable *)argv)->handler();
        return NULL;
    };

    Task getTask() { return buffer.get(); } // block if queue is empty
    size_t putTask(const Task &block) { return buffer.put(block); }
    size_t send(const Task &block) { return putTask(block); }

    ssize_t Recvfrom(char *pkt, size_t size)
    {
        return recvfrom(skt, pkt, size, 0, (struct sockaddr *)&srvaddr, &srvlen);
    }

    // Followings are APIs that you may need to use in ReliableImpl
    // Sendto: Send a well-formed segment ('pkt') to the destination.
    // 'pkt' is an array of char bytes. It should not contain UDP header.
    // 'len' is he length of 'seg'.
    ssize_t Sendto(const char *pkt, const size_t len)
    {
        return sendto(skt, pkt, len, 0, (struct sockaddr *)&srvaddr, srvlen);
    }

    // updateRWND: Update the receive window size.
    //'rwnd' means the bytes of the receive window.
    uint32_t updateRWND(uint32_t _rwnd)
    {
        rwnd = _rwnd;
        return rwnd;
    }

    // setTimer: Set a timer. We implement our own Timer in this lab (See Util.h).
    // The function 'callback' will be called with 'args' as arguments
    // after 'timesec' seconds.
    Timer *setTimer(double timesec, void *(*callback)(void *), void *args)
    {
        Timer *timer = new Timer(timesec, callback, args); //delete in handler
        timerHeap.push(timer);
        return timer;
    }
};
