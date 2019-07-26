









#include "webrtc/modules/video_coding/main/source/timing.h"

#include "webrtc/modules/video_coding/main/source/internal_defines.h"
#include "webrtc/modules/video_coding/main/source/jitter_buffer_common.h"
#include "webrtc/modules/video_coding/main/source/timestamp_extrapolator.h"
#include "webrtc/system_wrappers/interface/clock.h"
#include "webrtc/system_wrappers/interface/trace.h"

namespace webrtc {

VCMTiming::VCMTiming(Clock* clock,
                     int32_t vcmId,
                     int32_t timingId,
                     VCMTiming* masterTiming)
:
_critSect(CriticalSectionWrapper::CreateCriticalSection()),
_vcmId(vcmId),
_clock(clock),
_timingId(timingId),
_master(false),
_tsExtrapolator(),
_codecTimer(),
_renderDelayMs(kDefaultRenderDelayMs),
_minTotalDelayMs(0),
_requiredDelayMs(0),
_currentDelayMs(0),
_prevFrameTimestamp(0),
_maxVideoDelayMs(kMaxVideoDelayMs)
{
    if (masterTiming == NULL)
    {
        _master = true;
        _tsExtrapolator = new VCMTimestampExtrapolator(_clock, vcmId, timingId);
    }
    else
    {
        _tsExtrapolator = masterTiming->_tsExtrapolator;
    }
}

VCMTiming::~VCMTiming()
{
    if (_master)
    {
        delete _tsExtrapolator;
    }
    delete _critSect;
}

void
VCMTiming::Reset(int64_t nowMs )
{
    CriticalSectionScoped cs(_critSect);
    if (nowMs > -1)
    {
        _tsExtrapolator->Reset(nowMs);
    }
    else
    {
        _tsExtrapolator->Reset();
    }
    _codecTimer.Reset();
    _renderDelayMs = kDefaultRenderDelayMs;
    _minTotalDelayMs = 0;
    _requiredDelayMs = 0;
    _currentDelayMs = 0;
    _prevFrameTimestamp = 0;
}

void VCMTiming::ResetDecodeTime()
{
    _codecTimer.Reset();
}

void
VCMTiming::SetRenderDelay(uint32_t renderDelayMs)
{
    CriticalSectionScoped cs(_critSect);
    _renderDelayMs = renderDelayMs;
}

void
VCMTiming::SetMinimumTotalDelay(uint32_t minTotalDelayMs)
{
    CriticalSectionScoped cs(_critSect);
    _minTotalDelayMs = minTotalDelayMs;
}

void
VCMTiming::SetRequiredDelay(uint32_t requiredDelayMs)
{
    CriticalSectionScoped cs(_critSect);
    if (requiredDelayMs != _requiredDelayMs)
    {
        if (_master)
        {
            WEBRTC_TRACE(webrtc::kTraceDebug, webrtc::kTraceVideoCoding, VCMId(_vcmId, _timingId),
                    "Desired jitter buffer level: %u ms", requiredDelayMs);
        }
        _requiredDelayMs = requiredDelayMs;
        
        if (_currentDelayMs == 0) {
           _currentDelayMs = _requiredDelayMs;
        }
    }
}

void VCMTiming::UpdateCurrentDelay(uint32_t frameTimestamp)
{
    CriticalSectionScoped cs(_critSect);
    uint32_t targetDelayMs = TargetDelayInternal();

    
    if (targetDelayMs < _minTotalDelayMs)
    {
        targetDelayMs = _minTotalDelayMs;
    }

    if (_currentDelayMs == 0)
    {
        
        _currentDelayMs = targetDelayMs;
    }
    else if (targetDelayMs != _currentDelayMs)
    {
        int64_t delayDiffMs = static_cast<int64_t>(targetDelayMs) -
                                    _currentDelayMs;
        
        
        
        
        
        int64_t maxChangeMs = 0;
        if (frameTimestamp < 0x0000ffff && _prevFrameTimestamp > 0xffff0000)
        {
            
            maxChangeMs = kDelayMaxChangeMsPerS * (frameTimestamp +
                         (static_cast<int64_t>(1)<<32) - _prevFrameTimestamp) / 90000;
        }
        else
        {
            maxChangeMs = kDelayMaxChangeMsPerS *
                          (frameTimestamp - _prevFrameTimestamp) / 90000;
        }
        if (maxChangeMs <= 0)
        {
            
            
            
            return;
        }
        else if (delayDiffMs < -maxChangeMs)
        {
            delayDiffMs = -maxChangeMs;
        }
        else if (delayDiffMs > maxChangeMs)
        {
            delayDiffMs = maxChangeMs;
        }
        _currentDelayMs = _currentDelayMs + static_cast<int32_t>(delayDiffMs);
    }
    _prevFrameTimestamp = frameTimestamp;
}

void VCMTiming::UpdateCurrentDelay(int64_t renderTimeMs,
                                   int64_t actualDecodeTimeMs)
{
    CriticalSectionScoped cs(_critSect);
    uint32_t targetDelayMs = TargetDelayInternal();
    
    if (targetDelayMs < _minTotalDelayMs)
    {
        targetDelayMs = _minTotalDelayMs;
    }
    int64_t delayedMs = actualDecodeTimeMs -
                              (renderTimeMs - MaxDecodeTimeMs() - _renderDelayMs);
    if (delayedMs < 0)
    {
        return;
    }
    else if (_currentDelayMs + delayedMs <= targetDelayMs)
    {
        _currentDelayMs += static_cast<uint32_t>(delayedMs);
    }
    else
    {
        _currentDelayMs = targetDelayMs;
    }
}

int32_t
VCMTiming::StopDecodeTimer(uint32_t timeStamp,
                           int64_t startTimeMs,
                           int64_t nowMs)
{
    CriticalSectionScoped cs(_critSect);
    const int32_t maxDecTime = MaxDecodeTimeMs();
    int32_t timeDiffMs = _codecTimer.StopTimer(startTimeMs, nowMs);
    if (timeDiffMs < 0)
    {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideoCoding, VCMId(_vcmId, _timingId),
            "Codec timer error: %d", timeDiffMs);
        assert(false);
    }

