













#ifndef VPM_DEFLICKERING_H
#define VPM_DEFLICKERING_H

#include "typedefs.h"
#include "video_processing.h"

#include <cstring>  

namespace webrtc {

class VPMDeflickering
{
public:
    VPMDeflickering();
    ~VPMDeflickering();

    WebRtc_Word32 ChangeUniqueId(WebRtc_Word32 id);

    void Reset();

    WebRtc_Word32 ProcessFrame(WebRtc_UWord8* frame,
                             WebRtc_UWord32 width,
                             WebRtc_UWord32 height,
                             WebRtc_UWord32 timestamp,
                             VideoProcessingModule::FrameStats& stats);
private:
    WebRtc_Word32 PreDetection(WebRtc_UWord32 timestamp,
                             const VideoProcessingModule::FrameStats& stats);

    WebRtc_Word32 DetectFlicker();

    enum { kMeanBufferLength = 32 };
    enum { kFrameHistorySize = 15 };
    enum { kNumProbs = 12 };
    enum { kNumQuants = kNumProbs + 2 };
    enum { kMaxOnlyLength = 5 };

    WebRtc_Word32 _id;

    WebRtc_UWord32  _meanBufferLength;
    WebRtc_UWord8   _detectionState;    
                                      
                                      
    WebRtc_Word32    _meanBuffer[kMeanBufferLength];
    WebRtc_UWord32   _timestampBuffer[kMeanBufferLength];
    WebRtc_UWord32   _frameRate;
    static const WebRtc_UWord16 _probUW16[kNumProbs];
    static const WebRtc_UWord16 _weightUW16[kNumQuants - kMaxOnlyLength];
    WebRtc_UWord8 _quantHistUW8[kFrameHistorySize][kNumQuants];
};

} 

#endif 

