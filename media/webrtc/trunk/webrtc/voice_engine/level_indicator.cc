









#include "level_indicator.h"
#include "module_common_types.h"
#include "signal_processing_library.h"

namespace webrtc {

namespace voe {





const WebRtc_Word8 permutation[33] =
    {0,1,2,3,4,4,5,5,5,5,6,6,6,6,6,7,7,7,7,8,8,8,9,9,9,9,9,9,9,9,9,9,9};


AudioLevel::AudioLevel() :
    _absMax(0),
    _count(0),
    _currentLevel(0),
    _currentLevelFullRange(0)
{
}

AudioLevel::~AudioLevel()
{
}

void
AudioLevel::Clear()
{
    _absMax = 0;
    _count = 0;
    _currentLevel = 0;
    _currentLevelFullRange = 0;
}

void
AudioLevel::ComputeLevel(const AudioFrame& audioFrame)
{
    WebRtc_Word16 absValue(0);

    
    absValue = WebRtcSpl_MaxAbsValueW16(
        audioFrame.data_,
        audioFrame.samples_per_channel_*audioFrame.num_channels_);
    if (absValue > _absMax)
    _absMax = absValue;

    
    if (_count++ == kUpdateFrequency)
    {
        _currentLevelFullRange = _absMax;

        _count = 0;

        
        
        
        WebRtc_Word32 position = _absMax/1000;

        
        
        if ((position == 0) && (_absMax > 250))
        {
            position = 1;
        }
        _currentLevel = permutation[position];

        
        _absMax >>= 2;
    }
}

WebRtc_Word8
AudioLevel::Level() const
{
    return _currentLevel;
}

WebRtc_Word16
AudioLevel::LevelFullRange() const
{
    return _currentLevelFullRange;
}

}  

}  