    if (_master)
    {
        WEBRTC_TRACE(webrtc::kTraceDebug, webrtc::kTraceVideoCoding, VCMId(_vcmId, _timingId),
                "Frame decoded: timeStamp=%u decTime=%d maxDecTime=%u, at %u",
                timeStamp, timeDiffMs, maxDecTime, MaskWord64ToUWord32(nowMs));
    }
    return 0;
}

void
VCMTiming::IncomingTimestamp(uint32_t timeStamp, int64_t nowMs)
{
    CriticalSectionScoped cs(_critSect);
    _tsExtrapolator->Update(nowMs, timeStamp, _master);
}

int64_t
VCMTiming::RenderTimeMs(uint32_t frameTimestamp, int64_t nowMs) const
{
    CriticalSectionScoped cs(_critSect);
    const int64_t renderTimeMs = RenderTimeMsInternal(frameTimestamp, nowMs);
    if (renderTimeMs < 0)
    {
        return renderTimeMs;
    }
    if (_master)
    {
        WEBRTC_TRACE(webrtc::kTraceDebug, webrtc::kTraceVideoCoding, VCMId(_vcmId, _timingId),
            "Render frame %u at %u. Render delay %u, required delay %u,"
                " max decode time %u, min total delay %u",
            frameTimestamp, MaskWord64ToUWord32(renderTimeMs), _renderDelayMs,
            _requiredDelayMs, MaxDecodeTimeMs(),_minTotalDelayMs);
    }
    return renderTimeMs;
}

int64_t
VCMTiming::RenderTimeMsInternal(uint32_t frameTimestamp, int64_t nowMs) const
{
    int64_t estimatedCompleteTimeMs =
            _tsExtrapolator->ExtrapolateLocalTime(frameTimestamp);
    if (estimatedCompleteTimeMs - nowMs > _maxVideoDelayMs)
    {
        if (_master)
        {
            WEBRTC_TRACE(webrtc::kTraceDebug, webrtc::kTraceVideoCoding, VCMId(_vcmId, _timingId),
                    "Timestamp arrived 2 seconds early, reset statistics",
                    frameTimestamp, estimatedCompleteTimeMs);
        }
        return -1;
    }
    if (_master)
    {
        WEBRTC_TRACE(webrtc::kTraceDebug, webrtc::kTraceVideoCoding, VCMId(_vcmId, _timingId),
                "ExtrapolateLocalTime(%u)=%u ms",
                frameTimestamp, MaskWord64ToUWord32(estimatedCompleteTimeMs));
    }
    if (estimatedCompleteTimeMs == -1)
    {
        estimatedCompleteTimeMs = nowMs;
    }
    return estimatedCompleteTimeMs + _currentDelayMs;
}


int32_t
VCMTiming::MaxDecodeTimeMs(FrameType frameType ) const
{
    const int32_t decodeTimeMs = _codecTimer.RequiredDecodeTimeMs(frameType);

    if (decodeTimeMs < 0)
    {
        WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideoCoding, VCMId(_vcmId, _timingId),
            "Negative maximum decode time: %d", decodeTimeMs);
        return -1;
    }
    return decodeTimeMs;
}

uint32_t
VCMTiming::MaxWaitingTime(int64_t renderTimeMs, int64_t nowMs) const
{
    CriticalSectionScoped cs(_critSect);

    const int64_t maxWaitTimeMs = renderTimeMs - nowMs -
                                        MaxDecodeTimeMs() - _renderDelayMs;

    if (maxWaitTimeMs < 0)
    {
        return 0;
    }
    return static_cast<uint32_t>(maxWaitTimeMs);
}

bool
VCMTiming::EnoughTimeToDecode(uint32_t availableProcessingTimeMs) const
{
    CriticalSectionScoped cs(_critSect);
    int32_t maxDecodeTimeMs = MaxDecodeTimeMs();
    if (maxDecodeTimeMs < 0)
    {
        
        
        return true;
    }
    else if (maxDecodeTimeMs == 0)
    {
        
        
        maxDecodeTimeMs = 1;
    }
    return static_cast<int32_t>(availableProcessingTimeMs) - maxDecodeTimeMs > 0;
}

void VCMTiming::SetMaxVideoDelay(int maxVideoDelayMs)
{
    CriticalSectionScoped cs(_critSect);
    _maxVideoDelayMs = maxVideoDelayMs;
}

uint32_t
VCMTiming::TargetVideoDelay() const
{
    CriticalSectionScoped cs(_critSect);
    return TargetDelayInternal();
}

uint32_t
VCMTiming::TargetDelayInternal() const
{
    return std::max(_minTotalDelayMs,
                    _requiredDelayMs + MaxDecodeTimeMs() + _renderDelayMs);
}

}
