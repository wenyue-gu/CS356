#include "Reliable.h"

Reliable::Reliable(int hport, int rport)
{
    status = CLOSED;
    bytesInFly = 0;
    rwnd = MAX_BDP;
    cwnd = MAX_BDP;
    pkt = NULL;
    reliImpl = NULL;

    skt = socket(AF_INET, SOCK_DGRAM, 0);

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(hport);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    bind(skt, (struct sockaddr *)&addr, sizeof(addr));

    srvaddr.sin_family = AF_INET;
    srvaddr.sin_port = htons(rport);
    srvaddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    srvlen = sizeof(srvaddr);

    srand((unsigned)time(NULL));
}

Reliable::~Reliable()
{
    Free();
}

void Reliable::setSktTimeout(int timesec)
{
    struct timeval tv;
    tv.tv_sec = timesec;
    tv.tv_usec = 0;
    setsockopt(skt, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv, sizeof(tv));
}

int Reliable::connect(bool nflag, uint32_t n)
{
    setSktTimeout(1);
    pkt = (char *)malloc(sizeof(char) * PACKET_SIZE);

    uint32_t seqNum = nflag ? n : rand32();
    status = SYNSENT;
    int synretry = 0;
    while (status != CONNECTED)
    {
        Segment seg(seqNum, 0, 0, 0, 1, 0, NULL, 0);
        ssize_t len = Segment::pack(seg, 0, pkt, PACKET_SIZE);
        len = Segment::pack(seg, ReliableImpl::checksum(pkt, len), pkt, PACKET_SIZE);
        Sendto(pkt, len);

        len = Recvfrom(pkt, PACKET_SIZE);
        if (len < 0 || ReliableImpl::checksum(pkt, len) != 0)
        {
            if (synretry > 60)
            {
                status = CLOSED;
                return -1;
            }
            synretry += 1;
            continue;
        }

        seg = Segment(pkt, len);
        if (seg.syn && seg.ack && seg.ackNum == (seqNum + 1))
            status = CONNECTED;
    }
    setSktTimeout(0);
    reliImpl = new ReliableImpl(this, seqNum);
    pthread_create(&thHandler, NULL, Reliable::handlerCaller, this);
    return 0;
}

void Reliable::Close()
{
    putTask(Task());               //task.fin=true;
    pthread_join(thHandler, NULL); //return error number if thHanlder is not initialized
    Free();
}

void Reliable::Free()
{
    buffer.lock();
    while (!buffer.Empty())
    {
        Task &tsk = buffer.front();
        tsk.Free();
        buffer.pop();
    }
    buffer.unlock();
    while (!timerHeap.empty())
    {
        Timer *timer = timerHeap.top();
        timerHeap.pop();
        delete timer;
    }
    if (pkt != NULL)
    {
        free(pkt);
        pkt = NULL;
    }
    if (reliImpl != NULL)
    {
        delete reliImpl;
        reliImpl = NULL;
    }
}

void Reliable::handler()
{
    fd_set inputs, outputs;
    while (status != CLOSED)
    {
        FD_ZERO(&inputs);
        FD_SET(this->skt, &inputs);
        FD_ZERO(&outputs);
        FD_SET(this->skt, &outputs);
        select(this->skt + 1, &inputs, &outputs, NULL, NULL);

        if (FD_ISSET(this->skt, &inputs))
        {
            ssize_t len = Recvfrom(pkt, PACKET_SIZE);
            if (len <= 0 || ReliableImpl::checksum(pkt, len) != 0)
                goto outputLabel;
            Segment seg(pkt, len);
            if (status == CONNECTED)
            {
                if (seg.ack && !seg.syn && !seg.fin)
                    this->bytesInFly += reliImpl->recvAck(&seg, false);
            }
            else if (status == FINWAIT)
            {
                if (seg.ack && !seg.syn && !seg.fin)
                    this->bytesInFly += reliImpl->recvAck(&seg, false);
                else if (seg.ack && seg.fin)
                {
                    this->bytesInFly += reliImpl->recvAck(&seg, true);
                    status = CLOSED;
                }
            }
        }

    outputLabel:
        if (FD_ISSET(this->skt, &outputs))
        {
            if (status != CONNECTED || bytesInFly >= std::min(rwnd, cwnd))
                goto TimerLabel;
            Task block = getTask();
            if (block.fin)
            {
                this->bytesInFly += reliImpl->sendData(NULL, 0, true);
                status = FINWAIT;
            }
            else
                this->bytesInFly += reliImpl->sendData(block.buf, block.len, false);
            block.Free();
        }

    TimerLabel:
        double now = get_current_time();
        while (!timerHeap.empty())
        {
            Timer *timer = timerHeap.top();
            if (!timer->isTrigger(now))
                break;
            timerHeap.pop();
            if (timer->isEnable())
                timer->run();
            delete timer;
        }
    }
}
