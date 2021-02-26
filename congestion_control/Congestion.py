import time
import math
from Util import *

MinRTO = 0.3
G = 0.1
K = 4
alpha = 0.125
beta = 0.25

# You can add variables here


# updateCWND: Update reli.cwnd according to the congestion control algorithm.
# 'reli' provides an interface to access class Reliable.
# 'reliImpl' provides an interface to access class ReliableImpl.
# 'acked'=True when a segment is acked.
# 'loss'=True when a segment is considered lost or should be fast retransmitted.
# 'fast'=True when a segment should be fast retransmitted.
def updateCWND(reli, reliImpl, acked=False, loss=False, fast=False):
    #TODO: Your code here
    pass


# updateRTO: Run RTT estimation and update RTO.
# You can use time.time() to get current timestamp.
# 'reli' provides an interface to access class Reliable.
# 'reliImpl' provides an interface to access class ReliableImpl.
# 'timestamp' indicates the time when the sampled packet is sent out.
def updateRTO(reli, reliImpl, timestamp):
    #TODO: Your code here
    pass
