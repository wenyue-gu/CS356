from Util import *


class ReliableImpl:
    def __init__(self, reli=None, seqNum=None):
        super().__init__()
        self.reli = reli
        self.seqNum = seqNum
        #TODO: Your code here

    @staticmethod
    def checksum(s):
        #TODO: Your code here

    def recvAck(self, seg, isFin):
        #TODO: Your code here

    def sendData(self, block, isFin):
        #TODO: Your code here

    def retransmission(self, seqNum, blocklen):
        #TODO: Your code here
