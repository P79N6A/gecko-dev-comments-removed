



#include "CSFLog.h"
#include "nspr.h"
#include "plstr.h"


#include "ccsdp.h"

#include "VideoConduit.h"
#include "AudioConduit.h"
#include "nsThreadUtils.h"
#include "LoadManager.h"
#include "YuvStamper.h"
#include "nsServiceManagerUtils.h"
#include "nsIPrefService.h"
#include "nsIPrefBranch.h"

#include "webrtc/common_types.h"
#include "webrtc/common_video/interface/native_handle.h"
#include "webrtc/video_engine/include/vie_errors.h"
#include "browser_logging/WebRtcLog.h"

#ifdef MOZ_WIDGET_ANDROID
#include "AndroidJNIWrapper.h"
#endif

#include <algorithm>
#include <math.h>

#define DEFAULT_VIDEO_MAX_FRAMERATE 30

namespace mozilla {

static const char* logTag ="WebrtcVideoSessionConduit";


const unsigned int WebrtcVideoConduit::CODEC_PLNAME_SIZE = 32;




mozilla::RefPtr<VideoSessionConduit> VideoSessionConduit::Create(VideoSessionConduit *aOther)
{
  NS_ASSERTION(NS_IsMainThread(), "Only call on main thread");
  CSFLogDebug(logTag,  "%s ", __FUNCTION__);

  WebrtcVideoConduit* obj = new WebrtcVideoConduit();
  if(obj->Init(static_cast<WebrtcVideoConduit*>(aOther)) != kMediaConduitNoError)
  {
    CSFLogError(logTag,  "%s VideoConduit Init Failed ", __FUNCTION__);
    delete obj;
    return nullptr;
  }
  CSFLogDebug(logTag,  "%s Successfully created VideoConduit ", __FUNCTION__);
  return obj;
}

WebrtcVideoConduit::WebrtcVideoConduit():
  mOtherDirection(nullptr),
  mShutDown(false),
  mVideoEngine(nullptr),
  mTransport(nullptr),
  mRenderer(nullptr),
  mPtrExtCapture(nullptr),
  mEngineTransmitting(false),
  mEngineReceiving(false),
  mChannel(-1),
  mCapId(-1),
  mCurSendCodecConfig(nullptr),
  mSendingWidth(0),
  mSendingHeight(0),
  mReceivingWidth(640),
  mReceivingHeight(480),
  mVideoLatencyTestEnable(false),
  mVideoLatencyAvg(0),
  mMinBitrate(200),
  mStartBitrate(300),
  mMaxBitrate(2000)
{
}

WebrtcVideoConduit::~WebrtcVideoConduit()
{
  NS_ASSERTION(NS_IsMainThread(), "Only call on main thread");
  CSFLogDebug(logTag,  "%s ", __FUNCTION__);

  for(std::vector<VideoCodecConfig*>::size_type i=0;i < mRecvCodecList.size();i++)
  {
    delete mRecvCodecList[i];
  }

  delete mCurSendCodecConfig;

  
  
  if(mPtrViECapture)
  {
    if (!mShutDown) {
      mPtrViECapture->DisconnectCaptureDevice(mCapId);
      mPtrViECapture->ReleaseCaptureDevice(mCapId);
      mPtrExtCapture = nullptr;
      if (mOtherDirection)
        mOtherDirection->mPtrExtCapture = nullptr;
    }
  }

   if (mPtrExtCodec) {
     mPtrExtCodec->Release();
     mPtrExtCodec = NULL;
   }

  
  if(mPtrViERender)
  {
    if (!mShutDown) {
      if(mRenderer) {
        mPtrViERender->StopRender(mChannel);
      }
      mPtrViERender->RemoveRenderer(mChannel);
    }
  }

  
  if(mPtrViENetwork)
  {
    if (!mShutDown) {
      mPtrViENetwork->DeregisterSendTransport(mChannel);
    }
  }

  if(mPtrViEBase)
  {
    if (!mShutDown) {
      mPtrViEBase->StopSend(mChannel);
      mPtrViEBase->StopReceive(mChannel);
      SyncTo(nullptr);
      mPtrViEBase->DeleteChannel(mChannel);
    }
  }

  if (mOtherDirection)
  {
    
    mOtherDirection->mOtherDirection = nullptr;
    
    mOtherDirection->mShutDown = true;
    mVideoEngine = nullptr;
  } else {
    
    if (mVideoCodecStat) {
      mVideoCodecStat->EndOfCallStats();
    }
    mVideoCodecStat = nullptr;
    
    
    mPtrViEBase = nullptr;
    mPtrViECapture = nullptr;
    mPtrViECodec = nullptr;
    mPtrViENetwork = nullptr;
    mPtrViERender = nullptr;
    mPtrRTP = nullptr;
    mPtrExtCodec = nullptr;

    
    if(mVideoEngine)
    {
      webrtc::VideoEngine::Delete(mVideoEngine);
    }
  }
}

bool WebrtcVideoConduit::GetLocalSSRC(unsigned int* ssrc)
{
  return !mPtrRTP->GetLocalSSRC(mChannel, *ssrc);
}

bool WebrtcVideoConduit::GetRemoteSSRC(unsigned int* ssrc)
{
  return !mPtrRTP->GetRemoteSSRC(mChannel, *ssrc);
}

bool WebrtcVideoConduit::GetVideoEncoderStats(double* framerateMean,
                                              double* framerateStdDev,
                                              double* bitrateMean,
                                              double* bitrateStdDev,
                                              uint32_t* droppedFrames)
{
  if (!mEngineTransmitting) {
    return false;
  }
  MOZ_ASSERT(mVideoCodecStat);
  mVideoCodecStat->GetEncoderStats(framerateMean, framerateStdDev,
                                   bitrateMean, bitrateStdDev,
                                   droppedFrames);
  return true;
}

bool WebrtcVideoConduit::GetVideoDecoderStats(double* framerateMean,
                                              double* framerateStdDev,
                                              double* bitrateMean,
                                              double* bitrateStdDev,
                                              uint32_t* discardedPackets)
{
  if (!mEngineReceiving) {
    return false;
  }
  MOZ_ASSERT(mVideoCodecStat);
  mVideoCodecStat->GetDecoderStats(framerateMean, framerateStdDev,
                                   bitrateMean, bitrateStdDev,
                                   discardedPackets);
  return true;
}

bool WebrtcVideoConduit::GetAVStats(int32_t* jitterBufferDelayMs,
                                    int32_t* playoutBufferDelayMs,
                                    int32_t* avSyncOffsetMs) {
  return false;
}

bool WebrtcVideoConduit::GetRTPStats(unsigned int* jitterMs,
                                     unsigned int* cumulativeLost) {
  unsigned short fractionLost;
  unsigned extendedMax;
  int rttMs;
  
  return !mPtrRTP->GetReceivedRTCPStatistics(mChannel, fractionLost,
                                             *cumulativeLost,
                                             extendedMax,
                                             *jitterMs,
                                             rttMs);
}

bool WebrtcVideoConduit::GetRTCPReceiverReport(DOMHighResTimeStamp* timestamp,
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
                                                    jitterMs,
                                                    &fractionLost,
                                                    cumulativeLost,
                                                    rttMs);
  if (result) {
    *timestamp = NTPtoDOMHighResTimeStamp(ntpHigh, ntpLow);
  }
  return result;
}

