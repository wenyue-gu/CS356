import time
import math
from Util import *

MinRTO = 0.3
G = 0.1
K = 4
alpha = 0.125
beta = 0.25

# You can add variables here
first = True


# updateCWND: Update reli.cwnd according to the congestion control algorithm.
# 'reli' provides an interface to access class Reliable.
# 'reliImpl' provides an interface to access class ReliableImpl.
# 'acked'=True when a segment is acked.
# 'loss'=True when a segment is considered lost or should be fast retransmitted.
# 'fast'=True when a segment should be fast retransmitted.
def updateCWND(reli, reliImpl, acked=False, loss=False, fast=False):
    #TODO: Your code here
    if reliImpl.congestionStatus==1:
        if acked:
            reli.cwnd = reli.cwnd+PayloadSize
        else if loss:
            ssthresh = max(reli.cwnd/2, PayloadSize)
            reli.cwnd=ssthresh
            reliImpl.congestionStatus=2
        if reli.cwnd>ssthresh:
            reliImpl.congestionStatus=2

    else if reliImpl.congestionStatus==2:

    pass


# updateRTO: Run RTT estimation and update RTO.
# You can use time.time() to get current timestamp.
# 'reli' provides an interface to access class Reliable.
# 'reliImpl' provides an interface to access class ReliableImpl.
# 'timestamp' indicates the time when the sampled packet is sent out.
def updateRTO(reli, reliImpl, timestamp):
    #TODO: Your code here
    rtt = time.time()-timestamp
    if(first==True):
        first=False
        reliImpl.srtt = rtt
        reliImpl.rttvar = rtt/2
    else:
        a = reliImpl.srtt - rtt
        if a<0:
            a=a*-1
        reliImpl.rttvar = (1-beta) * reliImpl.rttvar + beta * a
        reliImpl.srtt = (1-alpha) * reliImpl.srtt+alpha*rtt
    
    reliImpl.rto = reliImpl.srtt+max(G, 4*reliImpl.rttvar)

    if reliImpl.rto<MIN_RTO:
        reliImpl.rto=MIN_RTO
    pass
