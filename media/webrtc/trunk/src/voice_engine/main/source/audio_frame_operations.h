









#ifndef WEBRTC_VOICE_ENGINE_AUDIO_FRAME_OPERATIONS_H
#define WEBRTC_VOICE_ENGINE_AUDIO_FRAME_OPERATIONS_H

#include "typedefs.h"

namespace webrtc {

class AudioFrame;

namespace voe {

class AudioFrameOperations
{
public:
    static WebRtc_Word32 MonoToStereo(AudioFrame& audioFrame);

    static WebRtc_Word32 StereoToMono(AudioFrame& audioFrame);

    static WebRtc_Word32 Mute(AudioFrame& audioFrame);

    static WebRtc_Word32 Scale(const float left,
                               const float right,
                               AudioFrame& audioFrame);

    static WebRtc_Word32 ScaleWithSat(const float scale,
                                      AudioFrame& audioFrame);
};

}  

}  

#endif  
