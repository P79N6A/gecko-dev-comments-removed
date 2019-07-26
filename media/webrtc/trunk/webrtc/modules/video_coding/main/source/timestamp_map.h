









#ifndef WEBRTC_MODULES_VIDEO_CODING_TIMESTAMP_MAP_H_
#define WEBRTC_MODULES_VIDEO_CODING_TIMESTAMP_MAP_H_

#include "webrtc/typedefs.h"

namespace webrtc
{

struct VCMTimestampDataTuple
{
    uint32_t    timestamp;
    void*             data;
};

class VCMTimestampMap
{
public:
    
    
    VCMTimestampMap(const int32_t length = 10);

    
    ~VCMTimestampMap();

    
    void Reset();

    int32_t Add(uint32_t timestamp, void*  data);
    void* Pop(uint32_t timestamp);

private:
    bool IsEmpty() const;

    VCMTimestampDataTuple* _map;
    int32_t                   _nextAddIx;
    int32_t                   _nextPopIx;
    int32_t                   _length;
};

}  

#endif 
