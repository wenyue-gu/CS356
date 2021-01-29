import time
import heapq
import select
import socket
import random
import threading
from ReliableImpl import ReliableImpl
from queue import Queue
from collections import deque
from Util import *


SYNSENT = 0
CONNECTED = 1
FINWAIT = 2
CLOSED = 3


class Reliable:
    def __init__(self, hport=10000, rport=50001):
        self.__dst = ('127.0.0.1', rport)
        self.__buf = Queue(maxsize=MaxBufSize)
        self.__skt = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.__skt.bind(('', hport))
        self.__skt.settimeout(1)

        self.status = CLOSED
        self.bytesInFly = 0
        self.rwnd = MaxBDP
        # self.cwnd = 3*BlockSize
        self.cwnd = MaxBDP

        self.timerHeap = []
        self.persistTimer = None
        self.handlerThread = None
        self.reliImpl = None

    def __del__(self):
        self.__skt.close()

    def getskt(self):
        return self.__skt

    def connect(self):
        seqNum = random.randint(0, (1 << 32)-1)
        self.status = SYNSENT
        synretry = 0
        while self.status != CONNECTED:
            temp = Segment.pack(seqNum, 0, 0, 0, 1, 0, 0, b'')
            self.sendto(Segment.pack(seqNum, 0, 0, 0, 1, 0, ReliableImpl.checksum(temp), b''))

            try:
                seg = self.recvfrom()
            except Exception as e:
                if synretry > 60:
                    self.status = CLOSED
                    return -1
                synretry += 1
                continue
            if ReliableImpl.checksum(seg) != 0:
                continue

            seg = Segment(seg)
            if seg.syn and seg.ack and seg.ackNum == (seqNum+1) % SeqNumSpace:
                self.status = CONNECTED
        self.reliImpl = ReliableImpl(self, seqNum)
        self.handlerThread = Handler(self, self.reliImpl)
        self.handlerThread.start()
        return 0

    def close(self):
        self.putTask(None)
        self.handlerThread.join()

    def getTask(self):
        return self.__buf.get()  # block if queue is empty

    def putTask(self, task):
        return self.__buf.put(task)  # block if queue is full

    def send(self, task):
        return self.putTask(task)

    def sendto(self, seg):
        return self.__skt.sendto(seg, self.__dst)

    def recvfrom(self):
        (seg, addr) = self.__skt.recvfrom(SegmentSize)
        return seg

    def updateRWND(self, rwnd):
        self.rwnd = rwnd
        return self.rwnd

    def setTimer(self, timesec, callback, args):
        timer = Timer(timesec, callback, args)
        heapq.heappush(self.timerHeap, timer)
        return timer


class Handler(threading.Thread):
    def __init__(self, reli=None, reliImpl=None):
        super().__init__()
        self.reli = reli
        self.reliImpl = reliImpl

    def run(self):
        inputs = [self.reli.getskt()]
        outputs = [self.reli.getskt()]
        while self.reli.status != CLOSED:
            readable, writable, exceptional = select.select(inputs, outputs, inputs)
            for skt in readable:
                seg = self.reli.recvfrom()
                if ReliableImpl.checksum(seg) != 0:
                    continue
                seg = Segment(seg)

                if self.reli.status == CONNECTED:
                    if seg.ack and not seg.syn and not seg.fin:
                        self.reli.bytesInFly += self.reliImpl.recvAck(seg, False)
                elif self.reli.status == FINWAIT:
                    if seg.ack and not seg.syn and not seg.fin:
                        self.reli.bytesInFly += self.reliImpl.recvAck(seg, False)
                    elif seg.ack and seg.fin:
                        self.reli.bytesInFly += self.reliImpl.recvAck(seg, True)
                        self.reli.status = CLOSED

            for skt in writable:
                if self.reli.status != CONNECTED or self.reli.bytesInFly >= min(self.reli.rwnd, self.reli.cwnd):
                    continue

                block = self.reli.getTask()
                if block is None:
                    self.reli.bytesInFly += self.reliImpl.sendData(b'', True)
                    self.reli.status = FINWAIT
                else:
                    self.reli.bytesInFly += self.reliImpl.sendData(block, False)

            timestamp = time.time()
            while len(self.reli.timerHeap) > 0:
                timer = self.reli.timerHeap[0]
                if timestamp < timer.timestamp:
                    break
                heapq.heappop(self.reli.timerHeap)
                if timer.enable:
                    timer.run()
