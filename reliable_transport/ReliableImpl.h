#pragma once

#include "Util.h"

class ReliableImpl;
#include "Reliable.h"

class ReliableImpl
{
    Reliable *reli;
    uint32_t seqNum;

    //TODO: Your code here

public:
    ReliableImpl(Reliable *_reli, uint32_t _seqNum);
    ~ReliableImpl();

    static uint16_t checksum(const char *buf, ssize_t len);
    uint32_t recvAck(const Segment *seg, bool isFin);
    uint32_t sendData(char *block, uint16_t len, bool isFin);
    static void *retransmission(void *args);
};

//TODO: You can define additional class/struct here
