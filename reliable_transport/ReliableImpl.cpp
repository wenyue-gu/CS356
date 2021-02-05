#include "ReliableImpl.h"

// ReliableImpl: Constructor. You can add variables to maintain your
// sliding window states.
// 'reli' provides an interface to call functions of class Reliable in ReliableImpl.
// 'seqNum' indicates the initail sequence number in the SYN segment.
ReliableImpl::ReliableImpl(Reliable *_reli, uint32_t _seqNum)
{
    reli = _reli;
    seqNum = _seqNum;

    //TODO: Your code here
}

// ~ReliableImpl: Destructor, free the memory for maintaining states.
ReliableImpl::~ReliableImpl()
{
    //TODO: Your code here
}

// checksum: 16-bit Internet checksum (refer to RFC 1071 for calculation)
// This function should return the value of checksum (an unsigned 16-bit integer).
// You should calculate the checksum over 'buf', which is an array of char bytes
// 'len' ist the length of buf.
uint16_t ReliableImpl::checksum(const char *buf, ssize_t len)
{
    //TODO: Your code here
    return 0;
}

// recvAck: When an ACK or FINACK segment is received, the framework will
// call this function to handle the segment.
// The checksum will be verified before calling this function, so you
// do not need to verify checksum again in this function.
// Remember to call this->reli->updateRWND to update the receive window size.
// Note that this function should return the reduction of bytes in flight
// (a non-positive integer) so that class Reliable can update the bytes in flight.
// 'seg' is an instance of class Segment (see Util.h)
// 'isFin'=True means 'seg' is a FINACK, otherwise it is an ACK.
int32_t ReliableImpl::recvAck(const Segment *seg, bool isFin)
{
    //TODO: Your code here
    return 0;
}

// sendData: This function is called when a block of data should be sent out.
// You can call Segment::pack in Util.h to encapsulate a segment and
// call this->reli->setTimer (see Reliable.h) to set a Timer for retransmission.
// Use this->reli->Sendto (see Reliable.h) to send a segment to the receiver.
// Note that this function should return the increment of bytes in flight
// (a non-negative integer) so that class Reliable can update the bytes in flight.
// 'block' is an array of char bytes.
// 'blocklen' is the length of block. 
// 'isFin'=True means a FIN segment should be sent out.
int32_t ReliableImpl::sendData(char *block, uint16_t blocklen, bool isFin)
{
    //TODO: Your code here
    return 0;
}

// retransmission: A callback function for retransmission when you call
// this->reli->setTimer.
// In C/C++, you should NOT modify the arguments of this function.
void *ReliableImpl::retransmission(void *args)
{
    //TODO: Your code here
    return NULL;
}
