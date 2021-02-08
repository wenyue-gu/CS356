#include "Reliable.h"

Task *taskCreate(uint16_t size, bool fin)
{
    Task *tsk = (Task *)malloc(sizeof(Task));
    tsk->buf = (char *)malloc(sizeof(char) * size);
    tsk->fin = fin;
    return tsk;
}

void taskClose(Task *tsk)
{
    Free(tsk->buf);
    Free(tsk);
}

void *reliGetTask(Reliable *reli)
{
    return queueGet(&reli->buffer, 0);
}

int reliSend(Reliable *reli, Task *block)
{
    return queuePut(&reli->buffer, block, 0);
}

ssize_t reliRecvfrom(Reliable *reli, char *pkt, size_t size)
{
    return recvfrom(reli->skt, pkt, size, 0, (struct sockaddr *)&(reli->srvaddr), &(reli->srvlen));
}

ssize_t reliSendto(Reliable *reli, const char *pkt, const size_t len)
{
    return sendto(reli->skt, pkt, len, 0, (struct sockaddr *)&reli->srvaddr, reli->srvlen);
}

uint32_t reliUpdateRWND(Reliable *reli, uint32_t _rwnd)
{
    reli->rwnd = _rwnd;
    return reli->rwnd;
}

Timer *reliSetTimer(Reliable *reli, double timesec, void *(*callback)(void *), void *args)
{
    Timer *timer = (Timer *)malloc(sizeof(Timer));
    timerInit(timer, timesec, callback, args);
    heapPush(&reli->timerHeap, timer);
    return timer;
}

static void setSktTimeout(int skt, int timesec)
{
    struct timeval tv;
    tv.tv_sec = timesec;
    tv.tv_usec = 0;
    setsockopt(skt, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv, sizeof(tv));
}

static void *reliHandler(void *args);

Reliable *reliCreate(int hport, int rport)
{
    Reliable *reli = (Reliable *)malloc(sizeof(Reliable));
    reli->status = CLOSED;
    reli->bytesInFly = 0;
    reli->rwnd = MAX_BDP;
    reli->cwnd = MAX_BDP;
    reli->pkt = NULL;
    reli->reliImpl = NULL;
    queueInit(&reli->buffer, 10 * MAX_BDP / BLOCK_SIZE);
    heapInit(&reli->timerHeap, timerCmp);

    reli->skt = socket(AF_INET, SOCK_DGRAM, 0);

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(hport);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    bind(reli->skt, (struct sockaddr *)&addr, sizeof(addr));

    reli->srvaddr.sin_family = AF_INET;
    reli->srvaddr.sin_port = htons(rport);
    reli->srvaddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    reli->srvlen = sizeof(reli->srvaddr);

    srand((unsigned)time(NULL));
    return reli;
}

void reliClose(Reliable *reli)
{
    Task *fin = taskCreate(0, true);
    reliSend(reli, fin);                 //task.fin=true;
    pthread_join(reli->thHandler, NULL); //return error number if thHanlder is not initialized

    while (reli->buffer.count > 0)
    {
        Task *tsk = queueFront(&reli->buffer);
        queuePop(&reli->buffer);
        Free(tsk->buf);
    }
    queueClear(&reli->buffer);

    while (reli->timerHeap.count > 0)
    {
        Timer *timer = heapTop(&reli->timerHeap);
        heapPop(&reli->timerHeap);
        Free(timer);
    }
    heapClear(&reli->timerHeap);

    reliImplClose(reli->reliImpl);
    Free(reli->pkt);
    Free(reli);
}

int reliConnect(Reliable *reli, bool nflag, uint32_t n)
{
    setSktTimeout(reli->skt, 1);

    reli->pkt = (char *)malloc(sizeof(char) * PACKET_SIZE);

    uint32_t seqNum = nflag ? n : rand32();
    reli->status = SYNSENT;
    int synretry = 0;
    while (reli->status != CONNECTED)
    {
        Segment seg = {seqNum, 0, 0, 0, 1, 0, NULL, 0};
        ssize_t len = segPack(&seg, 0, reli->pkt, PACKET_SIZE);
        len = segPack(&seg, reliImplChecksum(reli->pkt, len), reli->pkt, PACKET_SIZE);
        reliSendto(reli, reli->pkt, len);

        len = reliRecvfrom(reli, reli->pkt, PACKET_SIZE);
        if (len < 0 || reliImplChecksum(reli->pkt, len) != 0)
        {
            if (synretry > 60)
            {
                reli->status = CLOSED;
                return -1;
            }
            synretry += 1;
            continue;
        }

        segParse(&seg, reli->pkt, len);
        if (seg.syn && seg.ack && seg.ackNum == (seqNum + 1))
            reli->status = CONNECTED;
    }
    setSktTimeout(reli->skt, 0);
    reli->reliImpl = reliImplCreate(reli, seqNum);
    pthread_create(&(reli->thHandler), NULL, reliHandler, reli);
    return 0;
}

static void *reliHandler(void *args)
{
    Reliable *reli = (Reliable *)args;
    fd_set inputs, outputs;
    while (reli->status != CLOSED)
    {
        FD_ZERO(&inputs);
        FD_SET(reli->skt, &inputs);
        FD_ZERO(&outputs);
        FD_SET(reli->skt, &outputs);
        select(reli->skt + 1, &inputs, &outputs, NULL, NULL);

        if (FD_ISSET(reli->skt, &inputs))
        {
            ssize_t len = reliRecvfrom(reli, reli->pkt, PACKET_SIZE);
            if (len <= 0 || reliImplChecksum(reli->pkt, len) != 0)
                goto outputLabel;
            Segment seg;
            segParse(&seg, reli->pkt, len);
            if (reli->status == CONNECTED)
            {
                if (seg.ack && !seg.syn && !seg.fin)
                    reli->bytesInFly += reliImplRecvAck(reli->reliImpl, &seg, false);
            }
            else if (reli->status == FINWAIT)
            {
                if (seg.ack && !seg.syn && !seg.fin)
                    reli->bytesInFly += reliImplRecvAck(reli->reliImpl, &seg, false);
                else if (seg.ack && seg.fin)
                {
                    reli->bytesInFly += reliImplRecvAck(reli->reliImpl, &seg, true);
                    reli->status = CLOSED;
                }
            }
        }

    outputLabel:;
        if (FD_ISSET(reli->skt, &outputs))
        {
            if (reli->status != CONNECTED || reli->bytesInFly >= MIN(reli->rwnd, reli->cwnd))
                goto timerLabel;
            Task *block = reliGetTask(reli);
            if (block->fin)
            {
                reli->bytesInFly += reliImplSendData(reli->reliImpl, NULL, 0, true);
                reli->status = FINWAIT;
            }
            else
                reli->bytesInFly += reliImplSendData(reli->reliImpl, block->buf, block->len, false);
            usleep(1); // Avoid sending too fast and overflowing UDP buffer at the receiver
            taskClose(block);
        }

    timerLabel:;
        double now = get_current_time();
        while (reli->timerHeap.count > 0)
        {
            Timer *timer = heapTop(&reli->timerHeap);
            if (now < timer->timestamp)
                break;
            if (timer->enable)
                timerRun(timer);

            heapPop(&reli->timerHeap);
            Free(timer);
        }
    }
    return NULL;
}
