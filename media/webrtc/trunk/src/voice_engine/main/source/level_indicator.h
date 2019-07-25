









#ifndef WEBRTC_VOICE_ENGINE_LEVEL_INDICATOR_H
#define WEBRTC_VOICE_ENGINE_LEVEL_INDICATOR_H

#include "typedefs.h"
#include "voice_engine_defines.h"

namespace webrtc {

class AudioFrame;
namespace voe {

class AudioLevel
{
public:
    AudioLevel();
    virtual ~AudioLevel();

    void ComputeLevel(const AudioFrame& audioFrame);

    WebRtc_Word8 Level() const;

    WebRtc_Word16 LevelFullRange() const;

    void Clear();

private:
    enum { kUpdateFrequency = 10};

    WebRtc_Word16 _absMax;
    WebRtc_Word16 _count;
    WebRtc_Word8 _currentLevel;
    WebRtc_Word16 _currentLevelFullRange;
};

}  

}  

#endif
