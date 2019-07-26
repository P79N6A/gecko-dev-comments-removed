









#include "bitrate_estimator.h"

namespace webrtc {

const float kBitrateAverageWindow = 500.0f;

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
            kBitrateAverageWindow)
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
    return static_cast<WebRtc_UWord32>(_accumulatedBytes * 8.0f * 1000.0f /
                                       kBitrateAverageWindow + 0.5f);
}

}  
