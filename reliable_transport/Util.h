#pragma once

#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <time.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include "SafeQueue.h"

#define SEGMENT_SIZE 1024
#define PACKET_SIZE (1024 - 8) // UDP header: 8 bytes
#define BLOCK_SIZE (PACKET_SIZE - 16)
#define MAX_BAND (50 * 1024 * 1024 / 8)
#define MAX_DELAY 0.5
#define MAX_BDP int(MAX_BAND * MAX_DELAY)
#define MAX_RTO 60.0

static void ErrorHandler(const char *err)
{
    fprintf(stderr, "%s\n", err);
    exit(0);
}

static double get_current_time()
{
    struct timeval current_t;
    gettimeofday(&current_t, NULL);
    return current_t.tv_sec + 1.0 * current_t.tv_usec / 1000000;
}

static uint32_t rand32()
{
    uint32_t x;
    x = rand() & 0xff;
    x |= (rand() & 0xff) << 8;
    x |= (rand() & 0xff) << 16;
    x |= (rand() & 0xff) << 24;
    return x;
}

static void uint32AddrBytes(uint32_t x, char *buf)
{
    for (int i = 0; i < 4; i++)
        buf[i] = *(((char *)&x) + i);
}

static void uint16AddrBytes(uint16_t x, char *buf)
{
    for (int i = 0; i < 2; i++)
        buf[i] = *(((char *)&x) + i);
}

static void uint32ToBytes(uint32_t x, char *buf)
{
    x = htonl(x);
    uint32AddrBytes(x, buf);
}

static void uint16ToBytes(uint16_t x, char *buf)
{
    x = htons(x);
    uint16AddrBytes(x, buf);
}

struct Segment
{
    uint32_t seqNum, ackNum, rwnd;
    uint16_t len, checksum;
    bool ack, syn, fin;
    char *payload;

    Segment(uint32_t _seqNum, uint32_t _ackNum, uint32_t _rwnd, bool _ack, bool _syn, bool _fin, char *_payload, uint16_t _len)
    {
        seqNum = _seqNum;
        ackNum = _ackNum;
        rwnd = _rwnd;
        ack = _ack;
        syn = _syn;
        fin = _fin;
        payload = _payload;
        len = _len;
    }

    Segment(char *buf, size_t _len)
    {
        seqNum = ntohl(*(uint32_t *)buf);
        ackNum = ntohl(*(uint32_t *)(buf + 4));
        rwnd = ntohl(*(uint32_t *)(buf + 8));
        uint16_t flags = ntohs(*(uint16_t *)(buf + 12));
        ack = ((flags & 4) == 4);
        syn = ((flags & 2) == 2);
        fin = ((flags & 1) == 1);
        checksum = ntohs(*(uint16_t *)(buf + 14));
        payload = buf + 16;
        len = _len - 16;
    }

    static size_t pack(Segment &seg, uint16_t checksum, char *buf, size_t size)
    {
        uint32ToBytes(seg.seqNum, buf);
        uint32ToBytes(seg.ackNum, buf + 4);
        uint32ToBytes(seg.rwnd, buf + 8);
        uint16ToBytes(seg.ack * 4 + seg.syn * 2 + seg.fin, buf + 12);
        uint16AddrBytes(checksum, buf + 14);
        if (seg.payload != NULL && size - 16 >= seg.len)
            memcpy(buf + 16, seg.payload, seg.len);
        return seg.len + 16;
    }

    void Print()
    {
        printf("%u,%u,%u,%u\n", seqNum, ackNum, rwnd, len);
        printf("%u,%u,%u\n", ack, syn, fin);
        for (int i = 0; i < len; i++)
            printf("%d ", payload[i]);
        printf("\n");
    }
};

class Timer
{
    double timestamp;
    bool enable;
    void *(*callback)(void *);
    void *args;

public:
    Timer(double timesec, void *(*_callback)(void *), void *_args)
    {
        timestamp = get_current_time() + timesec;
        callback = _callback;
        args = _args;
        enable = true;
    }

    bool operator<(const Timer &rhs)
    {
        return this->timestamp < rhs.timestamp;
    }

    bool isTrigger(double now) { return timestamp <= now; }
    bool isEnable() { return enable; }
    void cancel() { enable = false; }
    void *run() { return callback(args); }
};

struct Task
{
    char *buf;
    uint16_t len;
    bool fin;

    Task()
    {
        buf = NULL, len = 0, fin = true;
    };

    Task(uint16_t size)
    {
        buf = (char *)malloc(size * sizeof(char)), len = size, fin = false;
    }

    void Free() //do not put in destruction
    {
        if (buf != NULL)
        {
            free(buf);
            buf = NULL;
        }
    }
};
