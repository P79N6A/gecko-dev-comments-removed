









#ifndef WEBRTC_MODULES_AUDIO_CONFERENCE_MIXER_SOURCE_LEVEL_INDICATOR_H_
#define WEBRTC_MODULES_AUDIO_CONFERENCE_MIXER_SOURCE_LEVEL_INDICATOR_H_

#include "typedefs.h"

namespace webrtc {
class LevelIndicator
{
public:
    enum{TICKS_BEFORE_CALCULATION = 10};

    LevelIndicator();
    ~LevelIndicator();

    
    void ComputeLevel(const WebRtc_Word16* speech,
                      const WebRtc_UWord16 nrOfSamples);

    WebRtc_Word32 GetLevel();
private:
    WebRtc_Word32  _max;
    WebRtc_UWord32 _count;
    WebRtc_UWord32 _currentLevel;
};
} 

#endif
