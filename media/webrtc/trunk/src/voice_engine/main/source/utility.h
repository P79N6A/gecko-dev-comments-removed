













#ifndef WEBRTC_VOICE_ENGINE_UTILITY_H
#define WEBRTC_VOICE_ENGINE_UTILITY_H

#include "typedefs.h"
#include "voice_engine_defines.h"

namespace webrtc
{

class Module;

namespace voe
{

class Utility
{
public:
    static void MixWithSat(WebRtc_Word16 target[],
                           int target_channel,
                           const WebRtc_Word16 source[],
                           int source_channel,
                           int source_len);

    static void MixSubtractWithSat(WebRtc_Word16 target[],
                                   const WebRtc_Word16 source[],
                                   WebRtc_UWord16 len);

    static void MixAndScaleWithSat(WebRtc_Word16 target[],
                                   const WebRtc_Word16 source[],
                                   float scale,
                                   WebRtc_UWord16 len);

    static void Scale(WebRtc_Word16 vector[], float scale, WebRtc_UWord16 len);

    static void ScaleWithSat(WebRtc_Word16 vector[],
                             float scale,
                             WebRtc_UWord16 len);
};

} 

} 

#endif  
