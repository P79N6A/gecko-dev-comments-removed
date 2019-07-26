









#ifndef WEBRTC_MODULES_REMOTE_BITRATE_ESTIMATOR_BITRATE_ESTIMATOR_H_
#define WEBRTC_MODULES_REMOTE_BITRATE_ESTIMATOR_BITRATE_ESTIMATOR_H_

#include <list>

#include "typedefs.h"

namespace webrtc {

class BitRateStats
{
public:
    BitRateStats();
    ~BitRateStats();

    void Init();
    void Update(WebRtc_UWord32 packetSizeBytes, WebRtc_Word64 nowMs);
    WebRtc_UWord32 BitRate(WebRtc_Word64 nowMs);

private:
    struct DataTimeSizeTuple
    {
        DataTimeSizeTuple(uint32_t sizeBytes, int64_t timeCompleteMs)
            :
              _sizeBytes(sizeBytes),
              _timeCompleteMs(timeCompleteMs) {}

        WebRtc_UWord32    _sizeBytes;
        WebRtc_Word64     _timeCompleteMs;
    };

    void EraseOld(WebRtc_Word64 nowMs);

    std::list<DataTimeSizeTuple*> _dataSamples;
    WebRtc_UWord32 _accumulatedBytes;
};

}  

#endif  
