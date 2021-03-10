from Util import *
from collections import deque   
from Congestion import *


# You can add necessary functions here
class sgm:
  def __init__(self, timer, segnum, resendFlag, timestamp, value, rto):
      self.timer = timer
      self.segnum = segnum
      self.resendFlag = resendFlag
      self.timestamp = timestamp
      self.value = value
      self.rto=rto


class ReliableImpl:

    #  __init__: You can add variables to maintain your sliding window states.
    # 'reli' (self.reli) provides an interface to call functions of
    # class Reliable in ReliableImpl.
    # 'seqNum' indicates the initail sequence number in the SYN segment.
    # 'srvSeqNum' indicates the initial sequence number in the SYNACK segment.
    def __init__(self, reli=None, seqNum=None, srvSeqNum=None):
        super().__init__()
        self.reli = reli
        self.seqNum = seqNum
        self.srvAckNum = (srvSeqNum+1) % SeqNumSpace  # srvAckNum remains unchanged in this lab
        # TODO: Your code here
        self.lastcheck = seqNum
        #self.largestsent = seqNum
        self.first = True
        self.queue = deque([])
        self.congestionStatus = 1 #slow start, 2=avoidance
        self.srtt=0
        self.ssthresh=30000
        self.rto=MinRTO
        self.rttvar=0
        self.FRCount = 0
        pass

    # checksum: 16-bit Internet checksum (refer to RFC 1071 for calculation)
    # This function should return the value of checksum (an unsigned 16-bit integer).
    # You should calculate the checksum over 's', which is an array of bytes
    # (type(s)=<class 'bytes'>).
    @staticmethod
    def checksum(s):
        # TODO: Your code here
        s1 = 0
        s2 = 0
        for i in range(len(s)):
            if(i%2==0):
                s1+=s[i]
            else:
                s2+=s[i]
        a = True
        while a:
            x1 = 0
            x2 = 0
            if s1 < 256:
                if s2<256:
                    a = False
            if s1>=256:
                x1 = s1//256
                s1 = s1%256
            if s2>=256:
                x2 = s2//256
                s2 = s2%256
            s1 = s1+x2
            s2 = s2+x1
            if s1 < 256:
                if s2<256:
                    a = False
        ret = s1*256 + s2
        while (ret>>16):
           ret = (ret & 0xffff) + (ret >> 16)
        r = ~ret+16**4
        return r

    def checkInWrap(head, tail, index):
        if(head<=tail and (index<head or tail<=index)):
            return False
        if(head>tail and (index<head and tail<=index)):
            return False
        return True

    # recvAck: When an ACK or FINACK segment is received, the framework will
    # call this function to handle the segment.
    # The checksum will be verified before calling this function, so you
    # do not need to verify checksum again in this function.
    # Remember to call self.reli.updateRWND to update the receive window size.
    # You should call fastRetransmission, Congestion.updateRTO and Congestion.updateCWND properly.
    # Note that this function should return the reduction of bytes in flight
    # (a non-negative integer) so that class Reliable can update the bytes in flight.
    # 'seg' is an instance of class Segment (see Util.py)
    # 'isFin'=True means 'seg' is a FINACK, otherwise it is an ACK.
    def recvAck(self, seg, isFin):
        # TODO: Your code here
        # check whether the segment is a duplicate ack (seg.ackNum is largestAcked)

        if seg.ackNum==self.lastcheck:
            # increase FRCount if it is duplicate ack
            self.FRCount+=1
            if self.FRCount>=3:
                # call fastRetransmission and return 0 if FRCount equals to 3
                fastRetransmission(seg.ackNum)
                return 0
        
        # check whether the segment has ever been acked
        wrap = self.checkInWrap(self.lastcheck+1, self.seqNum+2, seg.ackNum)
        # return 0 if the segment is already acked before # ie not wraparound and acknum smaller
        if wrap==False and seg.ackNum < self.lastcheck:
            return 0
        # reset FRCount as 0
        self.FRCount=0
        # calculate the reduction of bytes in flight (See @230 to handle wrap around)
        n=0
        if wrap==False and seg.ackNum > self.lastcheck:
            n = seg.ackNum - self.lastcheck
        else:
            n = 16**4 - self.lastcheck + seg.ackNum

        # get resendFlag and timestamp from the queue
        fl = self.queue[0].resendFlag
        ts = self.queue[0].timestamp
        # call updateRTO if the segment is not retransmitted (resendFlag is false)
        if(resendFlag==False):
            updateRTO(self.reli, self, ts)

        # while queue that stores the sent segments is not empty
        while(self.queue.empty()==False):
            # check the head element of the queue
            a = self.queue[0]
            # break the while loop if the seqNum of the head element is not acked
            if a.segnum>seg.ackNum:
                break
            # cancel the timer of the head element 
            a.timer.cancel()
            # call update cwnd
            updateCWND(self.reli, self, True, False, False)
            # pop out the head of the queue
            self.queue.popleft()
            
        # update the largestAcked
        self.lastcheck = seg.ackNum    
        # updateRWND(seg.rwnd)
        self.reli.updateRWND(seg.rwnd)  
        # return bytes in flight
        return n

    # sendData: This function is called when a piece of payload should be sent out.
    # You can call Segment.pack in Util.py to encapsulate a segment and
    # call self.reli.setTimer (see Reliable.py) to set a Timer for retransmission.
    # Use self.reli.sendto (see Reliable.py) to send a segment to the receiver.
    # Note that this function should return the increment of bytes in flight
    # (a non-negative integer) so that class Reliable can update the bytes in flight.
    # 'payload' is an array of bytes (type(payload)=<class 'bytes'>).
    # 'isFin'=True means a FIN segment should be sent out.
    def sendData(self, payload, isFin):
        # TODO: Your code here

        fin=0
        if isFin:
            fin=1
        if self.first:
            self.first = False
            self.seqNum+=1
            print(self.seqNum)

        ret = len(payload)
        # calculate the new sequence number from largestSent
        seq = self.seqNum
        # segPack and calculate checksum (See connect() in Reliable)
        k = Segment.pack(seq, self.srvAckNum, 0, 0, 0, fin, 0, payload)
        s = self.checksum(k)
        # segPack again (See connect() in Reliable)
        actual = Segment.pack(seq, self.srvAckNum, 0, 0, 0, fin, s, payload)
        # set a timer
        timer = self.reli.setTimer(0.3, self.retransmission, (seq))
        # create a resendFlag (false) for the segment
        flag = False
        # call get_current_time(C)/time.time(Python) to get current timestamp
        t = time.time()
        # push a struct including the timer, resendFlag, current timestamp, and necessary variables into the sent segment queue
        sgment = sgm(timer, seq, flag, t, actual,0.3)
        self.queue.append(sgment)
        # update largestSent
        self.seqNum+=ret
        if ret==0:
            self.seqNum+=1
        # call sendto
        self.reli.sendto(actual)
        # return bytes in flight
        return ret

    # retransmission: A callback function for retransmission when you call
    # self.reli.setTimer.
    # You should call Congestion.updateCWND to update the congestion window size.
    # In Python, you are allowed to modify the arguments of this function.
    def retransmission(self, seq):
        # TODO: Your code here
        if seq<self.lastcheck:
            return

        for i in range(len(self.queue)):
            if(self.queue[i].seqnum==seq):
                a = self.queue[i]
                # double the rto
                a.rto=a.rto*2
                # set timer again with the new rto
                a.timer = self.reli.setTimer(a.rto, self.retransmission, (seq))    
                # call updateCWND if the segment is the next expected one
                if(seq==self.lastcheck):
                    updateCWND(self.reli, self, False, True, False)
                # set resendFlag as true
                a.resendFlag=True
                # call sendto to resend the segment
                self.reli.sendto(a.value)
                break
        pass

    # fastRetransmission: The recvAck uses this function instead of retransmission to
    # do fast retransmission when recvAck considers some segments should be fast retransmitted.
    # You should call Congestion.updateCWND to update the congestion window size.
    # In Python, you are allowed to modify the arguments of this function.
    def fastRetransmission(self, seqNum):
        # TODO: Your code her

        for i in range(len(self.queue)):
            if(self.queue[i].seqnum==seqNum):
                a = self.queue[i]
                # cancel the previous timer
                a.timer.cancel()
                # double the rto
                a.rto=a.rto*2
                # set timer again with the new rto
                a.timer = self.reli.setTimer(a.rto, self.retransmission, (seq))  
                #call updateCWND   
                updateCWND(self.reli, self, False, True, True)  
                # set resendFlag as true
                a.resendFlag=True
                # call sendto to resend the segment
                self.reli.sendto(a.value)
                break
        pass
