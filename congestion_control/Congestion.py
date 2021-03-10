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

    # if congestionStatus is in slow start
    if reliImpl.congestionStatus==1:
        #     if a segment is acked
        if acked:
        #         Increase CWND by PAYLOAD_SIZE
            reli.cwnd = reli.cwnd+PayloadSize
        #     else if a segment is lost
        elif loss:
        #         set ssthresh as maximum of half of current CWND and PAYLOAD_SIZE
            ssthresh = max(reli.cwnd/2, PayloadSize)
        #         set CWND as updated ssthresh
            reli.cwnd=ssthresh
        #         set congestionStatus as congestion avoidance
            reliImpl.congestionStatus=2

        #     if CWND is larger than ssthresh
        if reli.cwnd>ssthresh:
        #        set congestionStatus as congestion avoidance
            reliImpl.congestionStatus=2

    # else if congestionStatus is in congestion avoidance
    elif reliImpl.congestionStatus==2:
        #     if a segment is acked
        if acked:
        #         Increase CWND by PAYLOAD_SIZE/(CWND/PAYLOAD_SIZE)
            reli.cwnd = reli.cwnd+PayloadSize/(reli.cwnd/PayloadSize)
        #     else if a segment is lost
        elif loss:
        #         set ssthresh as maximum of half of current CWND and PAYLOAD_SIZE
            ssthresh = max(reli.cwnd/2, PayloadSize)
        #         set CWND as updated ssthresh
            reli.cwnd=ssthresh

    pass


# updateRTO: Run RTT estimation and update RTO.
# You can use time.time() to get current timestamp.
# 'reli' provides an interface to access class Reliable.
# 'reliImpl' provides an interface to access class ReliableImpl.
# 'timestamp' indicates the time when the sampled packet is sent out.
def updateRTO(reli, reliImpl, timestamp):
    #TODO: Your code here
        
    # call get_current_time(C)/time.time(Python) and calculate the RTT
    rtt = time.time()-timestamp
    # if it is the first RTT measurement
    if(first==True):
        #     calculate rttvar
        #     calculate srtt
        #     calculate rto
        first=False
        reliImpl.srtt = rtt
        reliImpl.rttvar = rtt/2
        reliImpl.rto = reliImpl.srtt+max(G, 4*reliImpl.rttvar)

    # else
    else:
        #     calculate rttvar
        #     calculate srtt
        #     calculate rto
        a = reliImpl.srtt - rtt
        if a<0:
            a=a*-1
        reliImpl.rttvar = (1-beta) * reliImpl.rttvar + beta * a
        reliImpl.srtt = (1-alpha) * reliImpl.srtt+alpha*rtt
        reliImpl.rto = reliImpl.srtt+max(G, 4*reliImpl.rttvar)
    # set rto as MIN_RTO if it is smaller than MIN_RTO
    if reliImpl.rto<MinRTO:
        reliImpl.rto=MinRTO
    pass
