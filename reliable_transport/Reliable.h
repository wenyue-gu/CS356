#pragma once

#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/select.h>
#include <signal.h>
#include "Util.h"

typedef struct Reliable Reliable;
#include "ReliableImpl.h"

#define SYNSENT 0
#define CONNECTED 1
#define FINWAIT 2
#define CLOSED 3

typedef struct Task
{
    char *buf;
    uint16_t len;
    bool fin;
} Task;

Task *taskCreate(uint16_t size, bool fin);
void taskClose(Task *tsk);

struct Reliable
{
    int skt;
    struct sockaddr_in srvaddr;
    socklen_t srvlen;

    short status;
    uint32_t bytesInFly, rwnd, cwnd;
    SafeQueue buffer;
    Heap timerHeap;

    char *pkt;
    ReliableImpl *reliImpl;
    pthread_t thHandler;
};

Reliable *reliCreate(int hport, int rport);
void reliClose(Reliable *reli);
int reliConnect(Reliable *reli, bool nflag, uint32_t n);
void *reliGetTask(Reliable *reli);
int reliSend(Reliable *reli, Task *block);

ssize_t reliRecvfrom(Reliable *reli, char *pkt, size_t size);

// Followings are APIs that you may need to use in ReliableImpl
// Sendto: Send a well-formed segment ('pkt') to the destination.
// 'pkt' is an array of char bytes. It should not contain UDP header.
// 'len' is he length of 'seg'.
ssize_t reliSendto(Reliable *reli, const char *pkt, const size_t len);

// updateRWND: Update the receive window size.
//'rwnd' means the bytes of the receive window.
uint32_t reliUpdateRWND(Reliable *reli, uint32_t _rwnd);

// setTimer: Set a timer. We implement our own Timer in this lab (See Util.h).
// The function 'callback' will be called with 'args' as arguments
// after 'timesec' seconds.
Timer *reliSetTimer(Reliable *reli, double timesec, void *(*callback)(void *), void *args);