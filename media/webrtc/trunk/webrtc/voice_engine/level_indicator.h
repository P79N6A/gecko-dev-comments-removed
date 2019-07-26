









#ifndef WEBRTC_VOICE_ENGINE_LEVEL_INDICATOR_H
#define WEBRTC_VOICE_ENGINE_LEVEL_INDICATOR_H

#include "webrtc/typedefs.h"
#include "webrtc/voice_engine/voice_engine_defines.h"

namespace webrtc {

class AudioFrame;
class CriticalSectionWrapper;
namespace voe {

class AudioLevel
{
public:
    AudioLevel();
    virtual ~AudioLevel();

    
    
    int8_t Level() const;
    int16_t LevelFullRange() const;
    void Clear();

    
    
    
    void ComputeLevel(const AudioFrame& audioFrame);

private:
    enum { kUpdateFrequency = 10};

    CriticalSectionWrapper& _critSect;

    int16_t _absMax;
    int16_t _count;
    int8_t _currentLevel;
    int16_t _currentLevelFullRange;
};

}  

}  

#endif
