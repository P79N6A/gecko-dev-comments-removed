



#include "CSFLog.h"
#include "nspr.h"

#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#elif defined XP_WIN
#include <winsock2.h>
#endif

#include "AudioConduit.h"
#include "nsCOMPtr.h"
#include "mozilla/Services.h"
#include "nsServiceManagerUtils.h"
#include "nsIPrefService.h"
#include "nsIPrefBranch.h"
#include "nsThreadUtils.h"
#if !defined(MOZILLA_EXTERNAL_LINKAGE)
#include "Latency.h"
#include "mozilla/Telemetry.h"
#endif

#include "webrtc/voice_engine/include/voe_errors.h"
#include "webrtc/system_wrappers/interface/clock.h"
#include "browser_logging/WebRtcLog.h"

#ifdef MOZ_WIDGET_ANDROID
#include "AndroidJNIWrapper.h"
#endif

namespace mozilla {

static const char* logTag ="WebrtcAudioSessionConduit";


const unsigned int WebrtcAudioConduit::CODEC_PLNAME_SIZE = 32;




mozilla::RefPtr<AudioSessionConduit> AudioSessionConduit::Create()
{
  CSFLogDebug(logTag,  "%s ", __FUNCTION__);
  NS_ASSERTION(NS_IsMainThread(), "Only call on main thread");

  WebrtcAudioConduit* obj = new WebrtcAudioConduit();
  if(obj->Init() != kMediaConduitNoError)
  {
    CSFLogError(logTag,  "%s AudioConduit Init Failed ", __FUNCTION__);
    delete obj;
    return nullptr;
  }
  CSFLogDebug(logTag,  "%s Successfully created AudioConduit ", __FUNCTION__);
  return obj;
}




WebrtcAudioConduit::~WebrtcAudioConduit()
{
  NS_ASSERTION(NS_IsMainThread(), "Only call on main thread");

  CSFLogDebug(logTag,  "%s ", __FUNCTION__);
  for(std::vector<AudioCodecConfig*>::size_type i=0;i < mRecvCodecList.size();i++)
  {
    delete mRecvCodecList[i];
  }
  delete mCurSendCodecConfig;

  
  if(mPtrVoEXmedia)
  {
    mPtrVoEXmedia->SetExternalRecordingStatus(false);
    mPtrVoEXmedia->SetExternalPlayoutStatus(false);
  }

  
  if(mPtrVoENetwork)
  {
    mPtrVoENetwork->DeRegisterExternalTransport(mChannel);
  }

  if(mPtrVoEBase)
  {
    mPtrVoEBase->StopPlayout(mChannel);
    mPtrVoEBase->StopSend(mChannel);
    mPtrVoEBase->StopReceive(mChannel);
    mPtrVoEBase->DeleteChannel(mChannel);
    mPtrVoEBase->Terminate();
  }

  
  
  mPtrVoENetwork = nullptr;
  mPtrVoEBase = nullptr;
  mPtrVoECodec = nullptr;
  mPtrVoEXmedia = nullptr;
  mPtrVoEProcessing = nullptr;
  mPtrVoEVideoSync = nullptr;
  mPtrVoERTP_RTCP = nullptr;
  mPtrRTP = nullptr;

  if(mVoiceEngine)
  {
    webrtc::VoiceEngine::Delete(mVoiceEngine);
  }
}

bool WebrtcAudioConduit::SetLocalSSRC(unsigned int ssrc)
{
  return !mPtrRTP->SetLocalSSRC(mChannel, ssrc);
}

bool WebrtcAudioConduit::GetLocalSSRC(unsigned int* ssrc) {
  return !mPtrRTP->GetLocalSSRC(mChannel, *ssrc);
}

bool WebrtcAudioConduit::GetRemoteSSRC(unsigned int* ssrc) {
  return !mPtrRTP->GetRemoteSSRC(mChannel, *ssrc);
}

bool WebrtcAudioConduit::SetLocalCNAME(const char* cname)
{
  char temp[256];
  strncpy(temp, cname, sizeof(temp) - 1);
  temp[sizeof(temp) - 1] = 0;
  return !mPtrRTP->SetRTCP_CNAME(mChannel, temp);
}

bool WebrtcAudioConduit::GetAVStats(int32_t* jitterBufferDelayMs,
                                    int32_t* playoutBufferDelayMs,
                                    int32_t* avSyncOffsetMs) {
  return !mPtrVoEVideoSync->GetDelayEstimate(mChannel,
                                             jitterBufferDelayMs,
                                             playoutBufferDelayMs,
                                             avSyncOffsetMs);
}

bool WebrtcAudioConduit::GetRTPStats(unsigned int* jitterMs,
                                     unsigned int* cumulativeLost) {
  unsigned int maxJitterMs = 0;
  unsigned int discardedPackets;
  *jitterMs = 0;
  *cumulativeLost = 0;
  return !mPtrRTP->GetRTPStatistics(mChannel, *jitterMs, maxJitterMs,
                                    discardedPackets, *cumulativeLost);
}

DOMHighResTimeStamp
NTPtoDOMHighResTimeStamp(uint32_t ntpHigh, uint32_t ntpLow) {
  return (uint32_t(ntpHigh - webrtc::kNtpJan1970) +
          double(ntpLow) / webrtc::kMagicNtpFractionalUnit) * 1000;
}

bool WebrtcAudioConduit::GetRTCPReceiverReport(DOMHighResTimeStamp* timestamp,
                                               uint32_t* jitterMs,
                                               uint32_t* packetsReceived,
                                               uint64_t* bytesReceived,
                                               uint32_t* cumulativeLost,
                                               int32_t* rttMs) {
  uint32_t ntpHigh, ntpLow;
  uint16_t fractionLost;
  bool result = !mPtrRTP->GetRemoteRTCPReceiverInfo(mChannel, ntpHigh, ntpLow,
                                                    *packetsReceived,
                                                    *bytesReceived,
                                                    *jitterMs,
                                                    fractionLost,
                                                    *cumulativeLost,
                                                    *rttMs);
  if (result) {
    *timestamp = NTPtoDOMHighResTimeStamp(ntpHigh, ntpLow);
  }
  return result;
}

bool WebrtcAudioConduit::GetRTCPSenderReport(DOMHighResTimeStamp* timestamp,
                                             unsigned int* packetsSent,
                                             uint64_t* bytesSent) {
  struct webrtc::SenderInfo senderInfo;
  bool result = !mPtrRTP->GetRemoteRTCPSenderInfo(mChannel, &senderInfo);
  if (result) {
    *timestamp = NTPtoDOMHighResTimeStamp(senderInfo.NTP_timestamp_high,
                                          senderInfo.NTP_timestamp_low);
    *packetsSent = senderInfo.sender_packet_count;
    *bytesSent = senderInfo.sender_octet_count;
  }
  return result;
}




MediaConduitErrorCode WebrtcAudioConduit::Init()
{
  CSFLogDebug(logTag,  "%s this=%p", __FUNCTION__, this);

#ifdef MOZ_WIDGET_ANDROID
    jobject context = jsjni_GetGlobalContextRef();

    
    JavaVM *jvm = jsjni_GetVM();
    JNIEnv* jenv = jsjni_GetJNIForThread();

    if (webrtc::VoiceEngine::SetAndroidObjects(jvm, jenv, (void*)context) != 0) {
      CSFLogError(logTag, "%s Unable to set Android objects", __FUNCTION__);
      return kMediaConduitSessionNotInited;
    }
#endif

  
  if(!(mVoiceEngine = webrtc::VoiceEngine::Create()))
  {
    CSFLogError(logTag, "%s Unable to create voice engine", __FUNCTION__);
    return kMediaConduitSessionNotInited;
  }

  EnableWebRtcLog();

  if(!(mPtrVoEBase = VoEBase::GetInterface(mVoiceEngine)))
  {
    CSFLogError(logTag, "%s Unable to initialize VoEBase", __FUNCTION__);
    return kMediaConduitSessionNotInited;
  }

  if(!(mPtrVoENetwork = VoENetwork::GetInterface(mVoiceEngine)))
  {
    CSFLogError(logTag, "%s Unable to initialize VoENetwork", __FUNCTION__);
    return kMediaConduitSessionNotInited;
  }

  if(!(mPtrVoECodec = VoECodec::GetInterface(mVoiceEngine)))
  {
    CSFLogError(logTag, "%s Unable to initialize VoEBCodec", __FUNCTION__);
    return kMediaConduitSessionNotInited;
  }

  if(!(mPtrVoEProcessing = VoEAudioProcessing::GetInterface(mVoiceEngine)))
  {
    CSFLogError(logTag, "%s Unable to initialize VoEProcessing", __FUNCTION__);
    return kMediaConduitSessionNotInited;
  }
  if(!(mPtrVoEXmedia = VoEExternalMedia::GetInterface(mVoiceEngine)))
  {
    CSFLogError(logTag, "%s Unable to initialize VoEExternalMedia", __FUNCTION__);
    return kMediaConduitSessionNotInited;
  }
  if(!(mPtrVoERTP_RTCP = VoERTP_RTCP::GetInterface(mVoiceEngine)))
  {
    CSFLogError(logTag, "%s Unable to initialize VoERTP_RTCP", __FUNCTION__);
    return kMediaConduitSessionNotInited;
  }

  if(!(mPtrVoEVideoSync = VoEVideoSync::GetInterface(mVoiceEngine)))
  {
    CSFLogError(logTag, "%s Unable to initialize VoEVideoSync", __FUNCTION__);
    return kMediaConduitSessionNotInited;
  }
  if (!(mPtrRTP = webrtc::VoERTP_RTCP::GetInterface(mVoiceEngine)))
  {
    CSFLogError(logTag, "%s Unable to get audio RTP/RTCP interface ",
                __FUNCTION__);
    return kMediaConduitSessionNotInited;
  }

  
  if(mPtrVoEBase->Init() == -1)
  {
    CSFLogError(logTag, "%s VoiceEngine Base Not Initialized", __FUNCTION__);
    return kMediaConduitSessionNotInited;
  }

  if( (mChannel = mPtrVoEBase->CreateChannel()) == -1)
  {
    CSFLogError(logTag, "%s VoiceEngine Channel creation failed",__FUNCTION__);
    return kMediaConduitChannelError;
  }

  CSFLogDebug(logTag, "%s Channel Created %d ",__FUNCTION__, mChannel);

  if(mPtrVoENetwork->RegisterExternalTransport(mChannel, *this) == -1)
  {
    CSFLogError(logTag, "%s VoiceEngine, External Transport Failed",__FUNCTION__);
    return kMediaConduitTransportRegistrationFail;
  }

  if(mPtrVoEXmedia->SetExternalRecordingStatus(true) == -1)
  {
    CSFLogError(logTag, "%s SetExternalRecordingStatus Failed %d",__FUNCTION__,
                mPtrVoEBase->LastError());
    return kMediaConduitExternalPlayoutError;
  }

  if(mPtrVoEXmedia->SetExternalPlayoutStatus(true) == -1)
  {
    CSFLogError(logTag, "%s SetExternalPlayoutStatus Failed %d ",__FUNCTION__,
                mPtrVoEBase->LastError());
    return kMediaConduitExternalRecordingError;
  }

  CSFLogDebug(logTag ,  "%s AudioSessionConduit Initialization Done (%p)",__FUNCTION__, this);
  return kMediaConduitNoError;
}


MediaConduitErrorCode
WebrtcAudioConduit::SetTransmitterTransport(mozilla::RefPtr<TransportInterface> aTransport)
{
  CSFLogDebug(logTag,  "%s ", __FUNCTION__);

  ReentrantMonitorAutoEnter enter(mTransportMonitor);
  
  mTransmitterTransport = aTransport;
  return kMediaConduitNoError;
}

MediaConduitErrorCode
WebrtcAudioConduit::SetReceiverTransport(mozilla::RefPtr<TransportInterface> aTransport)
{
  CSFLogDebug(logTag,  "%s ", __FUNCTION__);

  ReentrantMonitorAutoEnter enter(mTransportMonitor);
  
  mReceiverTransport = aTransport;
  return kMediaConduitNoError;
}

MediaConduitErrorCode
WebrtcAudioConduit::ConfigureSendMediaCodec(const AudioCodecConfig* codecConfig)
{
  CSFLogDebug(logTag,  "%s ", __FUNCTION__);
  MediaConduitErrorCode condError = kMediaConduitNoError;
  int error = 0;
  webrtc::CodecInst cinst;

  
  if((condError = ValidateCodecConfig(codecConfig, true)) != kMediaConduitNoError)
  {
    return condError;
  }

  condError = StopTransmitting();
  if (condError != kMediaConduitNoError) {
    return condError;
  }

  if(!CodecConfigToWebRTCCodec(codecConfig,cinst))
  {
    CSFLogError(logTag,"%s CodecConfig to WebRTC Codec Failed ",__FUNCTION__);
    return kMediaConduitMalformedArgument;
  }

  if(mPtrVoECodec->SetSendCodec(mChannel, cinst) == -1)
  {
    error = mPtrVoEBase->LastError();
    CSFLogError(logTag, "%s SetSendCodec - Invalid Codec %d ",__FUNCTION__,
                                                                    error);

    if(error ==  VE_CANNOT_SET_SEND_CODEC || error == VE_CODEC_ERROR)
    {
      CSFLogError(logTag, "%s Invalid Send Codec", __FUNCTION__);
      return kMediaConduitInvalidSendCodec;
    }
    CSFLogError(logTag, "%s SetSendCodec Failed %d ", __FUNCTION__,
                                         mPtrVoEBase->LastError());
    return kMediaConduitUnknownError;
  }

#if !defined(MOZILLA_EXTERNAL_LINKAGE)
  
  nsresult rv;
  nsCOMPtr<nsIPrefService> prefs = do_GetService("@mozilla.org/preferences-service;1", &rv);
  if (NS_SUCCEEDED(rv)) {
    nsCOMPtr<nsIPrefBranch> branch = do_QueryInterface(prefs);

    if (branch) {
      branch->GetIntPref("media.peerconnection.capture_delay", &mCaptureDelay);
    }
  }
#endif

  condError = StartTransmitting();
  if (condError != kMediaConduitNoError) {
    return condError;
  }

  
  delete mCurSendCodecConfig;

  mCurSendCodecConfig = new AudioCodecConfig(codecConfig->mType,
                                              codecConfig->mName,
                                              codecConfig->mFreq,
                                              codecConfig->mPacSize,
                                              codecConfig->mChannels,
                                              codecConfig->mRate);

  return kMediaConduitNoError;
}

MediaConduitErrorCode
WebrtcAudioConduit::ConfigureRecvMediaCodecs(
                    const std::vector<AudioCodecConfig*>& codecConfigList)
{
  CSFLogDebug(logTag,  "%s ", __FUNCTION__);
  MediaConduitErrorCode condError = kMediaConduitNoError;
  int error = 0; 
  bool success = false;

  
  
  condError = StopReceiving();
  if (condError != kMediaConduitNoError) {
    return condError;
  }

  if(codecConfigList.empty())
  {
    CSFLogError(logTag, "%s Zero number of codecs to configure", __FUNCTION__);
    return kMediaConduitMalformedArgument;
  }

  
  
  
  for(std::vector<AudioCodecConfig*>::size_type i=0 ;i<codecConfigList.size();i++)
  {
    
    if((condError = ValidateCodecConfig(codecConfigList[i],false)) != kMediaConduitNoError)
    {
      return condError;
    }

    webrtc::CodecInst cinst;
    if(!CodecConfigToWebRTCCodec(codecConfigList[i],cinst))
    {
      CSFLogError(logTag,"%s CodecConfig to WebRTC Codec Failed ",__FUNCTION__);
      continue;
    }

    if(mPtrVoECodec->SetRecPayloadType(mChannel,cinst) == -1)
    {
      error = mPtrVoEBase->LastError();
      CSFLogError(logTag,  "%s SetRecvCodec Failed %d ",__FUNCTION__, error);
      continue;
    } else {
      CSFLogDebug(logTag, "%s Successfully Set RecvCodec %s", __FUNCTION__,
                                          codecConfigList[i]->mName.c_str());
      
      if(CopyCodecToDB(codecConfigList[i]))
      {
        success = true;
      } else {
        CSFLogError(logTag,"%s Unable to updated Codec Database", __FUNCTION__);
        return kMediaConduitUnknownError;
      }

    }

  } 

  if(!success)
  {
    CSFLogError(logTag, "%s Setting Receive Codec Failed ", __FUNCTION__);
    return kMediaConduitInvalidReceiveCodec;
  }

  
  condError = StartReceiving();
  if (condError != kMediaConduitNoError) {
    return condError;
  }

  DumpCodecDB();
  return kMediaConduitNoError;
}
MediaConduitErrorCode
WebrtcAudioConduit::EnableAudioLevelExtension(bool enabled, uint8_t id)
{
  CSFLogDebug(logTag,  "%s %d %d ", __FUNCTION__, enabled, id);

  if (mPtrVoERTP_RTCP->SetSendAudioLevelIndicationStatus(mChannel, enabled, id) == -1)
  {
    CSFLogError(logTag, "%s SetSendAudioLevelIndicationStatus Failed", __FUNCTION__);
    return kMediaConduitUnknownError;
  }

  return kMediaConduitNoError;
}

MediaConduitErrorCode
WebrtcAudioConduit::SendAudioFrame(const int16_t audio_data[],
                                    int32_t lengthSamples,
                                    int32_t samplingFreqHz,
                                    int32_t capture_delay)
{
  CSFLogDebug(logTag,  "%s ", __FUNCTION__);
  
  
  
  
  
  
  
  

  if(!audio_data || (lengthSamples <= 0) ||
                    (IsSamplingFreqSupported(samplingFreqHz) == false) ||
                    ((lengthSamples % (samplingFreqHz / 100) != 0)) )
  {
    CSFLogError(logTag, "%s Invalid Parameters ",__FUNCTION__);
    MOZ_ASSERT(PR_FALSE);
    return kMediaConduitMalformedArgument;
  }

  
  if(capture_delay < 0 )
  {
    CSFLogError(logTag,"%s Invalid Capture Delay ", __FUNCTION__);
    MOZ_ASSERT(PR_FALSE);
    return kMediaConduitMalformedArgument;
  }

  
  if(!mEngineTransmitting)
  {
    CSFLogError(logTag, "%s Engine not transmitting ", __FUNCTION__);
    return kMediaConduitSessionNotInited;
  }

#if !defined(MOZILLA_EXTERNAL_LINKAGE)
    if (PR_LOG_TEST(GetLatencyLog(), PR_LOG_DEBUG)) {
      struct Processing insert = { TimeStamp::Now(), 0 };
      mProcessing.AppendElement(insert);
    }
#endif

  capture_delay = mCaptureDelay;
  
  if(mPtrVoEXmedia->ExternalRecordingInsertData(audio_data,
                                                lengthSamples,
                                                samplingFreqHz,
                                                capture_delay) == -1)
  {
    int error = mPtrVoEBase->LastError();
    CSFLogError(logTag,  "%s Inserting audio data Failed %d", __FUNCTION__, error);
    if(error == VE_RUNTIME_REC_ERROR)
    {
      return kMediaConduitRecordingError;
    }
    return kMediaConduitUnknownError;
  }
  
  return kMediaConduitNoError;
}

MediaConduitErrorCode
WebrtcAudioConduit::GetAudioFrame(int16_t speechData[],
                                   int32_t samplingFreqHz,
                                   int32_t capture_delay,
                                   int& lengthSamples)
{

  CSFLogDebug(logTag,  "%s ", __FUNCTION__);
  unsigned int numSamples = 0;

  
  if(!speechData )
  {
    CSFLogError(logTag,"%s Null Audio Buffer Pointer", __FUNCTION__);
    MOZ_ASSERT(PR_FALSE);
    return kMediaConduitMalformedArgument;
  }

  
  if((numSamples = GetNum10msSamplesForFrequency(samplingFreqHz)) == 0  )
  {
    CSFLogError(logTag,"%s Invalid Sampling Frequency ", __FUNCTION__);
    MOZ_ASSERT(PR_FALSE);
    return kMediaConduitMalformedArgument;
  }

  
  if(capture_delay < 0 )
  {
    CSFLogError(logTag,"%s Invalid Capture Delay ", __FUNCTION__);
    MOZ_ASSERT(PR_FALSE);
    return kMediaConduitMalformedArgument;
  }

  
  
  if(!mEngineReceiving)
  {
    CSFLogError(logTag, "%s Engine not Receiving ", __FUNCTION__);
    return kMediaConduitSessionNotInited;
  }


  lengthSamples = 0;  

  if(mPtrVoEXmedia->ExternalPlayoutGetData( speechData,
                                            samplingFreqHz,
                                            capture_delay,
                                            lengthSamples) == -1)
  {
    int error = mPtrVoEBase->LastError();
    CSFLogError(logTag,  "%s Getting audio data Failed %d", __FUNCTION__, error);
    if(error == VE_RUNTIME_PLAY_ERROR)
    {
      return kMediaConduitPlayoutError;
    }
    return kMediaConduitUnknownError;
  }

  
  mSamples += lengthSamples;
  if (mSamples >= mLastSyncLog + samplingFreqHz) {
    int jitter_buffer_delay_ms;
    int playout_buffer_delay_ms;
    int avsync_offset_ms;
    if (GetAVStats(&jitter_buffer_delay_ms,
                   &playout_buffer_delay_ms,
                   &avsync_offset_ms)) {
#if !defined(MOZILLA_EXTERNAL_LINKAGE)
      if (avsync_offset_ms < 0) {
        Telemetry::Accumulate(Telemetry::WEBRTC_AVSYNC_WHEN_VIDEO_LAGS_AUDIO_MS,
                              -avsync_offset_ms);
      } else {
        Telemetry::Accumulate(Telemetry::WEBRTC_AVSYNC_WHEN_AUDIO_LAGS_VIDEO_MS,
                              avsync_offset_ms);
      }
#endif
      CSFLogError(logTag,
                  "A/V sync: sync delta: %dms, audio jitter delay %dms, playout delay %dms",
                  avsync_offset_ms, jitter_buffer_delay_ms, playout_buffer_delay_ms);
    } else {
      CSFLogError(logTag, "A/V sync: GetAVStats failed");
    }
    mLastSyncLog = mSamples;
  }

#if !defined(MOZILLA_EXTERNAL_LINKAGE)
  if (PR_LOG_TEST(GetLatencyLog(), PR_LOG_DEBUG)) {
    if (mProcessing.Length() > 0) {
      unsigned int now;
      mPtrVoEVideoSync->GetPlayoutTimestamp(mChannel, now);
      if (static_cast<uint32_t>(now) != mLastTimestamp) {
        mLastTimestamp = static_cast<uint32_t>(now);
        
        while (mProcessing.Length() > 0) {
          
          
          if (mProcessing[0].mRTPTimeStamp + 20*(48000/1000) >= now) {
            TimeDuration t = TimeStamp::Now() - mProcessing[0].mTimeStamp;
            
            int64_t delta = t.ToMilliseconds() + (now - mProcessing[0].mRTPTimeStamp)/(48000/1000);
            LogTime(AsyncLatencyLogger::AudioRecvRTP, ((uint64_t) this), delta);
            break;
          }
          mProcessing.RemoveElementAt(0);
        }
      }
    }
  }
#endif
  CSFLogDebug(logTag,"%s GetAudioFrame:Got samples: length %d ",__FUNCTION__,
                                                               lengthSamples);
  return kMediaConduitNoError;
}


MediaConduitErrorCode
WebrtcAudioConduit::ReceivedRTPPacket(const void *data, int len)
{
  CSFLogDebug(logTag,  "%s : channel %d", __FUNCTION__, mChannel);

  if(mEngineReceiving)
  {
#if !defined(MOZILLA_EXTERNAL_LINKAGE)
    if (PR_LOG_TEST(GetLatencyLog(), PR_LOG_DEBUG)) {
      
      struct Processing insert = { TimeStamp::Now(),
                                   ntohl(static_cast<const uint32_t *>(data)[1]) };
      mProcessing.AppendElement(insert);
    }
#endif

    
    if(mPtrVoENetwork->ReceivedRTPPacket(mChannel, data, len) == -1)
    {
      int error = mPtrVoEBase->LastError();
      CSFLogError(logTag, "%s RTP Processing Error %d", __FUNCTION__, error);
      if(error == VE_RTP_RTCP_MODULE_ERROR)
      {
        return kMediaConduitRTPRTCPModuleError;
      }
      return kMediaConduitUnknownError;
    }
  } else {
    CSFLogError(logTag, "Error: %s when not receiving", __FUNCTION__);
    return kMediaConduitSessionNotInited;
  }

  return kMediaConduitNoError;
}

MediaConduitErrorCode
WebrtcAudioConduit::ReceivedRTCPPacket(const void *data, int len)
{
  CSFLogDebug(logTag,  "%s : channel %d",__FUNCTION__, mChannel);

  if(mPtrVoENetwork->ReceivedRTCPPacket(mChannel, data, len) == -1)
  {
    int error = mPtrVoEBase->LastError();
    CSFLogError(logTag, "%s RTCP Processing Error %d", __FUNCTION__, error);
    if(error == VE_RTP_RTCP_MODULE_ERROR)
    {
      return kMediaConduitRTPRTCPModuleError;
    }
    return kMediaConduitUnknownError;
  }
  return kMediaConduitNoError;
}

MediaConduitErrorCode
WebrtcAudioConduit::StopTransmitting()
{
  if(mEngineTransmitting)
  {
    CSFLogDebug(logTag, "%s Engine Already Sending. Attemping to Stop ", __FUNCTION__);
    if(mPtrVoEBase->StopSend(mChannel) == -1)
    {
      CSFLogError(logTag, "%s StopSend() Failed %d ", __FUNCTION__,
                  mPtrVoEBase->LastError());
      return kMediaConduitUnknownError;
    }
    mEngineTransmitting = false;
  }

  return kMediaConduitNoError;
}

MediaConduitErrorCode
WebrtcAudioConduit::StartTransmitting()
{
  if (!mEngineTransmitting) {
    
    if(mPtrVoEBase->StartSend(mChannel) == -1)
    {
      int error = mPtrVoEBase->LastError();
      CSFLogError(logTag, "%s StartSend failed %d", __FUNCTION__, error);
      return kMediaConduitUnknownError;
    }
    mEngineTransmitting = true;
  }

  return kMediaConduitNoError;
}

MediaConduitErrorCode
WebrtcAudioConduit::StopReceiving()
{
  if(mEngineReceiving)
  {
    CSFLogDebug(logTag, "%s Engine Already Receiving. Attemping to Stop ", __FUNCTION__);
    
    
    mPtrVoEBase->StopReceive(mChannel);
    CSFLogDebug(logTag, "%s Attemping to Stop playout ", __FUNCTION__);
    if(mPtrVoEBase->StopPlayout(mChannel) == -1)
    {
      if( mPtrVoEBase->LastError() == VE_CANNOT_STOP_PLAYOUT)
      {
        CSFLogDebug(logTag, "%s Stop-Playout Failed %d", __FUNCTION__, mPtrVoEBase->LastError());
        return kMediaConduitPlayoutError;
      }
    }
    mEngineReceiving = false;
  }

  return kMediaConduitNoError;
}

MediaConduitErrorCode
WebrtcAudioConduit::StartReceiving()
{
  if (!mEngineReceiving) {
    if(mPtrVoEBase->StartReceive(mChannel) == -1)
    {
      int error = mPtrVoEBase->LastError();
      CSFLogError(logTag ,  "%s StartReceive Failed %d ",__FUNCTION__, error);
      if(error == VE_RECV_SOCKET_ERROR)
      {
        return kMediaConduitSocketError;
      }
      return kMediaConduitUnknownError;
    }


    if(mPtrVoEBase->StartPlayout(mChannel) == -1)
    {
      CSFLogError(logTag, "%s Starting playout Failed", __FUNCTION__);
      return kMediaConduitPlayoutError;
    }
    mEngineReceiving = true;
  }

  return kMediaConduitNoError;
}



int WebrtcAudioConduit::SendPacket(int channel, const void* data, int len)
{
  CSFLogDebug(logTag,  "%s : channel %d", __FUNCTION__, channel);

#if !defined(MOZILLA_EXTERNAL_LINKAGE)
  if (PR_LOG_TEST(GetLatencyLog(), PR_LOG_DEBUG)) {
    if (mProcessing.Length() > 0) {
      TimeStamp started = mProcessing[0].mTimeStamp;
      mProcessing.RemoveElementAt(0);
      mProcessing.RemoveElementAt(0); 
      TimeDuration t = TimeStamp::Now() - started;
      int64_t delta = t.ToMilliseconds();
      LogTime(AsyncLatencyLogger::AudioSendRTP, ((uint64_t) this), delta);
    }
  }
#endif
  ReentrantMonitorAutoEnter enter(mTransportMonitor);
  if(mTransmitterTransport &&
     (mTransmitterTransport->SendRtpPacket(data, len) == NS_OK))
  {
    CSFLogDebug(logTag, "%s Sent RTP Packet ", __FUNCTION__);
    return len;
  } else {
    CSFLogError(logTag, "%s RTP Packet Send Failed ", __FUNCTION__);
    return -1;
  }
}


int WebrtcAudioConduit::SendRTCPPacket(int channel, const void* data, int len)
{
  CSFLogDebug(logTag,  "%s : channel %d , len %d, first rtcp = %u ",
              __FUNCTION__,
              channel,
              len,
              static_cast<unsigned>(((uint8_t *) data)[1]));

  
  
  
  ReentrantMonitorAutoEnter enter(mTransportMonitor);
  if(mReceiverTransport &&
     mReceiverTransport->SendRtcpPacket(data, len) == NS_OK)
  {
    
    CSFLogDebug(logTag, "%s Sent RTCP Packet ", __FUNCTION__);
    return len;
  } else if(mTransmitterTransport &&
            (mTransmitterTransport->SendRtcpPacket(data, len) == NS_OK)) {
      CSFLogDebug(logTag, "%s Sent RTCP Packet (sender report) ", __FUNCTION__);
      return len;
  } else {
    CSFLogError(logTag, "%s RTCP Packet Send Failed ", __FUNCTION__);
    return -1;
  }
}





bool
WebrtcAudioConduit::CodecConfigToWebRTCCodec(const AudioCodecConfig* codecInfo,
                                              webrtc::CodecInst& cinst)
 {
  const unsigned int plNameLength = codecInfo->mName.length();
  memset(&cinst, 0, sizeof(webrtc::CodecInst));
  if(sizeof(cinst.plname) < plNameLength+1)
  {
    CSFLogError(logTag, "%s Payload name buffer capacity mismatch ",
                                                      __FUNCTION__);
    return false;
  }
  memcpy(cinst.plname, codecInfo->mName.c_str(), plNameLength);
  cinst.plname[plNameLength]='\0';
  cinst.pltype   =  codecInfo->mType;
  cinst.rate     =  codecInfo->mRate;
  cinst.pacsize  =  codecInfo->mPacSize;
  cinst.plfreq   =  codecInfo->mFreq;
  if (codecInfo->mName == "G722") {
    
    cinst.plfreq = 16000;
  }
  cinst.channels =  codecInfo->mChannels;
  return true;
 }




bool
WebrtcAudioConduit::IsSamplingFreqSupported(int freq) const
{
  if(GetNum10msSamplesForFrequency(freq))
  {
    return true;
  } else {
    return false;
  }
}


unsigned int
WebrtcAudioConduit::GetNum10msSamplesForFrequency(int samplingFreqHz) const
{
  switch(samplingFreqHz)
  {
    case 16000: return 160; 
    case 32000: return 320; 
    case 44100: return 441; 
    case 48000: return 480; 
    default:    return 0; 
  }
}


bool
WebrtcAudioConduit::CopyCodecToDB(const AudioCodecConfig* codecInfo)
{

  AudioCodecConfig* cdcConfig = new AudioCodecConfig(codecInfo->mType,
                                                     codecInfo->mName,
                                                     codecInfo->mFreq,
                                                     codecInfo->mPacSize,
                                                     codecInfo->mChannels,
                                                     codecInfo->mRate);
  mRecvCodecList.push_back(cdcConfig);
  return true;
}




bool
WebrtcAudioConduit::CheckCodecsForMatch(const AudioCodecConfig* curCodecConfig,
                                         const AudioCodecConfig* codecInfo) const
{
  if(!curCodecConfig)
  {
    return false;
  }

  if(curCodecConfig->mType   == codecInfo->mType &&
      (curCodecConfig->mName.compare(codecInfo->mName) == 0) &&
      curCodecConfig->mFreq   == codecInfo->mFreq &&
      curCodecConfig->mPacSize == codecInfo->mPacSize &&
      curCodecConfig->mChannels == codecInfo->mChannels &&
      curCodecConfig->mRate == codecInfo->mRate)
  {
    return true;
  }

  return false;
}




bool
WebrtcAudioConduit::CheckCodecForMatch(const AudioCodecConfig* codecInfo) const
{
  
  for(std::vector<AudioCodecConfig*>::size_type i=0;i < mRecvCodecList.size();i++)
  {
    if(CheckCodecsForMatch(mRecvCodecList[i],codecInfo))
    {
      
      return true;
    }
  }
  
  return false;
}






MediaConduitErrorCode
WebrtcAudioConduit::ValidateCodecConfig(const AudioCodecConfig* codecInfo,
                                        bool send) const
{
  bool codecAppliedAlready = false;

  if(!codecInfo)
  {
    CSFLogError(logTag, "%s Null CodecConfig ", __FUNCTION__);
    return kMediaConduitMalformedArgument;
  }

  if((codecInfo->mName.empty()) ||
     (codecInfo->mName.length() >= CODEC_PLNAME_SIZE))
  {
    CSFLogError(logTag, "%s Invalid Payload Name Length ", __FUNCTION__);
    return kMediaConduitMalformedArgument;
  }

  
  if( (codecInfo->mChannels != 1) && (codecInfo->mChannels != 2))
  {
    CSFLogError(logTag, "%s Channel Unsupported ", __FUNCTION__);
    return kMediaConduitMalformedArgument;
  }

  
  if(send)
  {
    codecAppliedAlready = CheckCodecsForMatch(mCurSendCodecConfig,codecInfo);
  } else {
    codecAppliedAlready = CheckCodecForMatch(codecInfo);
  }

  if(codecAppliedAlready)
  {
    CSFLogDebug(logTag, "%s Codec %s Already Applied  ", __FUNCTION__, codecInfo->mName.c_str());
  }
  return kMediaConduitNoError;
}

void
WebrtcAudioConduit::DumpCodecDB() const
 {
    for(std::vector<AudioCodecConfig*>::size_type i=0;i < mRecvCodecList.size();i++)
    {
      CSFLogDebug(logTag,"Payload Name: %s", mRecvCodecList[i]->mName.c_str());
      CSFLogDebug(logTag,"Payload Type: %d", mRecvCodecList[i]->mType);
      CSFLogDebug(logTag,"Payload Frequency: %d", mRecvCodecList[i]->mFreq);
      CSFLogDebug(logTag,"Payload PacketSize: %d", mRecvCodecList[i]->mPacSize);
      CSFLogDebug(logTag,"Payload Channels: %d", mRecvCodecList[i]->mChannels);
      CSFLogDebug(logTag,"Payload Sampling Rate: %d", mRecvCodecList[i]->mRate);
    }
 }
}
