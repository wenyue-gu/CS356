from Util import *

# You can add necessary functions here


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
        self.first = True
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
        #print(r)
        return r





    # recvAck: When an ACK or FINACK segment is received, the framework will
    # call this function to handle the segment.
    # The checksum will be verified before calling this function, so you
    # do not need to verify checksum again in this function.
    # Remember to call self.reli.updateRWND to update the receive window size.
    # Note that this function should return the reduction of bytes in flight
    # (a non-negative integer) so that class Reliable can update the bytes in flight.
    # 'seg' is an instance of class Segment (see Util.py)
    # 'isFin'=True means 'seg' is a FINACK, otherwise it is an ACK.
    def recvAck(self, seg, isFin):
        # TODO: Your code here
        self.reli.updateRWND(seg.rwnd)
        n=0
        if seg.ackNum>=self.lastcheck:
            n = seg.ackNum - self.lastcheck
        else:
            n = 16**4 - self.lastcheck + seg.ackNum
        if isFin:
            print(seg.ackNum)
        self.lastcheck = seg.ackNum
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
        seq = self.seqNum
        k = Segment.pack(seq, self.srvAckNum, 0, 0, 0, fin, 0, payload)
        s = self.checksum(k)
        actual = Segment.pack(seq, self.srvAckNum, 0, 0, 0, fin, s, payload)
        self.reli.sendto(actual)
        self.seqNum+=ret
        
        self.reli.setTimer(0.3, self.retransmission, (0.6, seq, s, payload, fin))

        return ret

    # retransmission: A callback function for retransmission when you call
    # self.reli.setTimer.
    # In Python, you are allowed to modify the arguments of this function.
    def retransmission(self, time, seq, s, payload, fin):
        # TODO: Your code here
        if seq<self.lastcheck:
            return
        actual = Segment.pack(seq, self.srvAckNum, 0, 0, 0, fin, s, payload)
        self.reli.sendto(actual)
        self.reli.setTimer(time, self.retransmission, (time*2,seq, s, payload, fin))
        pass



# scp -oProxyCommand="ssh -W %h:%p wg74@vcm-18887.vm.duke.edu" /home/vcm/compsci_ece356_spring2021_labs/reliable_transport/trace1.pcap ~/Documents/ECE356

