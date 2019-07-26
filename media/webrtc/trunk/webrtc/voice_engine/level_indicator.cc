









#include "critical_section_wrapper.h"
#include "level_indicator.h"
#include "module_common_types.h"
#include "signal_processing_library.h"

namespace webrtc {

namespace voe {




const int8_t permutation[33] =
    {0,1,2,3,4,4,5,5,5,5,6,6,6,6,6,7,7,7,7,8,8,8,9,9,9,9,9,9,9,9,9,9,9};


AudioLevel::AudioLevel() :
    _critSect(*CriticalSectionWrapper::CreateCriticalSection()),
    _absMax(0),
    _count(0),
    _currentLevel(0),
    _currentLevelFullRange(0) {
}

AudioLevel::~AudioLevel() {
    delete &_critSect;
}

void AudioLevel::Clear()
{
    CriticalSectionScoped cs(&_critSect);
    _absMax = 0;
    _count = 0;
    _currentLevel = 0;
    _currentLevelFullRange = 0;
}

void AudioLevel::ComputeLevel(const AudioFrame& audioFrame)
{
    int16_t absValue(0);

    
    absValue = WebRtcSpl_MaxAbsValueW16(
        audioFrame.data_,
        audioFrame.samples_per_channel_*audioFrame.num_channels_);

    
    
    CriticalSectionScoped cs(&_critSect);

    if (absValue > _absMax)
    _absMax = absValue;

    
    if (_count++ == kUpdateFrequency)
    {
        _currentLevelFullRange = _absMax;

        _count = 0;

        
        
        
        int32_t position = _absMax/1000;

        
        
        if ((position == 0) && (_absMax > 250))
        {
            position = 1;
        }
        _currentLevel = permutation[position];

        
        _absMax >>= 2;
    }
}

int8_t AudioLevel::Level() const
{
    CriticalSectionScoped cs(&_critSect);
    return _currentLevel;
}

int16_t AudioLevel::LevelFullRange() const
{
    CriticalSectionScoped cs(&_critSect);
    return _currentLevelFullRange;
}

}  

}  
