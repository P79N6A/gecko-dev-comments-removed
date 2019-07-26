









#ifndef WEBRTC_MODULES_AUDIO_CONFERENCE_MIXER_SOURCE_LEVEL_INDICATOR_H_
#define WEBRTC_MODULES_AUDIO_CONFERENCE_MIXER_SOURCE_LEVEL_INDICATOR_H_

#include "webrtc/typedefs.h"

namespace webrtc {
class LevelIndicator
{
public:
    enum{TICKS_BEFORE_CALCULATION = 10};

    LevelIndicator();
    ~LevelIndicator();

    
    void ComputeLevel(const int16_t* speech,
                      const uint16_t nrOfSamples);

    int32_t GetLevel();
private:
    int32_t  _max;
    uint32_t _count;
    uint32_t _currentLevel;
};
}  

#endif
