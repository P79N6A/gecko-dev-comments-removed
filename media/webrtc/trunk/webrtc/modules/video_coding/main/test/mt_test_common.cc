









#include "mt_test_common.h"

#include <cmath>

#include "rtp_dump.h"
#include "webrtc/system_wrappers/interface/clock.h"

namespace webrtc {

TransportCallback::TransportCallback(Clock* clock, const char* filename)
    : RTPSendCompleteCallback(clock, filename) {
}

TransportCallback::~TransportCallback()
{
    
}

int
TransportCallback::SendPacket(int channel, const void *data, int len)
{
    _sendCount++;
    _totalSentLength += len;

    if (_rtpDump != NULL)
    {
        if (_rtpDump->DumpPacket((const uint8_t*)data, len) != 0)
        {
            return -1;
        }
    }

    bool transmitPacket = true;
    
    if (_sendCount > 20)
    {
        transmitPacket = PacketLoss();
    }

    Clock* clock = Clock::GetRealTimeClock();
    int64_t now = clock->TimeInMilliseconds();
    
    if (transmitPacket)
    {
        RtpPacket* newPacket = new RtpPacket();
        memcpy(newPacket->data, data, len);
        newPacket->length = len;
        
        
        
        int32_t
        simulatedDelay = (int32_t)NormalDist(_networkDelayMs,
                                                   sqrt(_jitterVar));
        newPacket->receiveTime = now + simulatedDelay;
        _rtpPackets.push_back(newPacket);
    }
    return 0;
}

int
TransportCallback::TransportPackets()
{
    
    RtpPacket* packet = NULL;
    Clock* clock = Clock::GetRealTimeClock();
    int64_t now = clock->TimeInMilliseconds();

    while (!_rtpPackets.empty())
    {
        
        packet = _rtpPackets.front();
        int64_t timeToReceive = packet->receiveTime - now;
        if (timeToReceive > 0)
        {
            
            break;
        }

        _rtpPackets.pop_front();
        
        if (_rtp->IncomingPacket((const uint8_t*)packet->data,
                                     packet->length) < 0)
        {
            delete packet;
            packet = NULL;
            
            return -1;
        }
        delete packet;
        packet = NULL;
    }
    return 0; 
}



bool VCMProcessingThread(void* obj)
{
    SharedRTPState* state = static_cast<SharedRTPState*>(obj);
    if (state->_vcm.TimeUntilNextProcess() <= 0)
    {
        if (state->_vcm.Process() < 0)
        {
            return false;
        }
    }
    return true;
}


bool VCMDecodeThread(void* obj)
{
    SharedRTPState* state = static_cast<SharedRTPState*>(obj);
    state->_vcm.Decode();
    return true;
}

bool TransportThread(void *obj)
{
    SharedTransportState* state = static_cast<SharedTransportState*>(obj);
    state->_transport.TransportPackets();
    return true;
}

}  
