









#include "media_optimization.h"

#include "content_metrics_processing.h"
#include "frame_dropper.h"
#include "qm_select.h"
#include "modules/video_coding/main/source/tick_time_base.h"

namespace webrtc {

VCMMediaOptimization::VCMMediaOptimization(WebRtc_Word32 id,
                                           TickTimeBase* clock):
_id(id),
_clock(clock),
_maxBitRate(0),
_sendCodecType(kVideoCodecUnknown),
_codecWidth(0),
_codecHeight(0),
_userFrameRate(0),
_fractionLost(0),
_sendStatisticsZeroEncode(0),
_maxPayloadSize(1460),
_targetBitRate(0),
_incomingFrameRate(0),
_enableQm(false),
_videoProtectionCallback(NULL),
_videoQMSettingsCallback(NULL),
_encodedFrameSamples(),
_avgSentBitRateBps(0.0f),
_keyFrameCnt(0),
_deltaFrameCnt(0),
_lastQMUpdateTime(0),
_lastChangeTime(0),
_numLayers(0)
{
    memset(_sendStatistics, 0, sizeof(_sendStatistics));
    memset(_incomingFrameTimes, -1, sizeof(_incomingFrameTimes));

    _frameDropper  = new VCMFrameDropper(_id);
    _lossProtLogic = new VCMLossProtectionLogic(_clock->MillisecondTimestamp());
    _content = new VCMContentMetricsProcessing();
    _qmResolution = new VCMQmResolution();
}

VCMMediaOptimization::~VCMMediaOptimization(void)
{
    _lossProtLogic->Release();
    delete _lossProtLogic;
    delete _frameDropper;
    delete _content;
    delete _qmResolution;
}

WebRtc_Word32
VCMMediaOptimization::Reset()
{
    memset(_incomingFrameTimes, -1, sizeof(_incomingFrameTimes));
    _incomingFrameRate = 0.0;
    _frameDropper->Reset();
    _lossProtLogic->Reset(_clock->MillisecondTimestamp());
    _frameDropper->SetRates(0, 0);
    _content->Reset();
    _qmResolution->Reset();
    _lossProtLogic->UpdateFrameRate(_incomingFrameRate);
    _lossProtLogic->Reset(_clock->MillisecondTimestamp());
    _sendStatisticsZeroEncode = 0;
    _targetBitRate = 0;
    _codecWidth = 0;
    _codecHeight = 0;
    _userFrameRate = 0;
    _keyFrameCnt = 0;
    _deltaFrameCnt = 0;
    _lastQMUpdateTime = 0;
    _lastChangeTime = 0;
    for (WebRtc_Word32 i = 0; i < kBitrateMaxFrameSamples; i++)
    {
        _encodedFrameSamples[i]._sizeBytes = -1;
        _encodedFrameSamples[i]._timeCompleteMs = -1;
    }
    _avgSentBitRateBps = 0.0f;
    _numLayers = 1;
    return VCM_OK;
}

WebRtc_UWord32
VCMMediaOptimization::SetTargetRates(WebRtc_UWord32 bitRate,
                                     WebRtc_UWord8 &fractionLost,
                                     WebRtc_UWord32 roundTripTimeMs)
{
    VCMProtectionMethod *selectedMethod = _lossProtLogic->SelectedMethod();
    _lossProtLogic->UpdateBitRate(static_cast<float>(bitRate));
    _lossProtLogic->UpdateRtt(roundTripTimeMs);
    _lossProtLogic->UpdateResidualPacketLoss(static_cast<float>(fractionLost));

    
    float actualFrameRate = SentFrameRate();

    
    if (actualFrameRate  < 1.0)
    {
        actualFrameRate = 1.0;
    }

    
    
    _lossProtLogic->UpdateFrameRate(actualFrameRate);

    _fractionLost = fractionLost;

    
    
    
    
    FilterPacketLossMode filter_mode = kMaxFilter;
    WebRtc_UWord8 packetLossEnc = _lossProtLogic->FilteredLoss(
        _clock->MillisecondTimestamp(), filter_mode, fractionLost);

    
    _lossProtLogic->UpdateFilteredLossPr(packetLossEnc);

    
    uint32_t protection_overhead_kbps = 0;

    
    float sent_video_rate = 0.0f;
    if (selectedMethod)
    {
        
        selectedMethod->UpdateContentMetrics(_content->ShortTermAvgData());

        
        
        
        _lossProtLogic->UpdateMethod();

        
        uint32_t sent_video_rate_bps = 0;
        uint32_t sent_nack_rate_bps = 0;
        uint32_t sent_fec_rate_bps = 0;
        
        
        
        UpdateProtectionCallback(selectedMethod,
                                 &sent_video_rate_bps,
                                 &sent_nack_rate_bps,
                                 &sent_fec_rate_bps);
        uint32_t sent_total_rate_bps = sent_video_rate_bps +
            sent_nack_rate_bps + sent_fec_rate_bps;
        
        
        if (sent_total_rate_bps > 0) {
          protection_overhead_kbps = static_cast<uint32_t>(bitRate *
              static_cast<double>(sent_nack_rate_bps + sent_fec_rate_bps) /
              sent_total_rate_bps + 0.5);
        }
        
        if (protection_overhead_kbps > bitRate / 2)
          protection_overhead_kbps = bitRate / 2;

        
        
        packetLossEnc = selectedMethod->RequiredPacketLossER();
        sent_video_rate =  static_cast<float>(sent_video_rate_bps / 1000.0);
    }

    
    _targetBitRate = bitRate - protection_overhead_kbps;

    
    _frameDropper->SetRates(static_cast<float>(_targetBitRate),
                                               _incomingFrameRate);

    if (_enableQm)
    {
        
        _qmResolution->UpdateRates((float)_targetBitRate, sent_video_rate,
                                  _incomingFrameRate, _fractionLost);
        
        bool selectQM = checkStatusForQMchange();
        if (selectQM)
        {
            SelectQuality();
        }
        
        _content->ResetShortTermAvgData();
    }

    return _targetBitRate;
}

int VCMMediaOptimization::UpdateProtectionCallback(
    VCMProtectionMethod *selected_method,
    uint32_t* video_rate_bps,
    uint32_t* nack_overhead_rate_bps,
    uint32_t* fec_overhead_rate_bps)
{
    if (!_videoProtectionCallback)
    {
        return VCM_OK;
    }
    FecProtectionParams delta_fec_params;
    FecProtectionParams key_fec_params;
    
    key_fec_params.fec_rate = selected_method->RequiredProtectionFactorK();

    
    delta_fec_params.fec_rate =
        selected_method->RequiredProtectionFactorD();

    
    key_fec_params.use_uep_protection =
        selected_method->RequiredUepProtectionK();

    
    delta_fec_params.use_uep_protection =
        selected_method->RequiredUepProtectionD();

    
    
    delta_fec_params.max_fec_frames = selected_method->MaxFramesFec();
    key_fec_params.max_fec_frames = selected_method->MaxFramesFec();

    
    
    
    
    delta_fec_params.fec_mask_type = kFecMaskRandom;
    key_fec_params.fec_mask_type = kFecMaskRandom;

    
    return _videoProtectionCallback->ProtectionRequest(&delta_fec_params,
                                                       &key_fec_params,
                                                       video_rate_bps,
                                                       nack_overhead_rate_bps,
                                                       fec_overhead_rate_bps);
}

bool
VCMMediaOptimization::DropFrame()
{
    
    _frameDropper->Leak((WebRtc_UWord32)(InputFrameRate() + 0.5f));

    return _frameDropper->DropFrame();
}

WebRtc_Word32
VCMMediaOptimization::SentFrameCount(VCMFrameCount &frameCount) const
{
    frameCount.numDeltaFrames = _deltaFrameCnt;
    frameCount.numKeyFrames = _keyFrameCnt;
    return VCM_OK;
}

WebRtc_Word32
VCMMediaOptimization::SetEncodingData(VideoCodecType sendCodecType,
                                      WebRtc_Word32 maxBitRate,
                                      WebRtc_UWord32 frameRate,
                                      WebRtc_UWord32 bitRate,
                                      WebRtc_UWord16 width,
                                      WebRtc_UWord16 height,
                                      int numLayers)
{
    
    
    
    
    _lastChangeTime = _clock->MillisecondTimestamp();
    _content->Reset();
    _content->UpdateFrameRate(frameRate);

    _maxBitRate = maxBitRate;
    _sendCodecType = sendCodecType;
    _targetBitRate = bitRate;
    _lossProtLogic->UpdateBitRate(static_cast<float>(bitRate));
    _lossProtLogic->UpdateFrameRate(static_cast<float>(frameRate));
    _lossProtLogic->UpdateFrameSize(width, height);
    _lossProtLogic->UpdateNumLayers(numLayers);
    _frameDropper->Reset();
    _frameDropper->SetRates(static_cast<float>(bitRate),
                            static_cast<float>(frameRate));
    _userFrameRate = static_cast<float>(frameRate);
    _codecWidth = width;
    _codecHeight = height;
    _numLayers = (numLayers <= 1) ? 1 : numLayers;  
    WebRtc_Word32 ret = VCM_OK;
    ret = _qmResolution->Initialize((float)_targetBitRate, _userFrameRate,
                                    _codecWidth, _codecHeight, _numLayers);
    return ret;
}

WebRtc_Word32
VCMMediaOptimization::RegisterProtectionCallback(VCMProtectionCallback*
                                                 protectionCallback)
{
    _videoProtectionCallback = protectionCallback;
    return VCM_OK;

}

void
VCMMediaOptimization::EnableFrameDropper(bool enable)
{
    _frameDropper->Enable(enable);
}

void
VCMMediaOptimization::EnableProtectionMethod(bool enable,
                                             VCMProtectionMethodEnum method)
{
    bool updated = false;
    if (enable)
    {
        updated = _lossProtLogic->SetMethod(method);
    }
    else
    {
        _lossProtLogic->RemoveMethod(method);
    }
    if (updated)
    {
        _lossProtLogic->UpdateMethod();
    }
}

bool
VCMMediaOptimization::IsProtectionMethodEnabled(VCMProtectionMethodEnum method)
{
    return (_lossProtLogic->SelectedType() == method);
}

void
VCMMediaOptimization::SetMtu(WebRtc_Word32 mtu)
{
    _maxPayloadSize = mtu;
}

float
VCMMediaOptimization::SentFrameRate()
{
    if (_frameDropper)
    {
        return _frameDropper->ActualFrameRate((WebRtc_UWord32)(InputFrameRate()
                                                               + 0.5f));
    }

    return VCM_CODEC_ERROR;
}

float
VCMMediaOptimization::SentBitRate()
{
    UpdateBitRateEstimate(-1, _clock->MillisecondTimestamp());
    return _avgSentBitRateBps / 1000.0f;
}

WebRtc_Word32
VCMMediaOptimization::MaxBitRate()
{
    return _maxBitRate;
}

WebRtc_Word32
VCMMediaOptimization::UpdateWithEncodedData(WebRtc_Word32 encodedLength,
                                            FrameType encodedFrameType)
{
    
    UpdateBitRateEstimate(encodedLength, _clock->MillisecondTimestamp());
    if(encodedLength > 0)
    {
        const bool deltaFrame = (encodedFrameType != kVideoFrameKey &&
                                 encodedFrameType != kVideoFrameGolden);

        _frameDropper->Fill(encodedLength, deltaFrame);
        if (_maxPayloadSize > 0 && encodedLength > 0)
        {
            const float minPacketsPerFrame = encodedLength /
                                             static_cast<float>(_maxPayloadSize);
            if (deltaFrame)
            {
                _lossProtLogic->UpdatePacketsPerFrame(
                    minPacketsPerFrame, _clock->MillisecondTimestamp());
            }
            else
            {
                _lossProtLogic->UpdatePacketsPerFrameKey(
                    minPacketsPerFrame, _clock->MillisecondTimestamp());
            }

            if (_enableQm)
            {
                
                _qmResolution->UpdateEncodedSize(encodedLength,
                                                 encodedFrameType);
            }
        }
        if (!deltaFrame && encodedLength > 0)
        {
            _lossProtLogic->UpdateKeyFrameSize(static_cast<float>(encodedLength));
        }

        
        if (deltaFrame)
        {
            _deltaFrameCnt++;
        }
        else
        {
            _keyFrameCnt++;
        }

    }

     return VCM_OK;

}

void VCMMediaOptimization::UpdateBitRateEstimate(WebRtc_Word64 encodedLength,
                                                 WebRtc_Word64 nowMs)
{
    int i = kBitrateMaxFrameSamples - 1;
    WebRtc_UWord32 frameSizeSum = 0;
    WebRtc_Word64 timeOldest = -1;
    
    
    for (; i >= 0; i--)
    {
        if (_encodedFrameSamples[i]._sizeBytes == -1)
        {
            
            break;
        }
        if (nowMs - _encodedFrameSamples[i]._timeCompleteMs <
            kBitrateAverageWinMs)
        {
            frameSizeSum += static_cast<WebRtc_UWord32>
                            (_encodedFrameSamples[i]._sizeBytes);
            if (timeOldest == -1)
            {
                timeOldest = _encodedFrameSamples[i]._timeCompleteMs;
            }
        }
    }
    if (encodedLength > 0)
    {
        if (i < 0)
        {
            
            for (i = kBitrateMaxFrameSamples - 2; i >= 0; i--)
            {
                _encodedFrameSamples[i + 1] = _encodedFrameSamples[i];
            }
            i++;
        }
        
        _encodedFrameSamples[i]._sizeBytes = encodedLength;
        _encodedFrameSamples[i]._timeCompleteMs = nowMs;
    }
    if (timeOldest > -1)
    {
        
        float denom = static_cast<float>(nowMs - timeOldest);
        if (denom < 1.0)
        {
            denom = 1.0;
        }
        _avgSentBitRateBps = (frameSizeSum + encodedLength) * 8 * 1000 / denom;
    }
    else if (encodedLength > 0)
    {
        _avgSentBitRateBps = static_cast<float>(encodedLength * 8);
    }
    else
    {
        _avgSentBitRateBps = 0;
    }
}


WebRtc_Word32
VCMMediaOptimization::RegisterVideoQMCallback(VCMQMSettingsCallback*
                                              videoQMSettings)
{
    _videoQMSettingsCallback = videoQMSettings;
    
    if (_videoQMSettingsCallback != NULL)
    {
        _enableQm = true;
    }
    else
    {
        _enableQm = false;
    }
    return VCM_OK;
}

void
VCMMediaOptimization::updateContentData(const VideoContentMetrics*
                                        contentMetrics)
{
    
    if (contentMetrics == NULL)
    {
         
         _enableQm = false;
         _qmResolution->Reset();
    }
    else
    {
        _content->UpdateContentData(contentMetrics);
    }
}

WebRtc_Word32
VCMMediaOptimization::SelectQuality()
{
    
    _qmResolution->ResetQM();

    
    _qmResolution->UpdateContent(_content->LongTermAvgData());

    
    VCMResolutionScale* qm = NULL;
    WebRtc_Word32 ret = _qmResolution->SelectResolution(&qm);
    if (ret < 0)
    {
        return ret;
    }

    
    QMUpdate(qm);

    
    _qmResolution->ResetRates();

    
    _lastQMUpdateTime = _clock->MillisecondTimestamp();

    
    _content->Reset();

    return VCM_OK;
}






bool
VCMMediaOptimization::checkStatusForQMchange()
{

    bool status  = true;

    
    
    
    
    WebRtc_Word64 now = _clock->MillisecondTimestamp();
    if ((now - _lastQMUpdateTime) < kQmMinIntervalMs ||
        (now  - _lastChangeTime) <  kQmMinIntervalMs)
    {
        status = false;
    }

    return status;

}

bool VCMMediaOptimization::QMUpdate(VCMResolutionScale* qm) {
  
  if (!qm->change_resolution_spatial && !qm->change_resolution_temporal) {
    return false;
  }

  
  if (qm->change_resolution_temporal) {
    _incomingFrameRate = qm->frame_rate;
    
    memset(_incomingFrameTimes, -1, sizeof(_incomingFrameTimes));
  }

  
  if (qm->change_resolution_spatial) {
    _codecWidth = qm->codec_width;
    _codecHeight = qm->codec_height;
  }

  WEBRTC_TRACE(webrtc::kTraceDebug, webrtc::kTraceVideoCoding, _id,
               "Resolution change from QM select: W = %d, H = %d, FR = %f",
               qm->codec_width, qm->codec_height, qm->frame_rate);

  
  
  
  
  
  
  _videoQMSettingsCallback->SetVideoQMSettings(qm->frame_rate,
                                               _codecWidth,
                                               _codecHeight);
  _content->UpdateFrameRate(qm->frame_rate);
  _qmResolution->UpdateCodecParameters(qm->frame_rate, _codecWidth,
                                       _codecHeight);
  return true;
}

void
VCMMediaOptimization::UpdateIncomingFrameRate()
{
    WebRtc_Word64 now = _clock->MillisecondTimestamp();
    if (_incomingFrameTimes[0] == 0)
    {
        
    } else
    {
        
        for(WebRtc_Word32 i = (kFrameCountHistorySize - 2); i >= 0 ; i--)
        {
            _incomingFrameTimes[i+1] = _incomingFrameTimes[i];
        }
    }
    _incomingFrameTimes[0] = now;
    ProcessIncomingFrameRate(now);
}


void
VCMMediaOptimization::ProcessIncomingFrameRate(WebRtc_Word64 now)
{
    WebRtc_Word32 num = 0;
    WebRtc_Word32 nrOfFrames = 0;
    for (num = 1; num < (kFrameCountHistorySize - 1); num++)
    {
        if (_incomingFrameTimes[num] <= 0 ||
            
            now - _incomingFrameTimes[num] > kFrameHistoryWinMs)
        {
            break;
        } else
        {
            nrOfFrames++;
        }
    }
    if (num > 1)
    {
        const WebRtc_Word64 diff = now - _incomingFrameTimes[num-1];
        _incomingFrameRate = 1.0;
        if(diff >0)
        {
            _incomingFrameRate = nrOfFrames * 1000.0f / static_cast<float>(diff);
        }
    }
}

WebRtc_UWord32
VCMMediaOptimization::InputFrameRate()
{
    ProcessIncomingFrameRate(_clock->MillisecondTimestamp());
    return WebRtc_UWord32 (_incomingFrameRate + 0.5f);
}

}
