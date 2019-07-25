









#ifndef WEBRTC_MODULES_RTP_RTCP_SOURCE_BITRATE_H_
#define WEBRTC_MODULES_RTP_RTCP_SOURCE_BITRATE_H_

#include "typedefs.h"
#include "rtp_rtcp_config.h"     
#include "common_types.h"            
#include <stdio.h>
#include <list>

namespace webrtc {
class RtpRtcpClock;

class Bitrate
{
public:
    Bitrate(RtpRtcpClock* clock);

    
    void Init();

    
    void Process();

    
    void Update(const WebRtc_Word32 bytes);

    
    WebRtc_UWord32 PacketRate() const;

    
    WebRtc_UWord32 BitrateLast() const;

    
    WebRtc_UWord32 BitrateNow() const;

protected:
    RtpRtcpClock&             _clock;

private:
    WebRtc_UWord32            _packetRate;
    WebRtc_UWord32            _bitrate;
    WebRtc_UWord8             _bitrateNextIdx;
    WebRtc_UWord32            _packetRateArray[10];
    WebRtc_UWord32            _bitrateArray[10];
    WebRtc_UWord32            _bitrateDiffMS[10];
    WebRtc_UWord32            _timeLastRateUpdate;
    WebRtc_UWord32            _bytesCount;
    WebRtc_UWord32            _packetCount;
};

struct DataTimeSizeTuple
{
    DataTimeSizeTuple(WebRtc_UWord32 sizeBytes, WebRtc_Word64 timeCompleteMs) :
                            _sizeBytes(sizeBytes),
                            _timeCompleteMs(timeCompleteMs) {}

    WebRtc_UWord32    _sizeBytes;
    WebRtc_Word64     _timeCompleteMs;
};

class BitRateStats
{
public:
    BitRateStats();
    ~BitRateStats();

    void Init();
    void Update(WebRtc_UWord32 packetSizeBytes, WebRtc_Word64 nowMs);
    WebRtc_UWord32 BitRate(WebRtc_Word64 nowMs);

private:
    void EraseOld(WebRtc_Word64 nowMs);

    std::list<DataTimeSizeTuple*> _dataSamples;
    WebRtc_UWord32                _accumulatedBytes;
};
} 

#endif 
