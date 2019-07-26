









#ifndef WEBRTC_MODULES_VIDEO_CODING_UTILITY_INCLUDE_FRAME_DROPPER_H_
#define WEBRTC_MODULES_VIDEO_CODING_UTILITY_INCLUDE_FRAME_DROPPER_H_

#include "webrtc/modules/video_coding/utility/include/exp_filter.h"
#include "webrtc/typedefs.h"

namespace webrtc
{




class FrameDropper
{
public:
    FrameDropper();
    explicit FrameDropper(float max_time_drops);
    virtual ~FrameDropper() {}

    
    
    
    virtual void Reset();

    virtual void Enable(bool enable);
    
    
    
    
    
    virtual bool DropFrame();
    
    
    
    
    
    
    
    
    
    
    virtual void Fill(uint32_t frameSizeBytes, bool deltaFrame);

    virtual void Leak(uint32_t inputFrameRate);

    void UpdateNack(uint32_t nackBytes);

    
    
    
    
    
    virtual void SetRates(float bitRate, float incoming_frame_rate);

    
    
    
    virtual float ActualFrameRate(uint32_t inputFrameRate) const;

private:
    void FillBucket(float inKbits, float outKbits);
    void UpdateRatio();
    void CapAccumulator();

    VCMExpFilter       _keyFrameSizeAvgKbits;
    VCMExpFilter       _keyFrameRatio;
    float           _keyFrameSpreadFrames;
    int32_t     _keyFrameCount;
    float           _accumulator;
    float           _accumulatorMax;
    float           _targetBitRate;
    bool            _dropNext;
    VCMExpFilter       _dropRatio;
    int32_t     _dropCount;
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
