













#ifndef WEBRTC_VOICE_ENGINE_UTILITY_H
#define WEBRTC_VOICE_ENGINE_UTILITY_H

#include "webrtc/typedefs.h"
#include "webrtc/voice_engine/voice_engine_defines.h"

namespace webrtc
{

class Module;

namespace voe
{

class Utility
{
public:
    static void MixWithSat(int16_t target[],
                           int target_channel,
                           const int16_t source[],
                           int source_channel,
                           int source_len);

    static void MixSubtractWithSat(int16_t target[],
                                   const int16_t source[],
                                   uint16_t len);

    static void MixAndScaleWithSat(int16_t target[],
                                   const int16_t source[],
                                   float scale,
                                   uint16_t len);

    static void Scale(int16_t vector[], float scale, uint16_t len);

    static void ScaleWithSat(int16_t vector[],
                             float scale,
                             uint16_t len);
};

}  

}  

#endif  
