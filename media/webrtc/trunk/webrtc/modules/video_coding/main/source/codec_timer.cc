









#include "webrtc/modules/video_coding/main/source/codec_timer.h"

#include <assert.h>

namespace webrtc
{


static const int32_t kIgnoredSampleCount = 5;

VCMCodecTimer::VCMCodecTimer()
:
_filteredMax(0),
_ignoredSampleCount(0),
_shortMax(0),
_history()
{
    Reset();
}

int32_t VCMCodecTimer::StopTimer(int64_t startTimeMs, int64_t nowMs)
{
    const int32_t timeDiff = static_cast<int32_t>(nowMs - startTimeMs);
    MaxFilter(timeDiff, nowMs);
    return timeDiff;
}

void VCMCodecTimer::Reset()
{
    _filteredMax = 0;
    _ignoredSampleCount = 0;
    _shortMax = 0;
    for (int i=0; i < MAX_HISTORY_SIZE; i++)
    {
        _history[i].shortMax = 0;
        _history[i].timeMs = -1;
    }
}


void VCMCodecTimer::MaxFilter(int32_t decodeTime, int64_t nowMs)
{
    if (_ignoredSampleCount >= kIgnoredSampleCount)
    {
        UpdateMaxHistory(decodeTime, nowMs);
        ProcessHistory(nowMs);
    }
    else
    {
        _ignoredSampleCount++;
    }
}

void
VCMCodecTimer::UpdateMaxHistory(int32_t decodeTime, int64_t now)
{
    if (_history[0].timeMs >= 0 &&
        now - _history[0].timeMs < SHORT_FILTER_MS)
    {
        if (decodeTime > _shortMax)
        {
            _shortMax = decodeTime;
        }
    }
    else
    {
        
        if(_history[0].timeMs == -1)
        {
            
            _shortMax = decodeTime;
        }
        else
        {
            
            for(int i = (MAX_HISTORY_SIZE - 2); i >= 0 ; i--)
            {
                _history[i+1].shortMax = _history[i].shortMax;
                _history[i+1].timeMs = _history[i].timeMs;
            }
        }
        if (_shortMax == 0)
        {
            _shortMax = decodeTime;
        }

        _history[0].shortMax = _shortMax;
        _history[0].timeMs = now;
        _shortMax = 0;
    }
}

void
VCMCodecTimer::ProcessHistory(int64_t nowMs)
{
    _filteredMax = _shortMax;
    if (_history[0].timeMs == -1)
    {
        return;
    }
    for (int i=0; i < MAX_HISTORY_SIZE; i++)
    {
        if (_history[i].timeMs == -1)
        {
            break;
        }
        if (nowMs - _history[i].timeMs > MAX_HISTORY_SIZE * SHORT_FILTER_MS)
        {
            
            break;
        }
        if (_history[i].shortMax > _filteredMax)
        {
            
            _filteredMax = _history[i].shortMax;
        }
    }
}


int32_t VCMCodecTimer::RequiredDecodeTimeMs(FrameType ) const
{
    return _filteredMax;
}

}
