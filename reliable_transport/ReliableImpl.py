from Util import *


class ReliableImpl:

    #  __init__: You can add variables to maintain your sliding window states.
    # 'reli' (self.reli) provide an interface to call functions in
    # class Reliable in ReliableImpl.
    # 'seqNum' indicates the initail sequence number in the SYN segment.
    def __init__(self, reli=None, seqNum=None):
        super().__init__()
        self.reli = reli
        self.seqNum = seqNum
        # TODO: Your code here
        pass

    # checksum: 16-bit Internet checksum (refer to RFC 1071 for calculation)
    # You should calculate the checksum over 's', which is an array of bytes
    # (type(s)=<class 'bytes'>).
    @staticmethod
    def checksum(s):
        # TODO: Your code here
        pass

    # recvAck: When an ACK or FINACK segment is received, the framework will
    # call this function to handle the segment.
    # The checksum will be verified before calling this function, so you
    # do not need to verify checksum  again in this function.
    # Remember to call self.reli.updateRWND to update the receive window size.
    # 'seg' is an instance of class Segment (see Util.py)
    # 'isFin'=True means seg is a FINACK, otherwise it is an ACK.
    def recvAck(self, seg, isFin):
        # TODO: Your code here
        pass

    # sendData: This function is called when a block of data should be sent out.
    # You can call Segment.pack in Util.py to encapsulate a segment and
    # call self.reli.setTimer (see Reliable.py) set a Timer for retransmission.
    # Use self.reli.sendto (see Reliable.py) to send a segment to the receiver.
    # 'block' is an array of bytes (type(block)=<class 'bytes'>).
    # 'isFin'=True means a FIN segment should be sent out.
    def sendData(self, block, isFin):
        # TODO: Your code here
        pass

    # retransmission: A callback function for retransmission when you call
    # self.reli.setTimer.
    # You are allowed to modify the arguments of this function.
    def retransmission(self, seqNum):
        # TODO: Your code here
        pass
