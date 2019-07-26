









#include "video_coding_impl.h"
#include "common_video/libyuv/include/webrtc_libyuv.h"
#include "common_types.h"
#include "encoded_frame.h"
#include "jitter_buffer.h"
#include "packet.h"
#include "trace.h"
#include "video_codec_interface.h"
#include "webrtc/system_wrappers/interface/clock.h"
#include "webrtc/system_wrappers/interface/trace_event.h"

namespace webrtc
{



uint32_t
VCMProcessTimer::Period() const
{
    return _periodMs;
}

uint32_t
VCMProcessTimer::TimeUntilProcess() const
{
    const int64_t time_since_process = _clock->TimeInMilliseconds() -
        static_cast<int64_t>(_latestMs);
    const int64_t time_until_process = static_cast<int64_t>(_periodMs) -
        time_since_process;
    if (time_until_process < 0)
      return 0;
    return time_until_process;
}

void
VCMProcessTimer::Processed()
{
    _latestMs = _clock->TimeInMilliseconds();
}

VideoCodingModuleImpl::VideoCodingModuleImpl(const int32_t id,
                                             Clock* clock,
                                             EventFactory* event_factory,
                                             bool owns_event_factory)
    : _id(id),
      clock_(clock),
      _receiveCritSect(CriticalSectionWrapper::CreateCriticalSection()),
      _receiverInited(false),
      _timing(clock_, id, 1),
      _dualTiming(clock_, id, 2, &_timing),
      _receiver(&_timing, clock_, event_factory, id, 1, true),
      _dualReceiver(&_dualTiming, clock_, event_factory, id, 2, false),
      _decodedFrameCallback(_timing, clock_),
      _dualDecodedFrameCallback(_dualTiming, clock_),
      _frameTypeCallback(NULL),
      _frameStorageCallback(NULL),
      _receiveStatsCallback(NULL),
      _packetRequestCallback(NULL),
      render_buffer_callback_(NULL),
      _decoder(NULL),
      _dualDecoder(NULL),
#ifdef DEBUG_DECODER_BIT_STREAM
      _bitStreamBeforeDecoder(NULL),
#endif
      _frameFromFile(),
      _keyRequestMode(kKeyOnError),
      _scheduleKeyRequest(false),
      max_nack_list_size_(0),
      _sendCritSect(CriticalSectionWrapper::CreateCriticalSection()),
      _encoder(),
      _encodedFrameCallback(),
      _nextFrameTypes(1, kVideoFrameDelta),
      _mediaOpt(id, clock_),
      _sendCodecType(kVideoCodecUnknown),
      _sendStatsCallback(NULL),
      _encoderInputFile(NULL),
      _codecDataBase(id),
      _receiveStatsTimer(1000, clock_),
      _sendStatsTimer(1000, clock_),
      _retransmissionTimer(10, clock_),
      _keyRequestTimer(500, clock_),
      event_factory_(event_factory),
      owns_event_factory_(owns_event_factory),
      frame_dropper_enabled_(true) {
  assert(clock_);
#ifdef DEBUG_DECODER_BIT_STREAM
  _bitStreamBeforeDecoder = fopen("decoderBitStream.bit", "wb");
#endif
}

VideoCodingModuleImpl::~VideoCodingModuleImpl()
{
    if (_dualDecoder != NULL)
    {
        _codecDataBase.ReleaseDecoder(_dualDecoder);
    }
    delete _receiveCritSect;
    delete _sendCritSect;
    if (owns_event_factory_) {
      delete event_factory_;
    }
#ifdef DEBUG_DECODER_BIT_STREAM
    fclose(_bitStreamBeforeDecoder);
#endif
    if (_encoderInputFile != NULL)
    {
        fclose(_encoderInputFile);
    }
}

VideoCodingModule*
VideoCodingModule::Create(const int32_t id)
{
    return new VideoCodingModuleImpl(id, Clock::GetRealTimeClock(),
                                     new EventFactoryImpl, true);
}

VideoCodingModule*
VideoCodingModule::Create(const int32_t id, Clock* clock,
                          EventFactory* event_factory)
{
    assert(clock);
    assert(event_factory);
    return new VideoCodingModuleImpl(id, clock, event_factory, false);
}

void
VideoCodingModule::Destroy(VideoCodingModule* module)
{
    if (module != NULL)
    {
        delete static_cast<VideoCodingModuleImpl*>(module);
    }
}

int32_t
VideoCodingModuleImpl::Process()
{
    int32_t returnValue = VCM_OK;

    
    if (_receiveStatsTimer.TimeUntilProcess() == 0)
    {
        _receiveStatsTimer.Processed();
        if (_receiveStatsCallback != NULL)
        {
            uint32_t bitRate;
            uint32_t frameRate;
            _receiver.ReceiveStatistics(&bitRate, &frameRate);
            _receiveStatsCallback->ReceiveStatistics(bitRate, frameRate);
        }

        
        if (render_buffer_callback_) {
          int buffer_size_ms = _receiver.RenderBufferSizeMs();
          render_buffer_callback_->RenderBufferSizeMs(buffer_size_ms);
      }
    }

    
    if (_sendStatsTimer.TimeUntilProcess() == 0)
    {
        _sendStatsTimer.Processed();
        if (_sendStatsCallback != NULL)
        {
            uint32_t bitRate;
            uint32_t frameRate;
            {
                CriticalSectionScoped cs(_sendCritSect);
                bitRate = _mediaOpt.SentBitRate();
                frameRate = _mediaOpt.SentFrameRate();
            }
            _sendStatsCallback->SendStatistics(bitRate, frameRate);
        }
    }

    
    
    
    if (_retransmissionTimer.TimeUntilProcess() == 0)
    {
        _retransmissionTimer.Processed();
        if (_packetRequestCallback != NULL)
        {
            uint16_t length;
            {
                CriticalSectionScoped cs(_receiveCritSect);
                length = max_nack_list_size_;
            }
            std::vector<uint16_t> nackList(length);
            const int32_t ret = NackList(&nackList[0], length);
            if (ret != VCM_OK && returnValue == VCM_OK)
            {
                returnValue = ret;
            }
            if (length > 0)
            {
                _packetRequestCallback->ResendPackets(&nackList[0], length);
            }
        }
    }

    
    if (_keyRequestTimer.TimeUntilProcess() == 0)
    {
        _keyRequestTimer.Processed();
        if (_scheduleKeyRequest && _frameTypeCallback != NULL)
        {
            const int32_t ret = RequestKeyFrame();
            if (ret != VCM_OK && returnValue == VCM_OK)
            {
                returnValue = ret;
            }
        }
    }

    return returnValue;
}

int32_t
VideoCodingModuleImpl::Id() const
{
    CriticalSectionScoped receiveCs(_receiveCritSect);
    {
        CriticalSectionScoped sendCs(_sendCritSect);
        return _id;
    }
}


int32_t
VideoCodingModuleImpl::ChangeUniqueId(const int32_t id)
{
    CriticalSectionScoped receiveCs(_receiveCritSect);
    {
        CriticalSectionScoped sendCs(_sendCritSect);
        _id = id;
        return VCM_OK;
    }
}



int32_t
VideoCodingModuleImpl::TimeUntilNextProcess()
{
    uint32_t timeUntilNextProcess = VCM_MIN(
                                    _receiveStatsTimer.TimeUntilProcess(),
                                    _sendStatsTimer.TimeUntilProcess());
    if ((_receiver.NackMode() != kNoNack) ||
        (_dualReceiver.State() != kPassive))
    {
        
        
        timeUntilNextProcess = VCM_MIN(timeUntilNextProcess,
                                       _retransmissionTimer.TimeUntilProcess());
    }
    timeUntilNextProcess = VCM_MIN(timeUntilNextProcess,
                                   _keyRequestTimer.TimeUntilProcess());

    return timeUntilNextProcess;
}


uint8_t
VideoCodingModule::NumberOfCodecs()
{
    return VCMCodecDataBase::NumberOfCodecs();
}


int32_t
VideoCodingModule::Codec(uint8_t listId, VideoCodec* codec)
{
    if (codec == NULL)
    {
        return VCM_PARAMETER_ERROR;
    }
    return VCMCodecDataBase::Codec(listId, codec) ? 0 : -1;
}


int32_t
VideoCodingModule::Codec(VideoCodecType codecType, VideoCodec* codec)
{
    if (codec == NULL)
    {
        return VCM_PARAMETER_ERROR;
    }
    return VCMCodecDataBase::Codec(codecType, codec) ? 0 : -1;
}






int32_t
VideoCodingModuleImpl::InitializeSender()
{
    CriticalSectionScoped cs(_sendCritSect);
    _codecDataBase.ResetSender();
    _encoder = NULL;
    _encodedFrameCallback.SetTransportCallback(NULL);
    
    _mediaOpt.SetEncodingData(kVideoCodecUnknown, 0, 0, 0, 0, 0, 0);
    _mediaOpt.Reset(); 
    return VCM_OK;
}


int32_t
VideoCodingModuleImpl::RegisterSendCodec(const VideoCodec* sendCodec,
                                         uint32_t numberOfCores,
                                         uint32_t maxPayloadSize)
{
    CriticalSectionScoped cs(_sendCritSect);
    if (sendCodec == NULL) {
        return VCM_PARAMETER_ERROR;
    }

    bool ret = _codecDataBase.SetSendCodec(sendCodec, numberOfCores,
                                           maxPayloadSize,
                                           &_encodedFrameCallback);
    if (!ret) {
        WEBRTC_TRACE(webrtc::kTraceError,
                     webrtc::kTraceVideoCoding,
                     VCMId(_id),
                     "Failed to initialize encoder");
        return VCM_CODEC_ERROR;
    }

    _encoder = _codecDataBase.GetEncoder();
    _sendCodecType = sendCodec->codecType;
    int numLayers = (_sendCodecType != kVideoCodecVP8) ? 1 :
                        sendCodec->codecSpecific.VP8.numberOfTemporalLayers;
    
    bool disable_frame_dropper =
        numLayers > 1 && sendCodec->mode == kScreensharing;
    if (disable_frame_dropper) {
      _mediaOpt.EnableFrameDropper(false);
    } else if (frame_dropper_enabled_) {
      _mediaOpt.EnableFrameDropper(true);
    }
    _nextFrameTypes.clear();
    _nextFrameTypes.resize(VCM_MAX(sendCodec->numberOfSimulcastStreams, 1),
                           kVideoFrameDelta);

    _mediaOpt.SetEncodingData(_sendCodecType,
                              sendCodec->maxBitrate * 1000,
                              sendCodec->maxFramerate * 1000,
                              sendCodec->startBitrate * 1000,
                              sendCodec->width,
                              sendCodec->height,
                              numLayers);
    _mediaOpt.SetMtu(maxPayloadSize);

    return VCM_OK;
}


int32_t
VideoCodingModuleImpl::SendCodec(VideoCodec* currentSendCodec) const
{
    CriticalSectionScoped cs(_sendCritSect);

    if (currentSendCodec == NULL)
    {
        return VCM_PARAMETER_ERROR;
    }
    return _codecDataBase.SendCodec(currentSendCodec) ? 0 : -1;
}


VideoCodecType
VideoCodingModuleImpl::SendCodec() const
{
    CriticalSectionScoped cs(_sendCritSect);

    return _codecDataBase.SendCodec();
}



int32_t
VideoCodingModuleImpl::RegisterExternalEncoder(VideoEncoder* externalEncoder,
                                               uint8_t payloadType,
                                               bool internalSource )
{
    CriticalSectionScoped cs(_sendCritSect);

    if (externalEncoder == NULL)
    {
        bool wasSendCodec = false;
        const bool ret = _codecDataBase.DeregisterExternalEncoder(
            payloadType, &wasSendCodec);
        if (wasSendCodec)
        {
            
            _encoder = NULL;
        }
        return ret ? 0 : -1;
    }
    _codecDataBase.RegisterExternalEncoder(externalEncoder, payloadType,
                                           internalSource);
    return 0;
}


int32_t
VideoCodingModuleImpl::CodecConfigParameters(uint8_t* buffer,
                                             int32_t size)
{
    CriticalSectionScoped cs(_sendCritSect);
    if (_encoder != NULL)
    {
        return _encoder->CodecConfigParameters(buffer, size);
    }
    return VCM_UNINITIALIZED;
}


int VideoCodingModuleImpl::Bitrate(unsigned int* bitrate) const
{
  CriticalSectionScoped cs(_sendCritSect);
  
  if (!_encoder) {
    return VCM_UNINITIALIZED;
  }
  *bitrate = _encoder->BitRate();
  return 0;
}


int VideoCodingModuleImpl::FrameRate(unsigned int* framerate) const
{
  CriticalSectionScoped cs(_sendCritSect);
  
  if (!_encoder) {
    return VCM_UNINITIALIZED;
  }
  *framerate = _encoder->FrameRate();
  return 0;
}


int32_t
VideoCodingModuleImpl::SetChannelParameters(uint32_t target_bitrate,
                                            uint8_t lossRate,
                                            uint32_t rtt)
{
    int32_t ret = 0;
    {
        CriticalSectionScoped sendCs(_sendCritSect);
        uint32_t targetRate = _mediaOpt.SetTargetRates(target_bitrate,
                                                             lossRate,
                                                             rtt);
        if (_encoder != NULL)
        {
            ret = _encoder->SetChannelParameters(lossRate, rtt);
            if (ret < 0 )
            {
                return ret;
            }
            ret = (int32_t)_encoder->SetRates(targetRate,
                                                    _mediaOpt.InputFrameRate());
            if (ret < 0)
            {
                return ret;
            }
        }
        else
        {
            return VCM_UNINITIALIZED;
        } 
    }
    return VCM_OK;
}

int32_t
VideoCodingModuleImpl::SetReceiveChannelParameters(uint32_t rtt)
{
    CriticalSectionScoped receiveCs(_receiveCritSect);
    _receiver.UpdateRtt(rtt);
    return 0;
}



int32_t
VideoCodingModuleImpl::RegisterTransportCallback(
    VCMPacketizationCallback* transport)
{
    CriticalSectionScoped cs(_sendCritSect);
    _encodedFrameCallback.SetMediaOpt(&_mediaOpt);
    _encodedFrameCallback.SetTransportCallback(transport);
    return VCM_OK;
}




int32_t
VideoCodingModuleImpl::RegisterSendStatisticsCallback(
    VCMSendStatisticsCallback* sendStats)
{
    CriticalSectionScoped cs(_sendCritSect);
    _sendStatsCallback = sendStats;
    return VCM_OK;
}



int32_t
VideoCodingModuleImpl::RegisterVideoQMCallback(
    VCMQMSettingsCallback* videoQMSettings)
{
    CriticalSectionScoped cs(_sendCritSect);
    return _mediaOpt.RegisterVideoQMCallback(videoQMSettings);
}




int32_t
VideoCodingModuleImpl::RegisterProtectionCallback(
    VCMProtectionCallback* protection)
{
    CriticalSectionScoped cs(_sendCritSect);
    _mediaOpt.RegisterProtectionCallback(protection);
    return VCM_OK;
}





int32_t
VideoCodingModuleImpl::SetVideoProtection(VCMVideoProtection videoProtection,
                                          bool enable)
{
    
    _receiver.SetDecodeWithErrors(false);
    
    _dualReceiver.SetDecodeWithErrors(false);
    switch (videoProtection)
    {

    case kProtectionNack:
        {
            
            SetVideoProtection(kProtectionNackSender, enable);
            SetVideoProtection(kProtectionNackReceiver, enable);
            break;
        }

    case kProtectionNackSender:
        {
            CriticalSectionScoped cs(_sendCritSect);
            _mediaOpt.EnableProtectionMethod(enable, media_optimization::kNack);
            break;
        }

    case kProtectionNackReceiver:
        {
            CriticalSectionScoped cs(_receiveCritSect);
            if (enable)
            {
              
                _receiver.SetNackMode(kNack, -1, -1);
            }
            else
            {
                _receiver.SetNackMode(kNoNack, -1, -1);
            }
            break;
        }

    case kProtectionDualDecoder:
        {
            CriticalSectionScoped cs(_receiveCritSect);
            if (enable)
            {
                
                
                _receiver.SetNackMode(kNack, 0, 0);
                
                
                _dualReceiver.SetNackMode(kNack, -1, -1);
                _receiver.SetDecodeWithErrors(true);
            }
            else
            {
                _dualReceiver.SetNackMode(kNoNack, -1, -1);
            }
            break;
        }

    case kProtectionKeyOnLoss:
        {
            CriticalSectionScoped cs(_receiveCritSect);
            if (enable)
            {
                _keyRequestMode = kKeyOnLoss;
                _receiver.SetDecodeWithErrors(true);
            }
            else if (_keyRequestMode == kKeyOnLoss)
            {
                _keyRequestMode = kKeyOnError; 
            }
            else
            {
                return VCM_PARAMETER_ERROR;
            }
            break;
        }

    case kProtectionKeyOnKeyLoss:
        {
            CriticalSectionScoped cs(_receiveCritSect);
            if (enable)
            {
                _keyRequestMode = kKeyOnKeyLoss;
            }
            else if (_keyRequestMode == kKeyOnKeyLoss)
            {
                _keyRequestMode = kKeyOnError; 
            }
            else
            {
                return VCM_PARAMETER_ERROR;
            }
            break;
        }

    case kProtectionNackFEC:
        {
            {
              
                CriticalSectionScoped cs(_receiveCritSect);
                if (enable)
                {
                    
                    
                    
                    _receiver.SetNackMode(kNack,
                                          media_optimization::kLowRttNackMs,
                                          -1);
                    _receiver.SetDecodeWithErrors(false);
                    _receiver.SetDecodeWithErrors(false);
                }
                else
                {
                    _receiver.SetNackMode(kNoNack, -1, -1);
                }
            }
            
            {
                CriticalSectionScoped cs(_sendCritSect);
                _mediaOpt.EnableProtectionMethod(enable,
                                                 media_optimization::kNackFec);
            }
            break;
        }

    case kProtectionFEC:
        {
            CriticalSectionScoped cs(_sendCritSect);
            _mediaOpt.EnableProtectionMethod(enable, media_optimization::kFec);
            break;
        }

    case kProtectionPeriodicKeyFrames:
        {
            CriticalSectionScoped cs(_sendCritSect);
            return _codecDataBase.SetPeriodicKeyFrames(enable) ? 0 : -1;
            break;
        }
    }
    return VCM_OK;
}


int32_t
VideoCodingModuleImpl::AddVideoFrame(const I420VideoFrame& videoFrame,
                                     const VideoContentMetrics* contentMetrics,
                                     const CodecSpecificInfo* codecSpecificInfo)
{
    CriticalSectionScoped cs(_sendCritSect);
    if (_encoder == NULL)
    {
        return VCM_UNINITIALIZED;
    }
    
    
    if (_nextFrameTypes[0] == kFrameEmpty)
    {
        return VCM_OK;
    }
    _mediaOpt.UpdateIncomingFrameRate();

    if (_mediaOpt.DropFrame())
    {
        WEBRTC_TRACE(webrtc::kTraceStream,
                     webrtc::kTraceVideoCoding,
                     VCMId(_id),
                     "Drop frame due to bitrate");
    }
    else
    {
        _mediaOpt.UpdateContentData(contentMetrics);
        int32_t ret = _encoder->Encode(videoFrame,
                                             codecSpecificInfo,
                                             _nextFrameTypes);
        if (_encoderInputFile != NULL)
        {
            if (PrintI420VideoFrame(videoFrame, _encoderInputFile) < 0)
            {
                return -1;
            }
        }
        if (ret < 0)
        {
            WEBRTC_TRACE(webrtc::kTraceError,
                         webrtc::kTraceVideoCoding,
                         VCMId(_id),
                         "Encode error: %d", ret);
            return ret;
        }
        for (size_t i = 0; i < _nextFrameTypes.size(); ++i) {
          _nextFrameTypes[i] = kVideoFrameDelta;  
        }
    }
    return VCM_OK;
}

int32_t VideoCodingModuleImpl::IntraFrameRequest(int stream_index) {
  CriticalSectionScoped cs(_sendCritSect);
  if (stream_index < 0 ||
      static_cast<unsigned int>(stream_index) >= _nextFrameTypes.size()) {
    return -1;
  }
  _nextFrameTypes[stream_index] = kVideoFrameKey;
  if (_encoder != NULL && _encoder->InternalSource()) {
    
    
    if (_encoder->RequestFrame(_nextFrameTypes) ==
        WEBRTC_VIDEO_CODEC_OK) {
      _nextFrameTypes[stream_index] = kVideoFrameDelta;
    }
  }
  return VCM_OK;
}

int32_t
VideoCodingModuleImpl::EnableFrameDropper(bool enable)
{
    CriticalSectionScoped cs(_sendCritSect);
    frame_dropper_enabled_ = enable;
    _mediaOpt.EnableFrameDropper(enable);
    return VCM_OK;
}


int32_t
VideoCodingModuleImpl::SentFrameCount(VCMFrameCount &frameCount) const
{
    CriticalSectionScoped cs(_sendCritSect);
    return _mediaOpt.SentFrameCount(frameCount);
}


int32_t
VideoCodingModuleImpl::InitializeReceiver()
{
    CriticalSectionScoped cs(_receiveCritSect);
    int32_t ret = _receiver.Initialize();
    if (ret < 0)
    {
        return ret;
    }

    ret = _dualReceiver.Initialize();
    if (ret < 0)
    {
        return ret;
    }
    _codecDataBase.ResetReceiver();
    _timing.Reset();

    _decoder = NULL;
    _decodedFrameCallback.SetUserReceiveCallback(NULL);
    _receiverInited = true;
    _frameTypeCallback = NULL;
    _frameStorageCallback = NULL;
    _receiveStatsCallback = NULL;
    _packetRequestCallback = NULL;
    _keyRequestMode = kKeyOnError;
    _scheduleKeyRequest = false;

    return VCM_OK;
}



int32_t
VideoCodingModuleImpl::RegisterReceiveCallback(
    VCMReceiveCallback* receiveCallback)
{
    CriticalSectionScoped cs(_receiveCritSect);
    _decodedFrameCallback.SetUserReceiveCallback(receiveCallback);
    return VCM_OK;
}

int32_t
VideoCodingModuleImpl::RegisterReceiveStatisticsCallback(
                                     VCMReceiveStatisticsCallback* receiveStats)
{
    CriticalSectionScoped cs(_receiveCritSect);
    _receiveStatsCallback = receiveStats;
    return VCM_OK;
}



int32_t
VideoCodingModuleImpl::RegisterExternalDecoder(VideoDecoder* externalDecoder,
                                               uint8_t payloadType,
                                               bool internalRenderTiming)
{
    CriticalSectionScoped cs(_receiveCritSect);
    if (externalDecoder == NULL)
    {
        
        _decoder = NULL;
        return _codecDataBase.DeregisterExternalDecoder(payloadType) ? 0 : -1;
    }
    return _codecDataBase.RegisterExternalDecoder(
        externalDecoder, payloadType, internalRenderTiming) ? 0 : -1;
}


int32_t
VideoCodingModuleImpl::RegisterFrameTypeCallback(
    VCMFrameTypeCallback* frameTypeCallback)
{
    CriticalSectionScoped cs(_receiveCritSect);
    _frameTypeCallback = frameTypeCallback;
    return VCM_OK;
}

int32_t
VideoCodingModuleImpl::RegisterFrameStorageCallback(
    VCMFrameStorageCallback* frameStorageCallback)
{
    CriticalSectionScoped cs(_receiveCritSect);
    _frameStorageCallback = frameStorageCallback;
    return VCM_OK;
}

int32_t
VideoCodingModuleImpl::RegisterPacketRequestCallback(
    VCMPacketRequestCallback* callback)
{
    CriticalSectionScoped cs(_receiveCritSect);
    _packetRequestCallback = callback;
    return VCM_OK;
}

int VideoCodingModuleImpl::RegisterRenderBufferSizeCallback(
  VCMRenderBufferSizeCallback* callback) {
  CriticalSectionScoped cs(_receiveCritSect);
  render_buffer_callback_ = callback;
  return VCM_OK;
}



int32_t
VideoCodingModuleImpl::Decode(uint16_t maxWaitTimeMs)
{
    TRACE_EVENT1("webrtc", "VCM::Decode", "max_wait", maxWaitTimeMs);
    int64_t nextRenderTimeMs;
    {
        CriticalSectionScoped cs(_receiveCritSect);
        if (!_receiverInited)
        {
            return VCM_UNINITIALIZED;
        }
        if (!_codecDataBase.DecoderRegistered())
        {
            return VCM_NO_CODEC_REGISTERED;
        }
    }

    const bool dualReceiverEnabledNotReceiving =
        (_dualReceiver.State() != kReceiving &&
         _dualReceiver.NackMode() == kNack);

    VCMEncodedFrame* frame = _receiver.FrameForDecoding(
        maxWaitTimeMs,
        nextRenderTimeMs,
        _codecDataBase.SupportsRenderScheduling(),
        &_dualReceiver);

    if (dualReceiverEnabledNotReceiving && _dualReceiver.State() == kReceiving)
    {
        
        
        
        
        
        CriticalSectionScoped cs(_receiveCritSect);
        if (_dualDecoder != NULL)
        {
            _codecDataBase.ReleaseDecoder(_dualDecoder);
        }
        _dualDecoder = _codecDataBase.CreateDecoderCopy();
        if (_dualDecoder != NULL)
        {
            _dualDecoder->RegisterDecodeCompleteCallback(
                &_dualDecodedFrameCallback);
        }
        else
        {
            _dualReceiver.Reset();
        }
    }

    if (frame == NULL)
      return VCM_FRAME_NOT_READY;
    else
    {
        CriticalSectionScoped cs(_receiveCritSect);

        
        _timing.UpdateCurrentDelay(frame->RenderTimeMs(),
                                   clock_->TimeInMilliseconds());

#ifdef DEBUG_DECODER_BIT_STREAM
        if (_bitStreamBeforeDecoder != NULL)
        {
          
          if (fwrite(frame->Buffer(), 1, frame->Length(),
                     _bitStreamBeforeDecoder) !=  frame->Length()) {
            return -1;
          }
        }
#endif
        if (_frameStorageCallback != NULL)
        {
            int32_t ret = frame->Store(*_frameStorageCallback);
            if (ret < 0)
            {
                return ret;
            }
        }

        const int32_t ret = Decode(*frame);
        _receiver.ReleaseFrame(frame);
        frame = NULL;
        if (ret != VCM_OK)
        {
            return ret;
        }
    }
    return VCM_OK;
}

int32_t
VideoCodingModuleImpl::RequestSliceLossIndication(
    const uint64_t pictureID) const
{
    TRACE_EVENT1("webrtc", "RequestSLI", "picture_id", pictureID);
    if (_frameTypeCallback != NULL)
    {
        const int32_t ret =
            _frameTypeCallback->SliceLossIndicationRequest(pictureID);
        if (ret < 0)
        {
            WEBRTC_TRACE(webrtc::kTraceError,
                         webrtc::kTraceVideoCoding,
                         VCMId(_id),
                         "Failed to request key frame");
            return ret;
        }
    } else
    {
        WEBRTC_TRACE(webrtc::kTraceWarning,
                     webrtc::kTraceVideoCoding,
                     VCMId(_id),
                     "No frame type request callback registered");
        return VCM_MISSING_CALLBACK;
    }
    return VCM_OK;
}

int32_t
VideoCodingModuleImpl::RequestKeyFrame()
{
    TRACE_EVENT0("webrtc", "RequestKeyFrame");
    if (_frameTypeCallback != NULL)
    {
        const int32_t ret = _frameTypeCallback->RequestKeyFrame();
        if (ret < 0)
        {
            WEBRTC_TRACE(webrtc::kTraceError,
                         webrtc::kTraceVideoCoding,
                         VCMId(_id),
                         "Failed to request key frame");
            return ret;
        }
        _scheduleKeyRequest = false;
    }
    else
    {
        WEBRTC_TRACE(webrtc::kTraceWarning,
                     webrtc::kTraceVideoCoding,
                     VCMId(_id),
                     "No frame type request callback registered");
        return VCM_MISSING_CALLBACK;
    }
    return VCM_OK;
}

int32_t
VideoCodingModuleImpl::DecodeDualFrame(uint16_t maxWaitTimeMs)
{
    CriticalSectionScoped cs(_receiveCritSect);
    if (_dualReceiver.State() != kReceiving ||
        _dualReceiver.NackMode() != kNack)
    {
        
        
        return VCM_OK;
    }
    int64_t dummyRenderTime;
    int32_t decodeCount = 0;
    
    
    
    _dualReceiver.SetDecodeWithErrors(false);
    VCMEncodedFrame* dualFrame = _dualReceiver.FrameForDecoding(
                                                            maxWaitTimeMs,
                                                            dummyRenderTime);
    if (dualFrame != NULL && _dualDecoder != NULL)
    {
        WEBRTC_TRACE(webrtc::kTraceStream,
                     webrtc::kTraceVideoCoding,
                     VCMId(_id),
                     "Decoding frame %u with dual decoder",
                     dualFrame->TimeStamp());
        
        int32_t ret = _dualDecoder->Decode(*dualFrame,
                                                 clock_->TimeInMilliseconds());
        if (ret != WEBRTC_VIDEO_CODEC_OK)
        {
            WEBRTC_TRACE(webrtc::kTraceWarning,
                         webrtc::kTraceVideoCoding,
                         VCMId(_id),
                         "Failed to decode frame with dual decoder");
            _dualReceiver.ReleaseFrame(dualFrame);
            return VCM_CODEC_ERROR;
        }
        if (_receiver.DualDecoderCaughtUp(dualFrame, _dualReceiver))
        {
            
            
            WEBRTC_TRACE(webrtc::kTraceStream,
                         webrtc::kTraceVideoCoding,
                         VCMId(_id),
                         "Dual decoder caught up");
            _codecDataBase.CopyDecoder(*_dualDecoder);
            _codecDataBase.ReleaseDecoder(_dualDecoder);
            _dualDecoder = NULL;
        }
        decodeCount++;
    }
    _dualReceiver.ReleaseFrame(dualFrame);
    return decodeCount;
}



int32_t
VideoCodingModuleImpl::Decode(const VCMEncodedFrame& frame)
{
    TRACE_EVENT2("webrtc", "Decode",
                 "timestamp", frame.TimeStamp(),
                 "type", frame.FrameType());
    
    const bool renderTimingBefore = _codecDataBase.SupportsRenderScheduling();
    _decoder = _codecDataBase.GetDecoder(frame.PayloadType(),
                                         &_decodedFrameCallback);
    if (renderTimingBefore != _codecDataBase.SupportsRenderScheduling())
    {
        
        
        _timing.ResetDecodeTime();
    }
    if (_decoder == NULL)
    {
        return VCM_NO_CODEC_REGISTERED;
    }
    
    int32_t ret = _decoder->Decode(frame, clock_->TimeInMilliseconds());

    
    if (ret < 0)
    {
        if (ret == VCM_ERROR_REQUEST_SLI)
        {
            return RequestSliceLossIndication(
                    _decodedFrameCallback.LastReceivedPictureID() + 1);
        }
        else
        {
            WEBRTC_TRACE(webrtc::kTraceError,
                         webrtc::kTraceVideoCoding,
                         VCMId(_id),
                         "Failed to decode frame %u, requesting key frame",
                         frame.TimeStamp());
            ret = RequestKeyFrame();
        }
    }
    else if (ret == VCM_REQUEST_SLI)
    {
        ret = RequestSliceLossIndication(
            _decodedFrameCallback.LastReceivedPictureID() + 1);
    }
    if (!frame.Complete() || frame.MissingFrame())
    {
        switch (_keyRequestMode)
        {
            case kKeyOnKeyLoss:
            {
                if (frame.FrameType() == kVideoFrameKey)
                {
                    _scheduleKeyRequest = true;
                    return VCM_OK;
                }
                break;
            }
            case kKeyOnLoss:
            {
                _scheduleKeyRequest = true;
                return VCM_OK;
            }
            default:
                break;
        }
    }
    return ret;
}

int32_t
VideoCodingModuleImpl::DecodeFromStorage(
    const EncodedVideoData& frameFromStorage)
{
    CriticalSectionScoped cs(_receiveCritSect);
    int32_t ret = _frameFromFile.ExtractFromStorage(frameFromStorage);
    if (ret < 0)
    {
        return ret;
    }
    return Decode(_frameFromFile);
}


int32_t
VideoCodingModuleImpl::ResetDecoder()
{
    CriticalSectionScoped cs(_receiveCritSect);
    if (_decoder != NULL)
    {
        _receiver.Initialize();
        _timing.Reset();
        _scheduleKeyRequest = false;
        _decoder->Reset();
    }
    if (_dualReceiver.State() != kPassive)
    {
        _dualReceiver.Initialize();
    }
    if (_dualDecoder != NULL)
    {
        _codecDataBase.ReleaseDecoder(_dualDecoder);
        _dualDecoder = NULL;
    }
    return VCM_OK;
}


int32_t
VideoCodingModuleImpl::RegisterReceiveCodec(const VideoCodec* receiveCodec,
                                                int32_t numberOfCores,
                                                bool requireKeyFrame)
{
    CriticalSectionScoped cs(_receiveCritSect);
    if (receiveCodec == NULL)
    {
        return VCM_PARAMETER_ERROR;
    }
    if (!_codecDataBase.RegisterReceiveCodec(receiveCodec, numberOfCores,
                                             requireKeyFrame)) {
      return -1;
    }
    return 0;
}


int32_t
VideoCodingModuleImpl::ReceiveCodec(VideoCodec* currentReceiveCodec) const
{
    CriticalSectionScoped cs(_receiveCritSect);
    if (currentReceiveCodec == NULL)
    {
        return VCM_PARAMETER_ERROR;
    }
    return _codecDataBase.ReceiveCodec(currentReceiveCodec) ? 0 : -1;
}


VideoCodecType
VideoCodingModuleImpl::ReceiveCodec() const
{
    CriticalSectionScoped cs(_receiveCritSect);
    return _codecDataBase.ReceiveCodec();
}


int32_t
VideoCodingModuleImpl::IncomingPacket(const uint8_t* incomingPayload,
                                    uint32_t payloadLength,
                                    const WebRtcRTPHeader& rtpInfo)
{
    if (rtpInfo.frameType == kVideoFrameKey) {
      TRACE_EVENT1("webrtc", "VCM::PacketKeyFrame",
                   "seqnum", rtpInfo.header.sequenceNumber);
    } else {
      TRACE_EVENT2("webrtc", "VCM::Packet",
                   "seqnum", rtpInfo.header.sequenceNumber,
                   "type", rtpInfo.frameType);
    }
    if (incomingPayload == NULL) {
      
      
      
      payloadLength = 0;
    }
    const VCMPacket packet(incomingPayload, payloadLength, rtpInfo);
    int32_t ret;
    if (_dualReceiver.State() != kPassive)
    {
        ret = _dualReceiver.InsertPacket(packet,
                                         rtpInfo.type.Video.width,
                                         rtpInfo.type.Video.height);
        if (ret == VCM_FLUSH_INDICATOR) {
          RequestKeyFrame();
          ResetDecoder();
        } else if (ret < 0) {
          return ret;
        }
    }
    ret = _receiver.InsertPacket(packet,
                                 rtpInfo.type.Video.width,
                                 rtpInfo.type.Video.height);
    
    
    if (ret == VCM_FLUSH_INDICATOR) {
      RequestKeyFrame();
      ResetDecoder();
    } else if (ret < 0) {
      return ret;
    }
    return VCM_OK;
}




int32_t
VideoCodingModuleImpl::SetMinimumPlayoutDelay(uint32_t minPlayoutDelayMs)
{
    _timing.set_min_playout_delay(minPlayoutDelayMs);
    return VCM_OK;
}



int32_t
VideoCodingModuleImpl::SetRenderDelay(uint32_t timeMS)
{
    _timing.set_render_delay(timeMS);
    return VCM_OK;
}


int32_t
VideoCodingModuleImpl::Delay() const
{
    return _timing.TargetVideoDelay();
}


int32_t
VideoCodingModuleImpl::NackList(uint16_t* nackList, uint16_t& size)
{
    VCMNackStatus nackStatus = kNackOk;
    uint16_t nack_list_length = 0;
    
    
    
    if (_receiver.NackMode() != kNoNack)
    {
        nackStatus = _receiver.NackList(nackList, size, &nack_list_length);
    }
    if (nack_list_length == 0 && _dualReceiver.State() != kPassive)
    {
        nackStatus = _dualReceiver.NackList(nackList, size, &nack_list_length);
    }
    size = nack_list_length;

    switch (nackStatus)
    {
    case kNackNeedMoreMemory:
        {
            WEBRTC_TRACE(webrtc::kTraceError,
                         webrtc::kTraceVideoCoding,
                         VCMId(_id),
                         "Out of memory");
            return VCM_MEMORY;
        }
    case kNackKeyFrameRequest:
        {
            CriticalSectionScoped cs(_receiveCritSect);
            WEBRTC_TRACE(webrtc::kTraceWarning,
                         webrtc::kTraceVideoCoding,
                         VCMId(_id),
                         "Failed to get NACK list, requesting key frame");
            return RequestKeyFrame();
        }
    default:
        break;
    }
    return VCM_OK;
}

int32_t
VideoCodingModuleImpl::ReceivedFrameCount(VCMFrameCount& frameCount) const
{
    _receiver.ReceivedFrameCount(&frameCount);
    return VCM_OK;
}

uint32_t VideoCodingModuleImpl::DiscardedPackets() const {
  return _receiver.DiscardedPackets();
}

int VideoCodingModuleImpl::SetSenderNackMode(SenderNackMode mode) {
  CriticalSectionScoped cs(_sendCritSect);

  switch (mode) {
    case kNackNone:
      _mediaOpt.EnableProtectionMethod(false, media_optimization::kNack);
      break;
    case kNackAll:
      _mediaOpt.EnableProtectionMethod(true, media_optimization::kNack);
      break;
    case kNackSelective:
      return VCM_NOT_IMPLEMENTED;
      break;
  }
  return VCM_OK;
}

int VideoCodingModuleImpl::SetSenderReferenceSelection(bool enable) {
  return VCM_NOT_IMPLEMENTED;
}

int VideoCodingModuleImpl::SetSenderFEC(bool enable) {
  CriticalSectionScoped cs(_sendCritSect);
  _mediaOpt.EnableProtectionMethod(enable, media_optimization::kFec);
  return VCM_OK;
}

int VideoCodingModuleImpl::SetSenderKeyFramePeriod(int periodMs) {
  return VCM_NOT_IMPLEMENTED;
}

int VideoCodingModuleImpl::SetReceiverRobustnessMode(
    ReceiverRobustness robustnessMode,
    DecodeErrors errorMode) {
  CriticalSectionScoped cs(_receiveCritSect);
  switch (robustnessMode) {
    case kNone:
      _receiver.SetNackMode(kNoNack, -1, -1);
      _dualReceiver.SetNackMode(kNoNack, -1, -1);
      if (errorMode == kNoDecodeErrors) {
        _keyRequestMode = kKeyOnLoss;
      } else {
        _keyRequestMode = kKeyOnError;
      }
      break;
    case kHardNack:
      if (errorMode == kAllowDecodeErrors) {
        return VCM_PARAMETER_ERROR;
      }
      
      _receiver.SetNackMode(kNack, -1, -1);
      _dualReceiver.SetNackMode(kNoNack, -1, -1);
      _keyRequestMode = kKeyOnError;  
      break;
    case kSoftNack:
      assert(false); 
      return VCM_NOT_IMPLEMENTED;
      
      
      _receiver.SetNackMode(kNack, media_optimization::kLowRttNackMs, -1);
      _dualReceiver.SetNackMode(kNoNack, -1, -1);
      _keyRequestMode = kKeyOnError;
      break;
    case kDualDecoder:
      if (errorMode == kNoDecodeErrors) {
        return VCM_PARAMETER_ERROR;
      }
      
      
      _receiver.SetNackMode(kNack, 0, 0);
      
      _dualReceiver.SetNackMode(kNack, -1, -1);
      _keyRequestMode = kKeyOnError;
      break;
    case kReferenceSelection:
      assert(false); 
      return VCM_NOT_IMPLEMENTED;
      if (errorMode == kNoDecodeErrors) {
        return VCM_PARAMETER_ERROR;
      }
      _receiver.SetNackMode(kNoNack, -1, -1);
      _dualReceiver.SetNackMode(kNoNack, -1, -1);
      break;
  }
  _receiver.SetDecodeWithErrors(errorMode == kAllowDecodeErrors);
  
  _dualReceiver.SetDecodeWithErrors(false);
  return VCM_OK;
}

void VideoCodingModuleImpl::SetNackSettings(size_t max_nack_list_size,
                                            int max_packet_age_to_nack,
                                            int max_incomplete_time_ms) {
  if (max_nack_list_size != 0) {
    CriticalSectionScoped cs(_receiveCritSect);
    max_nack_list_size_ = max_nack_list_size;
  }
  _receiver.SetNackSettings(max_nack_list_size, max_packet_age_to_nack,
                            max_incomplete_time_ms);
  _dualReceiver.SetNackSettings(max_nack_list_size, max_packet_age_to_nack,
                                max_incomplete_time_ms);
}

int VideoCodingModuleImpl::SetMinReceiverDelay(int desired_delay_ms) {
  return _receiver.SetMinReceiverDelay(desired_delay_ms);
}

int VideoCodingModuleImpl::StartDebugRecording(const char* file_name_utf8) {
  CriticalSectionScoped cs(_sendCritSect);
  _encoderInputFile = fopen(file_name_utf8, "wb");
  if (_encoderInputFile == NULL)
    return VCM_GENERAL_ERROR;
  return VCM_OK;
}

int VideoCodingModuleImpl::StopDebugRecording(){
  CriticalSectionScoped cs(_sendCritSect);
  if (_encoderInputFile != NULL) {
    fclose(_encoderInputFile);
    _encoderInputFile = NULL;
  }
  return VCM_OK;
}

}  
