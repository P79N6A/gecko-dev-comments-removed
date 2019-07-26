













#ifndef VPM_DEFLICKERING_H
#define VPM_DEFLICKERING_H

#include <string.h>  

#include "webrtc/modules/video_processing/main/interface/video_processing.h"
#include "webrtc/typedefs.h"

namespace webrtc {

class VPMDeflickering
{
public:
    VPMDeflickering();
    ~VPMDeflickering();

    int32_t ChangeUniqueId(int32_t id);

    void Reset();

    int32_t ProcessFrame(I420VideoFrame* frame,
                         VideoProcessingModule::FrameStats* stats);
private:
    int32_t PreDetection(uint32_t timestamp,
                         const VideoProcessingModule::FrameStats& stats);

    int32_t DetectFlicker();

    enum { kMeanBufferLength = 32 };
    enum { kFrameHistorySize = 15 };
    enum { kNumProbs = 12 };
    enum { kNumQuants = kNumProbs + 2 };
    enum { kMaxOnlyLength = 5 };

    int32_t _id;

    uint32_t  _meanBufferLength;
    uint8_t   _detectionState;    
                                      
                                      
    int32_t    _meanBuffer[kMeanBufferLength];
    uint32_t   _timestampBuffer[kMeanBufferLength];
    uint32_t   _frameRate;
    static const uint16_t _probUW16[kNumProbs];
    static const uint16_t _weightUW16[kNumQuants - kMaxOnlyLength];
    uint8_t _quantHistUW8[kFrameHistorySize][kNumQuants];
};

}  

#endif 
