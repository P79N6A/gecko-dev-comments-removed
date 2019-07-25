









#include "Bitrate.h"
#include "rtp_utility.h"

#define BITRATE_AVERAGE_WINDOW 2000

namespace webrtc {
Bitrate::Bitrate(RtpRtcpClock* clock) :
    _clock(*clock),
    _packetRate(0),
    _bitrate(0),
    _bitrateNextIdx(0),
    _timeLastRateUpdate(0),
    _bytesCount(0),
    _packetCount(0)
{
    memset(_packetRateArray, 0, sizeof(_packetRateArray));
    memset(_bitrateDiffMS, 0, sizeof(_bitrateDiffMS));
    memset(_bitrateArray, 0, sizeof(_bitrateArray));
}

void
Bitrate::Init()
{
    _packetRate = 0;
    _bitrate = 0;
    _timeLastRateUpdate = 0;
    _bytesCount = 0;
    _packetCount = 0;
    _bitrateNextIdx = 0;

    memset(_packetRateArray, 0, sizeof(_packetRateArray));
    memset(_bitrateDiffMS, 0, sizeof(_bitrateDiffMS));
    memset(_bitrateArray, 0, sizeof(_bitrateArray));
}

void
Bitrate::Update(const WebRtc_Word32 bytes)
{
    _bytesCount += bytes;
    _packetCount++;
}

WebRtc_UWord32
Bitrate::PacketRate() const
{
    return _packetRate;
}

WebRtc_UWord32
Bitrate::BitrateLast() const
{
    return _bitrate;
}

WebRtc_UWord32
Bitrate::BitrateNow() const
{
    WebRtc_UWord32 now = _clock.GetTimeInMS();
    WebRtc_UWord32 diffMS = now -_timeLastRateUpdate;

    if(diffMS > 10000) 
    {
        
        return _bitrate; 
    }
    WebRtc_UWord64 bitsSinceLastRateUpdate = 8*_bytesCount*1000;

    
    
    WebRtc_UWord64 bitrate = (((WebRtc_UWord64)_bitrate * 1000) + bitsSinceLastRateUpdate)/(1000+diffMS);
    return (WebRtc_UWord32)bitrate;
}

void
Bitrate::Process()
{
    
    WebRtc_UWord32 now = _clock.GetTimeInMS();
    WebRtc_UWord32 diffMS = now -_timeLastRateUpdate;

    if(diffMS > 100)
    {
        if(diffMS > 10000) 
        {
            
            _timeLastRateUpdate = now;
            _bytesCount = 0;
            _packetCount = 0;
            return;
        }
        _packetRateArray[_bitrateNextIdx] = (_packetCount*1000)/diffMS;
        _bitrateArray[_bitrateNextIdx]    = 8*((_bytesCount*1000)/diffMS);
        
        _bitrateDiffMS[_bitrateNextIdx]   = diffMS;
        _bitrateNextIdx++;
        if(_bitrateNextIdx >= 10)
        {
            _bitrateNextIdx = 0;
        }

        WebRtc_UWord32 sumDiffMS = 0;
        WebRtc_UWord64 sumBitrateMS = 0;
        WebRtc_UWord32 sumPacketrateMS = 0;
        for(int i= 0; i <10; i++)
        {
            
            sumDiffMS += _bitrateDiffMS[i];
            sumBitrateMS += _bitrateArray[i] * _bitrateDiffMS[i];
            sumPacketrateMS += _packetRateArray[i] * _bitrateDiffMS[i];
        }
        _timeLastRateUpdate = now;
        _bytesCount = 0;
        _packetCount = 0;

        _packetRate = sumPacketrateMS/sumDiffMS;
        _bitrate = WebRtc_UWord32(sumBitrateMS / sumDiffMS);
    }
}

BitRateStats::BitRateStats()
    :_dataSamples(), _accumulatedBytes(0)
{
}

BitRateStats::~BitRateStats()
{
    while (_dataSamples.size() > 0)
    {
        delete _dataSamples.front();
        _dataSamples.pop_front();
    }
}

void BitRateStats::Init()
{
    _accumulatedBytes = 0;
    while (_dataSamples.size() > 0)
    {
        delete _dataSamples.front();
        _dataSamples.pop_front();
    }
}

void BitRateStats::Update(WebRtc_UWord32 packetSizeBytes, WebRtc_Word64 nowMs)
{
    
    
    _dataSamples.push_back(new DataTimeSizeTuple(packetSizeBytes, nowMs));
    _accumulatedBytes += packetSizeBytes;
    EraseOld(nowMs);
}

void BitRateStats::EraseOld(WebRtc_Word64 nowMs)
{
    while (_dataSamples.size() > 0)
    {
        if (nowMs - _dataSamples.front()->_timeCompleteMs >
                                                        BITRATE_AVERAGE_WINDOW)
        {
            
            _accumulatedBytes -= _dataSamples.front()->_sizeBytes;
            delete _dataSamples.front();
            _dataSamples.pop_front();
        }
        else
        {
            break;
        }
    }
}

WebRtc_UWord32 BitRateStats::BitRate(WebRtc_Word64 nowMs)
{
    
    
    EraseOld(nowMs);
    WebRtc_Word64 timeOldest = nowMs;
    if (_dataSamples.size() > 0)
    {
        timeOldest = _dataSamples.front()->_timeCompleteMs;
    }
    
    float denom = static_cast<float>(nowMs - timeOldest);
    if (nowMs == timeOldest)
    {
        
        
        denom = 1000.0;
    }
    return static_cast<WebRtc_UWord32>(_accumulatedBytes * 8.0f * 1000.0f /
                                       denom + 0.5f);
}
} 
