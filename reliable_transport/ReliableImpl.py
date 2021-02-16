from Util import *


class ReliableImpl:

    #  __init__: You can add variables to maintain your sliding window states.
    # 'reli' (self.reli) provides an interface to call functions of
    # class Reliable in ReliableImpl.
    # 'seqNum' indicates the initail sequence number in the SYN segment.
    def __init__(self, reli=None, seqNum=None):
        super().__init__()
        self.reli = reli
        self.seqNum = seqNum
        # TODO: Your code here
        pass

    # checksum: 16-bit Internet checksum (refer to RFC 1071 for calculation)
    # This function should return the value of checksum (an unsigned 16-bit integer).
    # You should calculate the checksum over 's', which is an array of bytes
    # (type(s)=<class 'bytes'>).
    @staticmethod
    def checksum(s):
        # TODO: Your code here
        #s = '\x00\x01\xf2\x03\xf4\xf5\xf6\xf7'
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
        print(hex(ret))
        return ret





    # recvAck: When an ACK or FINACK segment is received, the framework will
    # call this function to handle the segment.
    # The checksum will be verified before calling this function, so you
    # do not need to verify checksum again in this function.
    # Remember to call self.reli.updateRWND to update the receive window size.
    # Note that this function should return the reduction of bytes in flight
    # (a non-positive integer) so that class Reliable can update the bytes in flight.
    # 'seg' is an instance of class Segment (see Util.py)
    # 'isFin'=True means 'seg' is a FINACK, otherwise it is an ACK.
    def recvAck(self, seg, isFin):
        # TODO: Your code here
        return 0
# scp -oProxyCommand="ssh -W %h:%p wg74@vcm-18887.vm.duke.edu"  /home/vcm/compsci_ece356_spring2021_labs/reliable_transport/Receiver  wg74@cs356.cs.duke.edu:~/
# ssh -oProxyCommand="ssh -W %h:%p wg74@vcm-18887.vm.duke.edu"  wg74@cs356.cs.duke.edu "pkill -u wg74 Receiver; ./Receiver temp.txt -p 7017 -s 50 -f 25"

    # sendData: This function is called when a block of data should be sent out.
    # You can call Segment.pack in Util.py to encapsulate a segment and
    # call self.reli.setTimer (see Reliable.py) to set a Timer for retransmission.
    # Use self.reli.sendto (see Reliable.py) to send a segment to the receiver.
    # Note that this function should return the increment of bytes in flight
    # (a non-negative integer) so that class Reliable can update the bytes in flight.
    # 'block' is an array of bytes (type(block)=<class 'bytes'>).
    # 'isFin'=True means a FIN segment should be sent out.
    def sendData(self, block, isFin):
        # TODO: Your code here
        return len(block)

    # retransmission: A callback function for retransmission when you call
    # self.reli.setTimer.
    # In python, you are allowed to modify the arguments of this function.
    def retransmission(self, seqNum):
        # TODO: Your code here
        pass


# scp -oProxyCommand="ssh -W %h:%p wg74@vcm-18887.vm.duke.edu" /home/vcm/compsci_ece356_spring2021_labs/reliable_transport/trace1.pcap ~/Documents/ECE356

