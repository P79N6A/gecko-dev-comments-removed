









#ifndef WEBRTC_MODULES_VIDEO_CODING_TIMESTAMP_MAP_H_
#define WEBRTC_MODULES_VIDEO_CODING_TIMESTAMP_MAP_H_

#include "typedefs.h"

namespace webrtc
{

struct VCMTimestampDataTuple
{
    WebRtc_UWord32    timestamp;
    void*             data;
};

class VCMTimestampMap
{
public:
    
    
    VCMTimestampMap(const WebRtc_Word32 length = 10);

    
    ~VCMTimestampMap();

    
    void Reset();

    WebRtc_Word32 Add(WebRtc_UWord32 timestamp, void*  data);
    void* Pop(WebRtc_UWord32 timestamp);

private:
    bool IsEmpty() const;

    VCMTimestampDataTuple* _map;
    WebRtc_Word32                   _nextAddIx;
    WebRtc_Word32                   _nextPopIx;
    WebRtc_Word32                   _length;
};

} 

#endif 