bool WebrtcVideoConduit::GetRTCPSenderReport(DOMHighResTimeStamp* timestamp,
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




MediaConduitErrorCode WebrtcVideoConduit::Init(WebrtcVideoConduit *other)
{
  CSFLogDebug(logTag,  "%s this=%p other=%p", __FUNCTION__, this, other);

#ifdef MOZILLA_INTERNAL_API
  
  MOZ_ASSERT(NS_IsMainThread());

  nsresult rv;
  nsCOMPtr<nsIPrefService> prefs = do_GetService("@mozilla.org/preferences-service;1", &rv);
  if (!NS_WARN_IF(NS_FAILED(rv)))
  {
    nsCOMPtr<nsIPrefBranch> branch = do_QueryInterface(prefs);

    if (branch)
    {
      int32_t temp;
      NS_WARN_IF(NS_FAILED(branch->GetBoolPref("media.video.test_latency", &mVideoLatencyTestEnable)));
      NS_WARN_IF(NS_FAILED(branch->GetIntPref("media.peerconnection.video.min_bitrate", &temp)));
      if (temp >= 0) {
        mMinBitrate = temp;
      }
      NS_WARN_IF(NS_FAILED(branch->GetIntPref("media.peerconnection.video.start_bitrate", &temp)));
      if (temp >= 0) {
        mStartBitrate = temp;
      }
      NS_WARN_IF(NS_FAILED(branch->GetIntPref("media.peerconnection.video.max_bitrate", &temp)));
      if (temp >= 0) {
        mMaxBitrate = temp;
      }
      bool use_loadmanager = false;
      NS_WARN_IF(NS_FAILED(branch->GetBoolPref("media.navigator.load_adapt", &use_loadmanager)));
      if (use_loadmanager) {
        mLoadManager = LoadManagerBuild();
      }
    }
  }
#endif

  if (other) {
    MOZ_ASSERT(!other->mOtherDirection);
    other->mOtherDirection = this;
    mOtherDirection = other;

    
    MOZ_ASSERT(other->mVideoEngine);
    mVideoEngine = other->mVideoEngine;
  } else {

#ifdef MOZ_WIDGET_ANDROID
    
    JavaVM *jvm = jsjni_GetVM();

    if (webrtc::VideoEngine::SetAndroidObjects(jvm) != 0) {
      CSFLogError(logTag,  "%s: could not set Android objects", __FUNCTION__);
      return kMediaConduitSessionNotInited;
    }
#endif

    
    if( !(mVideoEngine = webrtc::VideoEngine::Create()) )
    {
      CSFLogError(logTag, "%s Unable to create video engine ", __FUNCTION__);
      return kMediaConduitSessionNotInited;
    }

    EnableWebRtcLog();
  }

  if( !(mPtrViEBase = ViEBase::GetInterface(mVideoEngine)))
  {
    CSFLogError(logTag, "%s Unable to get video base interface ", __FUNCTION__);
    return kMediaConduitSessionNotInited;
  }

  if( !(mPtrViECapture = ViECapture::GetInterface(mVideoEngine)))
  {
    CSFLogError(logTag, "%s Unable to get video capture interface", __FUNCTION__);
    return kMediaConduitSessionNotInited;
  }

  if( !(mPtrViECodec = ViECodec::GetInterface(mVideoEngine)))
  {
    CSFLogError(logTag, "%s Unable to get video codec interface ", __FUNCTION__);
    return kMediaConduitSessionNotInited;
  }

  if( !(mPtrViENetwork = ViENetwork::GetInterface(mVideoEngine)))
  {
    CSFLogError(logTag, "%s Unable to get video network interface ", __FUNCTION__);
    return kMediaConduitSessionNotInited;
  }

  if( !(mPtrViERender = ViERender::GetInterface(mVideoEngine)))
  {
    CSFLogError(logTag, "%s Unable to get video render interface ", __FUNCTION__);
    return kMediaConduitSessionNotInited;
  }

  mPtrExtCodec = webrtc::ViEExternalCodec::GetInterface(mVideoEngine);
  if (!mPtrExtCodec) {
    CSFLogError(logTag, "%s Unable to get external codec interface: %d ",
                __FUNCTION__,mPtrViEBase->LastError());
    return kMediaConduitSessionNotInited;
  }

  if( !(mPtrRTP = webrtc::ViERTP_RTCP::GetInterface(mVideoEngine)))
  {
    CSFLogError(logTag, "%s Unable to get video RTCP interface ", __FUNCTION__);
    return kMediaConduitSessionNotInited;
  }

  if ( !(mPtrExtCodec = webrtc::ViEExternalCodec::GetInterface(mVideoEngine)))
  {
    CSFLogError(logTag, "%s Unable to get external codec interface %d ",
                __FUNCTION__, mPtrViEBase->LastError());
    return kMediaConduitSessionNotInited;
  }

  if (other) {
    mChannel = other->mChannel;
    mPtrExtCapture = other->mPtrExtCapture;
    mCapId = other->mCapId;
  } else {
    CSFLogDebug(logTag, "%s Engine Created: Init'ng the interfaces ",__FUNCTION__);

    if(mPtrViEBase->Init() == -1)
    {
      CSFLogError(logTag, " %s Video Engine Init Failed %d ",__FUNCTION__,
                  mPtrViEBase->LastError());
      return kMediaConduitSessionNotInited;
    }

    if(mPtrViEBase->CreateChannel(mChannel) == -1)
    {
      CSFLogError(logTag, " %s Channel creation Failed %d ",__FUNCTION__,
                  mPtrViEBase->LastError());
      return kMediaConduitChannelError;
    }

    if(mPtrViENetwork->RegisterSendTransport(mChannel, *this) == -1)
    {
      CSFLogError(logTag,  "%s ViENetwork Failed %d ", __FUNCTION__,
                  mPtrViEBase->LastError());
      return kMediaConduitTransportRegistrationFail;
    }

    if(mPtrViECapture->AllocateExternalCaptureDevice(mCapId,
                                                     mPtrExtCapture) == -1)
    {
      CSFLogError(logTag, "%s Unable to Allocate capture module: %d ",
                  __FUNCTION__, mPtrViEBase->LastError());
      return kMediaConduitCaptureError;
    }

    if(mPtrViECapture->ConnectCaptureDevice(mCapId,mChannel) == -1)
    {
      CSFLogError(logTag, "%s Unable to Connect capture module: %d ",
                  __FUNCTION__,mPtrViEBase->LastError());
      return kMediaConduitCaptureError;
    }

    if(mPtrViERender->AddRenderer(mChannel,
                                  webrtc::kVideoI420,
                                  (webrtc::ExternalRenderer*) this) == -1)
    {
      CSFLogError(logTag, "%s Failed to added external renderer ", __FUNCTION__);
      return kMediaConduitInvalidRenderer;
    }
    
    if(mPtrViENetwork->SetMTU(mChannel, 1200) != 0)
    {
      CSFLogError(logTag,  "%s MTU Failed %d ", __FUNCTION__,
                  mPtrViEBase->LastError());
      return kMediaConduitMTUError;
    }
    
    if(mPtrRTP->SetRTCPStatus(mChannel, webrtc::kRtcpCompound_RFC4585) != 0)
    {
      CSFLogError(logTag,  "%s RTCPStatus Failed %d ", __FUNCTION__,
                  mPtrViEBase->LastError());
      return kMediaConduitRTCPStatusError;
    }
  }

  CSFLogError(logTag, "%s Initialization Done", __FUNCTION__);
  return kMediaConduitNoError;
}

void
WebrtcVideoConduit::SyncTo(WebrtcAudioConduit *aConduit)
{
  CSFLogDebug(logTag, "%s Synced to %p", __FUNCTION__, aConduit);

  
  
  
  if (aConduit) {
    mPtrViEBase->SetVoiceEngine(aConduit->GetVoiceEngine());
    mPtrViEBase->ConnectAudioChannel(mChannel, aConduit->GetChannel());
    
  } else if ((mOtherDirection && mOtherDirection->mSyncedTo) || mSyncedTo) {
    mPtrViEBase->DisconnectAudioChannel(mChannel);
    mPtrViEBase->SetVoiceEngine(nullptr);
  }

  
  if (mSyncedTo || !mOtherDirection ) {
    mSyncedTo = aConduit;
  } else {
    mOtherDirection->mSyncedTo = aConduit;
  }
}

MediaConduitErrorCode
WebrtcVideoConduit::AttachRenderer(mozilla::RefPtr<VideoRenderer> aVideoRenderer)
{
  CSFLogDebug(logTag,  "%s ", __FUNCTION__);

  
  if(!aVideoRenderer)
  {
    CSFLogError(logTag, "%s NULL Renderer", __FUNCTION__);
    MOZ_ASSERT(PR_FALSE);
    return kMediaConduitInvalidRenderer;
  }

  
  if(!mRenderer)
  {
    mRenderer = aVideoRenderer; 

    if(mPtrViERender->StartRender(mChannel) == -1)
    {
      CSFLogError(logTag, "%s Starting the Renderer Failed %d ", __FUNCTION__,
                                                      mPtrViEBase->LastError());
      mRenderer = nullptr;
      return kMediaConduitRendererFail;
    }
  } else {
    
    mRenderer = aVideoRenderer;
  }

  return kMediaConduitNoError;
}

void
WebrtcVideoConduit::DetachRenderer()
{
  if(mRenderer)
  {
    mPtrViERender->StopRender(mChannel);
    mRenderer = nullptr;
  }
}

MediaConduitErrorCode
WebrtcVideoConduit::AttachTransport(mozilla::RefPtr<TransportInterface> aTransport)
{
  CSFLogDebug(logTag,  "%s ", __FUNCTION__);
  if(!aTransport)
  {
    CSFLogError(logTag, "%s NULL Transport", __FUNCTION__);
    return kMediaConduitInvalidTransport;
  }
  
  mTransport = aTransport;
  return kMediaConduitNoError;
}





MediaConduitErrorCode
WebrtcVideoConduit::ConfigureSendMediaCodec(const VideoCodecConfig* codecConfig)
{
  CSFLogDebug(logTag,  "%s for %s", __FUNCTION__, codecConfig ? codecConfig->mName.c_str() : "<null>");
  bool codecFound = false;
  MediaConduitErrorCode condError = kMediaConduitNoError;
  int error = 0; 
  webrtc::VideoCodec  video_codec;
  std::string payloadName;

  memset(&video_codec, 0, sizeof(video_codec));

  
  if((condError = ValidateCodecConfig(codecConfig,true)) != kMediaConduitNoError)
  {
    return condError;
  }

  
  if(CheckCodecsForMatch(mCurSendCodecConfig, codecConfig))
  {
    CSFLogDebug(logTag,  "%s Codec has been applied already ", __FUNCTION__);
    return kMediaConduitCodecInUse;
  }

  
  if(mEngineTransmitting)
  {
    CSFLogDebug(logTag, "%s Engine Already Sending. Attemping to Stop ", __FUNCTION__);
    if(mPtrViEBase->StopSend(mChannel) == -1)
    {
      CSFLogError(logTag, "%s StopSend() Failed %d ",__FUNCTION__,
                  mPtrViEBase->LastError());
      return kMediaConduitUnknownError;
    }

    mEngineTransmitting = false;
  }

  if (mLoadManager) {
    mPtrViEBase->RegisterCpuOveruseObserver(mChannel, mLoadManager);
    mPtrViEBase->SetLoadManager(mLoadManager);
  }

  if (mExternalSendCodec &&
      codecConfig->mType == mExternalSendCodec->mType) {
    CSFLogError(logTag, "%s Configuring External H264 Send Codec", __FUNCTION__);

    
    video_codec.width = 320;
    video_codec.height = 240;
#ifdef MOZ_WEBRTC_OMX
    if (codecConfig->mType == webrtc::kVideoCodecH264) {
      video_codec.resolution_divisor = 16;
    } else {
      video_codec.resolution_divisor = 1; 
    }
#else
    video_codec.resolution_divisor = 1; 
#endif
    video_codec.qpMax = 56;
    video_codec.numberOfSimulcastStreams = 1;
    video_codec.mode = webrtc::kRealtimeVideo;

    codecFound = true;
  } else {
    
    for(int idx=0; idx < mPtrViECodec->NumberOfCodecs(); idx++)
    {
      if(0 == mPtrViECodec->GetCodec(idx, video_codec))
      {
        payloadName = video_codec.plName;
        if(codecConfig->mName.compare(payloadName) == 0)
        {
          
          
          codecFound = true;
          break;
        }
      }
    }
  }

  if(codecFound == false)
  {
    CSFLogError(logTag, "%s Codec Mismatch ", __FUNCTION__);
    return kMediaConduitInvalidSendCodec;
  }
  
  CodecConfigToWebRTCCodec(codecConfig, video_codec);

  if(mPtrViECodec->SetSendCodec(mChannel, video_codec) == -1)
  {
    error = mPtrViEBase->LastError();
    if(error == kViECodecInvalidCodec)
    {
      CSFLogError(logTag, "%s Invalid Send Codec", __FUNCTION__);
      return kMediaConduitInvalidSendCodec;
    }
    CSFLogError(logTag, "%s SetSendCodec Failed %d ", __FUNCTION__,
                mPtrViEBase->LastError());
    return kMediaConduitUnknownError;
  }

  if (!mVideoCodecStat) {
    mVideoCodecStat = new VideoCodecStatistics(mChannel, mPtrViECodec, true);
  }

  mSendingWidth = 0;
  mSendingHeight = 0;

  if(codecConfig->RtcpFbIsSet(SDP_RTCP_FB_NACK_BASIC)) {
    CSFLogDebug(logTag, "Enabling NACK (send) for video stream\n");
    if (mPtrRTP->SetNACKStatus(mChannel, true) != 0)
    {
      CSFLogError(logTag,  "%s NACKStatus Failed %d ", __FUNCTION__,
                  mPtrViEBase->LastError());
      return kMediaConduitNACKStatusError;
    }
  }

  if(mPtrViEBase->StartSend(mChannel) == -1)
  {
    CSFLogError(logTag, "%s Start Send Error %d ", __FUNCTION__,
                mPtrViEBase->LastError());
    return kMediaConduitUnknownError;
  }

  
  delete mCurSendCodecConfig;

  mCurSendCodecConfig = new VideoCodecConfig(*codecConfig);

  mPtrRTP->SetRembStatus(mChannel, true, false);

  
  mEngineTransmitting = true;
  return kMediaConduitNoError;
}

MediaConduitErrorCode
WebrtcVideoConduit::ConfigureRecvMediaCodecs(
    const std::vector<VideoCodecConfig* >& codecConfigList)
{
  CSFLogDebug(logTag,  "%s ", __FUNCTION__);
  MediaConduitErrorCode condError = kMediaConduitNoError;
  int error = 0; 
  bool success = false;
  std::string  payloadName;

  
  
  if(mEngineReceiving)
  {
    CSFLogDebug(logTag, "%s Engine Already Receiving . Attemping to Stop ", __FUNCTION__);
    if(mPtrViEBase->StopReceive(mChannel) == -1)
    {
      error = mPtrViEBase->LastError();
      if(error == kViEBaseUnknownError)
      {
        CSFLogDebug(logTag, "%s StopReceive() Success ", __FUNCTION__);
        mEngineReceiving = false;
      } else {
        CSFLogError(logTag, "%s StopReceive() Failed %d ", __FUNCTION__,
                    mPtrViEBase->LastError());
        return kMediaConduitUnknownError;
      }
    }
  }

  mEngineReceiving = false;

  if(codecConfigList.empty())
  {
    CSFLogError(logTag, "%s Zero number of codecs to configure", __FUNCTION__);
    return kMediaConduitMalformedArgument;
  }

  webrtc::ViEKeyFrameRequestMethod kf_request = webrtc::kViEKeyFrameRequestNone;
  bool use_nack_basic = false;

  
  
  
  for(std::vector<VideoCodecConfig*>::size_type i=0;i < codecConfigList.size();i++)
  {
    
    if((condError = ValidateCodecConfig(codecConfigList[i],false)) != kMediaConduitNoError)
    {
      return condError;
    }

    
    
    if (codecConfigList[i]->RtcpFbIsSet(SDP_RTCP_FB_NACK_PLI))
    {
      kf_request = webrtc::kViEKeyFrameRequestPliRtcp;
    } else if(kf_request == webrtc::kViEKeyFrameRequestNone &&
              codecConfigList[i]->RtcpFbIsSet(SDP_RTCP_FB_CCM_FIR))
    {
      kf_request = webrtc::kViEKeyFrameRequestFirRtcp;
    }

    
    if(codecConfigList[i]->RtcpFbIsSet(SDP_RTCP_FB_NACK_BASIC))
    {
      use_nack_basic = true;
    }

    webrtc::VideoCodec  video_codec;

    mEngineReceiving = false;
    memset(&video_codec, 0, sizeof(webrtc::VideoCodec));

    if (mExternalRecvCodec &&
        codecConfigList[i]->mType == mExternalRecvCodec->mType) {
      CSFLogError(logTag, "%s Configuring External H264 Receive Codec", __FUNCTION__);

      
      
      CodecConfigToWebRTCCodec(codecConfigList[i], video_codec);

      
      if(mPtrViECodec->SetReceiveCodec(mChannel,video_codec) == -1)
      {
        CSFLogError(logTag, "%s Invalid Receive Codec %d ", __FUNCTION__,
                    mPtrViEBase->LastError());
      } else {
        CSFLogError(logTag, "%s Successfully Set the codec %s", __FUNCTION__,
                    codecConfigList[i]->mName.c_str());
        if(CopyCodecToDB(codecConfigList[i]))
        {
          success = true;
        } else {
          CSFLogError(logTag,"%s Unable to update Codec Database", __FUNCTION__);
          return kMediaConduitUnknownError;
        }
      }
    } else {
      
      for(int idx=0; idx < mPtrViECodec->NumberOfCodecs(); idx++)
      {
        if(mPtrViECodec->GetCodec(idx, video_codec) == 0)
        {
          payloadName = video_codec.plName;
          if(codecConfigList[i]->mName.compare(payloadName) == 0)
          {
            CodecConfigToWebRTCCodec(codecConfigList[i], video_codec);
            if(mPtrViECodec->SetReceiveCodec(mChannel,video_codec) == -1)
            {
              CSFLogError(logTag, "%s Invalid Receive Codec %d ", __FUNCTION__,
                          mPtrViEBase->LastError());
            } else {
              CSFLogError(logTag, "%s Successfully Set the codec %s", __FUNCTION__,
                          codecConfigList[i]->mName.c_str());
              if(CopyCodecToDB(codecConfigList[i]))
              {
                success = true;
              } else {
                CSFLogError(logTag,"%s Unable to update Codec Database", __FUNCTION__);
                return kMediaConduitUnknownError;
              }
            }
            break; 
          }
        }
      }
    }
  }

  if(!success)
  {
    CSFLogError(logTag, "%s Setting Receive Codec Failed ", __FUNCTION__);
    return kMediaConduitInvalidReceiveCodec;
  }

  if (!mVideoCodecStat) {
    mVideoCodecStat = new VideoCodecStatistics(mChannel, mPtrViECodec, false);
  }

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  if (kf_request != webrtc::kViEKeyFrameRequestNone)
  {
    CSFLogDebug(logTag, "Enabling %s frame requests for video stream\n",
                (kf_request == webrtc::kViEKeyFrameRequestPliRtcp ?
                 "PLI" : "FIR"));
    if(mPtrRTP->SetKeyFrameRequestMethod(mChannel, kf_request) != 0)
    {
      CSFLogError(logTag,  "%s KeyFrameRequest Failed %d ", __FUNCTION__,
                  mPtrViEBase->LastError());
      return kMediaConduitKeyFrameRequestError;
    }
  }

  switch (kf_request) {
    case webrtc::kViEKeyFrameRequestNone:
      mFrameRequestMethod = FrameRequestNone;
      break;
    case webrtc::kViEKeyFrameRequestPliRtcp:
      mFrameRequestMethod = FrameRequestPli;
      break;
    case webrtc::kViEKeyFrameRequestFirRtcp:
      mFrameRequestMethod = FrameRequestFir;
      break;
    default:
      MOZ_ASSERT(PR_FALSE);
      mFrameRequestMethod = FrameRequestUnknown;
  }

  if(use_nack_basic)
  {
    CSFLogDebug(logTag, "Enabling NACK (recv) for video stream\n");
    if (mPtrRTP->SetNACKStatus(mChannel, true) != 0)
    {
      CSFLogError(logTag,  "%s NACKStatus Failed %d ", __FUNCTION__,
                  mPtrViEBase->LastError());
      return kMediaConduitNACKStatusError;
    }
  }
  mUsingNackBasic = use_nack_basic;

  
  if(mPtrViEBase->StartReceive(mChannel) == -1)
  {
    error = mPtrViEBase->LastError();
    CSFLogError(logTag, "%s Start Receive Error %d ", __FUNCTION__, error);


    return kMediaConduitUnknownError;
  }

  
  mPtrRTP->SetRembStatus(mChannel, false, true);
  mEngineReceiving = true;
  DumpCodecDB();
  return kMediaConduitNoError;
}



bool
WebrtcVideoConduit::SelectSendResolution(unsigned short width,
                                         unsigned short height)
{
  

  
  
  if (mCurSendCodecConfig && mCurSendCodecConfig->mMaxFrameSize)
  {
    unsigned int cur_fs, max_width, max_height, mb_width, mb_height, mb_max;

    mb_width = (width + 15) >> 4;
    mb_height = (height + 15) >> 4;

    cur_fs = mb_width * mb_height;

    
    if (cur_fs > mCurSendCodecConfig->mMaxFrameSize)
    {
      double scale_ratio;

      scale_ratio = sqrt((double) mCurSendCodecConfig->mMaxFrameSize /
                         (double) cur_fs);

      mb_width = mb_width * scale_ratio;
      mb_height = mb_height * scale_ratio;

      
      if (mb_width == 0) {
        mb_width = 1;
        mb_height = std::min(mb_height, mCurSendCodecConfig->mMaxFrameSize);
      }
      if (mb_height == 0) {
        mb_height = 1;
        mb_width = std::min(mb_width, mCurSendCodecConfig->mMaxFrameSize);
      }
    }

    
    mb_max = (unsigned) sqrt(8 * (double) mCurSendCodecConfig->mMaxFrameSize);

    max_width = 16 * std::min(mb_width, mb_max);
    max_height = 16 * std::min(mb_height, mb_max);

    if (width * max_height > max_width * height)
    {
      if (width > max_width)
      {
        
        
        height = max_width * height / width + 1;
        width = max_width;
      }
    }
    else
    {
      if (height > max_height)
      {
        
        
        width = max_height * width / height + 1;
        height = max_height;
      }
    }

    
    width = std::max(width & ~1, 2);
    height = std::max(height & ~1, 2);
  }

  
  
  if (mSendingWidth != width || mSendingHeight != height)
  {
    
    
    
    mSendingWidth = width;
    mSendingHeight = height;

    
    webrtc::VideoCodec vie_codec;
    int32_t err;

    if ((err = mPtrViECodec->GetSendCodec(mChannel, vie_codec)) != 0)
    {
      CSFLogError(logTag, "%s: GetSendCodec failed, err %d", __FUNCTION__, err);
      return false;
    }
    if (vie_codec.width != width || vie_codec.height != height)
    {
      vie_codec.width = width;
      vie_codec.height = height;

      if ((err = mPtrViECodec->SetSendCodec(mChannel, vie_codec)) != 0)
      {
        CSFLogError(logTag, "%s: SetSendCodec(%ux%u) failed, err %d",
                    __FUNCTION__, width, height, err);
        return false;
      }
      CSFLogDebug(logTag, "%s: Encoder resolution changed to %ux%u",
                  __FUNCTION__, width, height);
    } 
  }
  return true;
}

MediaConduitErrorCode
WebrtcVideoConduit::SetExternalSendCodec(VideoCodecConfig* config,
                                         VideoEncoder* encoder) {
  if (!mPtrExtCodec->RegisterExternalSendCodec(mChannel,
                                              config->mType,
                                              static_cast<WebrtcVideoEncoder*>(encoder),
                                              false)) {
    mExternalSendCodecHandle = encoder;
    mExternalSendCodec = new VideoCodecConfig(*config);
    return kMediaConduitNoError;
  }
  return kMediaConduitInvalidSendCodec;
}

MediaConduitErrorCode
WebrtcVideoConduit::SetExternalRecvCodec(VideoCodecConfig* config,
                                         VideoDecoder* decoder) {
  if (!mPtrExtCodec->RegisterExternalReceiveCodec(mChannel,
                                                  config->mType,
                                                  static_cast<WebrtcVideoDecoder*>(decoder))) {
    mExternalRecvCodecHandle = decoder;
    mExternalRecvCodec = new VideoCodecConfig(*config);
    return kMediaConduitNoError;
  }
  return kMediaConduitInvalidReceiveCodec;
}

MediaConduitErrorCode
WebrtcVideoConduit::SendVideoFrame(unsigned char* video_frame,
                                   unsigned int video_frame_length,
                                   unsigned short width,
                                   unsigned short height,
                                   VideoType video_type,
                                   uint64_t capture_time)
{
  CSFLogDebug(logTag,  "%s ", __FUNCTION__);

  
  if(!video_frame || video_frame_length == 0 ||
     width == 0 || height == 0)
  {
    CSFLogError(logTag,  "%s Invalid Parameters ",__FUNCTION__);
    MOZ_ASSERT(PR_FALSE);
    return kMediaConduitMalformedArgument;
  }

  webrtc::RawVideoType type;
  switch (video_type) {
    case kVideoI420:
      type = webrtc::kVideoI420;
      break;
    case kVideoNV21:
      type = webrtc::kVideoNV21;
      break;
    default:
      CSFLogError(logTag,  "%s VideoType Invalid. Only 1420 and NV21 Supported",__FUNCTION__);
      MOZ_ASSERT(PR_FALSE);
      return kMediaConduitMalformedArgument;
  }
  
  if(!mEngineTransmitting)
  {
    CSFLogError(logTag, "%s Engine not transmitting ", __FUNCTION__);
    return kMediaConduitSessionNotInited;
  }

  if (!SelectSendResolution(width, height))
  {
    return kMediaConduitCaptureError;
  }

  
  MOZ_ASSERT(mPtrExtCapture);
  if(mPtrExtCapture->IncomingFrame(video_frame,
                                   video_frame_length,
                                   width, height,
                                   type,
                                   (unsigned long long)capture_time) == -1)
  {
    CSFLogError(logTag,  "%s IncomingFrame Failed %d ", __FUNCTION__,
                                            mPtrViEBase->LastError());
    return kMediaConduitCaptureError;
  }

  mVideoCodecStat->SentFrame();
  CSFLogDebug(logTag, "%s Inserted a frame", __FUNCTION__);
  return kMediaConduitNoError;
}


MediaConduitErrorCode
WebrtcVideoConduit::ReceivedRTPPacket(const void *data, int len)
{
  CSFLogDebug(logTag, "%s: Channel %d, Len %d ", __FUNCTION__, mChannel, len);

  
  if(mEngineReceiving)
  {
    
    
    if(mPtrViENetwork->ReceivedRTPPacket(mChannel, data, len, webrtc::PacketTime()) == -1)
    {
      int error = mPtrViEBase->LastError();
      CSFLogError(logTag, "%s RTP Processing Failed %d ", __FUNCTION__, error);
      if(error >= kViERtpRtcpInvalidChannelId && error <= kViERtpRtcpRtcpDisabled)
      {
        return kMediaConduitRTPProcessingFailed;
      }
      return kMediaConduitRTPRTCPModuleError;
    }
  } else {
    CSFLogError(logTag, "Error: %s when not receiving", __FUNCTION__);
    return kMediaConduitSessionNotInited;
  }

  return kMediaConduitNoError;
}

MediaConduitErrorCode
WebrtcVideoConduit::ReceivedRTCPPacket(const void *data, int len)
{
  CSFLogDebug(logTag, " %s Channel %d, Len %d ", __FUNCTION__, mChannel, len);

  
  if(mPtrViENetwork->ReceivedRTCPPacket(mChannel,data,len) == -1)
  {
    int error = mPtrViEBase->LastError();
    CSFLogError(logTag, "%s RTP Processing Failed %d", __FUNCTION__, error);
    if(error >= kViERtpRtcpInvalidChannelId && error <= kViERtpRtcpRtcpDisabled)
    {
      return kMediaConduitRTPProcessingFailed;
    }
    return kMediaConduitRTPRTCPModuleError;
  }
  return kMediaConduitNoError;
}


int WebrtcVideoConduit::SendPacket(int channel, const void* data, int len)
{
  CSFLogDebug(logTag,  "%s : channel %d len %d %s", __FUNCTION__, channel, len,
              (mEngineReceiving && mOtherDirection) ? "(using mOtherDirection)" : "");

  if (mEngineReceiving)
  {
    if (mOtherDirection)
    {
      return mOtherDirection->SendPacket(channel, data, len);
    }
    CSFLogDebug(logTag,  "%s : Asked to send RTP without an RTP sender on channel %d",
                __FUNCTION__, channel);
    return -1;
  } else {
    if(mTransport && (mTransport->SendRtpPacket(data, len) == NS_OK))
    {
      CSFLogDebug(logTag, "%s Sent RTP Packet ", __FUNCTION__);
      return len;
    } else {
      CSFLogError(logTag, "%s RTP Packet Send Failed ", __FUNCTION__);
      return -1;
    }
  }
}

int WebrtcVideoConduit::SendRTCPPacket(int channel, const void* data, int len)
{
  CSFLogDebug(logTag,  "%s : channel %d , len %d ", __FUNCTION__, channel,len);

  if (mEngineTransmitting)
  {
    if (mOtherDirection)
    {
      return mOtherDirection->SendRTCPPacket(channel, data, len);
    }
  }

  
  
  
  if(mTransport && mTransport->SendRtcpPacket(data, len) == NS_OK)
  {
    CSFLogDebug(logTag, "%s Sent RTCP Packet ", __FUNCTION__);
    return len;
  } else {
    CSFLogError(logTag, "%s RTCP Packet Send Failed ", __FUNCTION__);
    return -1;
  }
}


int
WebrtcVideoConduit::FrameSizeChange(unsigned int width,
                                    unsigned int height,
                                    unsigned int numStreams)
{
  CSFLogDebug(logTag,  "%s ", __FUNCTION__);


  mReceivingWidth = width;
  mReceivingHeight = height;

  if(mRenderer)
  {
    mRenderer->FrameSizeChange(width, height, numStreams);
    return 0;
  }

  CSFLogError(logTag,  "%s Renderer is NULL ", __FUNCTION__);
  return -1;
}

int
WebrtcVideoConduit::DeliverFrame(unsigned char* buffer,
                                 int buffer_size,
                                 uint32_t time_stamp,
                                 int64_t render_time,
                                 void *handle)
{
  CSFLogDebug(logTag,  "%s Buffer Size %d", __FUNCTION__, buffer_size);

  if(mRenderer)
  {
    layers::Image* img = nullptr;
    
    if (handle) {
      webrtc::NativeHandle* native_h = static_cast<webrtc::NativeHandle*>(handle);
      
      img = static_cast<layers::Image*>(native_h->GetHandle());
    }

    if (mVideoLatencyTestEnable && mReceivingWidth && mReceivingHeight) {
      uint64_t now = PR_Now();
      uint64_t timestamp = 0;
      bool ok = YuvStamper::Decode(mReceivingWidth, mReceivingHeight, mReceivingWidth,
                                   buffer,
                                   reinterpret_cast<unsigned char*>(&timestamp),
                                   sizeof(timestamp), 0, 0);
      if (ok) {
        VideoLatencyUpdate(now - timestamp);
      }
    }

    const ImageHandle img_h(img);
    mRenderer->RenderVideoFrame(buffer, buffer_size, time_stamp, render_time,
                                img_h);
    return 0;
  }

  CSFLogError(logTag,  "%s Renderer is NULL  ", __FUNCTION__);
  return -1;
}





void
WebrtcVideoConduit::CodecConfigToWebRTCCodec(const VideoCodecConfig* codecInfo,
                                              webrtc::VideoCodec& cinst)
{
  
  
  
  cinst.plType  = codecInfo->mType;
  if (codecInfo->mName == "H264_P0" || codecInfo->mName == "H264_P1") {
    cinst.codecType = webrtc::kVideoCodecH264;
    PL_strncpyz(cinst.plName, "H264", sizeof(cinst.plName));
  } else if (codecInfo->mName == "VP8") {
    cinst.codecType = webrtc::kVideoCodecVP8;
    PL_strncpyz(cinst.plName, "VP8", sizeof(cinst.plName));
  } else if (codecInfo->mName == "I420") {
    cinst.codecType = webrtc::kVideoCodecI420;
    PL_strncpyz(cinst.plName, "I420", sizeof(cinst.plName));
  } else {
    cinst.codecType = webrtc::kVideoCodecUnknown;
    PL_strncpyz(cinst.plName, "Unknown", sizeof(cinst.plName));
  }

  
  
  if (codecInfo->mMaxFrameRate > 0) {
    cinst.maxFramerate = codecInfo->mMaxFrameRate;
  } else {
    cinst.maxFramerate = DEFAULT_VIDEO_MAX_FRAMERATE;
  }

  cinst.minBitrate = mMinBitrate;
  cinst.startBitrate = mStartBitrate;
  cinst.maxBitrate = mMaxBitrate;

  if (cinst.codecType == webrtc::kVideoCodecH264)
  {
#ifdef MOZ_WEBRTC_OMX
    cinst.resolution_divisor = 16;
#endif
    cinst.codecSpecific.H264.profile = codecInfo->mProfile;
    cinst.codecSpecific.H264.constraints = codecInfo->mConstraints;
    cinst.codecSpecific.H264.level = codecInfo->mLevel;
    cinst.codecSpecific.H264.packetizationMode = codecInfo->mPacketizationMode;
    if (codecInfo->mMaxBitrate > 0 && codecInfo->mMaxBitrate < cinst.maxBitrate) {
      cinst.maxBitrate = codecInfo->mMaxBitrate;
    }
    if (codecInfo->mMaxMBPS > 0) {
      
      CSFLogError(logTag,  "%s H.264 max_mbps not supported yet  ", __FUNCTION__);
    }
    
    
    cinst.codecSpecific.H264.spsData = nullptr;
    cinst.codecSpecific.H264.spsLen = 0;
    cinst.codecSpecific.H264.ppsData = nullptr;
    cinst.codecSpecific.H264.ppsLen = 0;
  }
}


bool
WebrtcVideoConduit::CopyCodecToDB(const VideoCodecConfig* codecInfo)
{
  VideoCodecConfig* cdcConfig = new VideoCodecConfig(*codecInfo);
  mRecvCodecList.push_back(cdcConfig);
  return true;
}

bool
WebrtcVideoConduit::CheckCodecsForMatch(const VideoCodecConfig* curCodecConfig,
                                        const VideoCodecConfig* codecInfo) const
{
  if(!curCodecConfig)
  {
    return false;
  }

  if(curCodecConfig->mType  == codecInfo->mType &&
     curCodecConfig->mName.compare(codecInfo->mName) == 0 &&
     curCodecConfig->mMaxFrameSize == codecInfo->mMaxFrameSize &&
     curCodecConfig->mMaxFrameRate == codecInfo->mMaxFrameRate)
  {
    return true;
  }

  return false;
}




bool
WebrtcVideoConduit::CheckCodecForMatch(const VideoCodecConfig* codecInfo) const
{
  
  for(std::vector<VideoCodecConfig*>::size_type i=0;i < mRecvCodecList.size();i++)
  {
    if(CheckCodecsForMatch(mRecvCodecList[i],codecInfo))
    {
      
      return true;
    }
  }
  
  return false;
}





MediaConduitErrorCode
WebrtcVideoConduit::ValidateCodecConfig(const VideoCodecConfig* codecInfo,
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

  
  if(send)
  {
    codecAppliedAlready = CheckCodecsForMatch(mCurSendCodecConfig,codecInfo);
  } else {
    codecAppliedAlready = CheckCodecForMatch(codecInfo);
  }

  if(codecAppliedAlready)
  {
    CSFLogDebug(logTag, "%s Codec %s Already Applied  ", __FUNCTION__, codecInfo->mName.c_str());
    return kMediaConduitCodecInUse;
  }
  return kMediaConduitNoError;
}

void
WebrtcVideoConduit::DumpCodecDB() const
{
  for(std::vector<VideoCodecConfig*>::size_type i=0;i<mRecvCodecList.size();i++)
  {
    CSFLogDebug(logTag,"Payload Name: %s", mRecvCodecList[i]->mName.c_str());
    CSFLogDebug(logTag,"Payload Type: %d", mRecvCodecList[i]->mType);
    CSFLogDebug(logTag,"Payload Max Frame Size: %d", mRecvCodecList[i]->mMaxFrameSize);
    CSFLogDebug(logTag,"Payload Max Frame Rate: %d", mRecvCodecList[i]->mMaxFrameRate);
  }
}

void
WebrtcVideoConduit::VideoLatencyUpdate(uint64_t newSample)
{
  mVideoLatencyAvg = (sRoundingPadding * newSample + sAlphaNum * mVideoLatencyAvg) / sAlphaDen;
}

uint64_t
WebrtcVideoConduit::MozVideoLatencyAvg()
{
  return mVideoLatencyAvg / sRoundingPadding;
}

uint64_t
WebrtcVideoConduit::CodecPluginID()
{
  if (mExternalSendCodecHandle) {
    return mExternalSendCodecHandle->PluginID();
  } else if (mExternalRecvCodecHandle) {
    return mExternalRecvCodecHandle->PluginID();
  }
  return 0;
}

}
