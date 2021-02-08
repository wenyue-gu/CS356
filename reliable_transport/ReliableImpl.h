#pragma once

#include "Util.h"

typedef struct ReliableImpl ReliableImpl;
#include "Reliable.h"

struct ReliableImpl
{
    Reliable *reli;
    uint32_t seqNum;

    // Variables for maintaining sliding window
};

ReliableImpl *reliImplCreate(Reliable *_reli, uint32_t _seqNum);
void reliImplClose(ReliableImpl *reliImpl);
uint16_t reliImplChecksum(const char *buf, ssize_t len);
int32_t reliImplRecvAck(ReliableImpl *reliImpl, const Segment *seg, bool isFin);
int32_t reliImplSendData(ReliableImpl *reliImpl, char *block, uint16_t blocklen, bool isFin);
void *reliImplRetransmission(void *args);
