









#ifndef WEBRTC_MODULES_VIDEO_CODING_FRAME_DROPPER_H_
#define WEBRTC_MODULES_VIDEO_CODING_FRAME_DROPPER_H_

#include "exp_filter.h"
#include "typedefs.h"

namespace webrtc
{







class VCMFrameDropper
{
public:
    VCMFrameDropper(WebRtc_Word32 vcmId = 0);
    
    
    
    void Reset();

    void Enable(bool enable);
    
    
    
    
    
    bool DropFrame();
    
    
    
    
    
    
    
    
    
    
    void Fill(WebRtc_UWord32 frameSizeBytes, bool deltaFrame);

    void Leak(WebRtc_UWord32 inputFrameRate);

    void UpdateNack(WebRtc_UWord32 nackBytes);

    
    
    
    
    
    void SetRates(float bitRate, float incoming_frame_rate);

    
    
    
    float ActualFrameRate(WebRtc_UWord32 inputFrameRate) const;


private:
    void FillBucket(float inKbits, float outKbits);
    void UpdateRatio();
    void CapAccumulator();

    WebRtc_Word32     _vcmId;
    VCMExpFilter       _keyFrameSizeAvgKbits;
    VCMExpFilter       _keyFrameRatio;
    float           _keyFrameSpreadFrames;
    WebRtc_Word32     _keyFrameCount;
    float           _accumulator;
    float           _accumulatorMax;
    float           _targetBitRate;
    bool            _dropNext;
    VCMExpFilter       _dropRatio;
    WebRtc_Word32     _dropCount;
    float           _windowSize;
    float           _incoming_frame_rate;
    bool            _wasBelowMax;
    bool            _enabled;
    bool            _fastMode;
    float           _cap_buffer_size;
    float           _max_time_drops;
}; 

} 

#endif 
