



#ifndef MEDIAENGINEWEBRTC_H_
#define MEDIAENGINEWEBRTC_H_

#include "prcvar.h"
#include "prthread.h"
#include "nsIThread.h"
#include "nsIRunnable.h"

#include "mozilla/dom/File.h"
#include "mozilla/Mutex.h"
#include "mozilla/Monitor.h"
#include "nsCOMPtr.h"
#include "nsThreadUtils.h"
#include "DOMMediaStream.h"
#include "nsDirectoryServiceDefs.h"
#include "nsComponentManagerUtils.h"
#include "nsRefPtrHashtable.h"

#include "VideoUtils.h"
#include "MediaEngineCameraVideoSource.h"
#include "VideoSegment.h"
#include "AudioSegment.h"
#include "StreamBuffer.h"
#include "MediaStreamGraph.h"

#include "MediaEngineWrapper.h"
#include "mozilla/dom/MediaStreamTrackBinding.h"

#include "webrtc/common.h"

#include "webrtc/voice_engine/include/voe_base.h"
#include "webrtc/voice_engine/include/voe_codec.h"
#include "webrtc/voice_engine/include/voe_hardware.h"
#include "webrtc/voice_engine/include/voe_network.h"
#include "webrtc/voice_engine/include/voe_audio_processing.h"
#include "webrtc/voice_engine/include/voe_volume_control.h"
#include "webrtc/voice_engine/include/voe_external_media.h"
#include "webrtc/voice_engine/include/voe_audio_processing.h"



#undef FF
#include "webrtc/video_engine/include/vie_base.h"
#include "webrtc/video_engine/include/vie_codec.h"
#include "webrtc/video_engine/include/vie_render.h"
#include "webrtc/video_engine/include/vie_capture.h"

#include "NullTransport.h"
#include "AudioOutputObserver.h"

namespace mozilla {




class MediaEngineWebRTCVideoSource : public MediaEngineCameraVideoSource
                                   , public webrtc::ExternalRenderer
{
public:
  NS_DECL_THREADSAFE_ISUPPORTS

  
  virtual int FrameSizeChange(unsigned int w, unsigned int h, unsigned int streams) override;
  virtual int DeliverFrame(unsigned char* buffer,
                           int size,
                           uint32_t time_stamp,
                           int64_t ntp_time_ms,
                           int64_t render_time,
                           void *handle) override;
  




  virtual bool IsTextureSupported() override { return false; }

  MediaEngineWebRTCVideoSource(webrtc::VideoEngine* aVideoEnginePtr, int aIndex,
                               dom::MediaSourceEnum aMediaSource = dom::MediaSourceEnum::Camera)
    : MediaEngineCameraVideoSource(aIndex, "WebRTCCamera.Monitor")
    , mVideoEngine(aVideoEnginePtr)
    , mMinFps(-1)
    , mMediaSource(aMediaSource)
  {
    MOZ_ASSERT(aVideoEnginePtr);
    MOZ_ASSERT(aMediaSource != dom::MediaSourceEnum::Other);
    Init();
  }

  virtual nsresult Allocate(const dom::MediaTrackConstraints& aConstraints,
                            const MediaEnginePrefs& aPrefs,
                            const nsString& aDeviceId) override;
  virtual nsresult Deallocate() override;
  virtual nsresult Start(SourceMediaStream*, TrackID) override;
  virtual nsresult Stop(SourceMediaStream*, TrackID) override;
  virtual void NotifyPull(MediaStreamGraph* aGraph,
                          SourceMediaStream* aSource,
                          TrackID aId,
                          StreamTime aDesiredTime) override;

  virtual const dom::MediaSourceEnum GetMediaSource() override {
    return mMediaSource;
  }
  virtual nsresult TakePhoto(PhotoCallback* aCallback) override
  {
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  void Refresh(int aIndex);

  virtual void Shutdown() override;

protected:
  ~MediaEngineWebRTCVideoSource() { Shutdown(); }

private:
  
  void Init();

  
  webrtc::VideoEngine* mVideoEngine; 
  ScopedCustomReleasePtr<webrtc::ViEBase> mViEBase;
  ScopedCustomReleasePtr<webrtc::ViECapture> mViECapture;
  ScopedCustomReleasePtr<webrtc::ViERender> mViERender;

  int mMinFps; 
  dom::MediaSourceEnum mMediaSource; 

  size_t NumCapabilities() override;
  void GetCapability(size_t aIndex, webrtc::CaptureCapability& aOut) override;
};

class MediaEngineWebRTCAudioSource : public MediaEngineAudioSource,
                                     public webrtc::VoEMediaProcess,
                                     private MediaConstraintsHelper
{
public:
  MediaEngineWebRTCAudioSource(nsIThread* aThread, webrtc::VoiceEngine* aVoiceEnginePtr,
                               int aIndex, const char* name, const char* uuid)
    : MediaEngineAudioSource(kReleased)
    , mVoiceEngine(aVoiceEnginePtr)
    , mMonitor("WebRTCMic.Monitor")
    , mThread(aThread)
    , mCapIndex(aIndex)
    , mChannel(-1)
    , mInitDone(false)
    , mStarted(false)
    , mEchoOn(false), mAgcOn(false), mNoiseOn(false)
    , mEchoCancel(webrtc::kEcDefault)
    , mAGC(webrtc::kAgcDefault)
    , mNoiseSuppress(webrtc::kNsDefault)
    , mPlayoutDelay(0)
    , mNullTransport(nullptr) {
    MOZ_ASSERT(aVoiceEnginePtr);
    mDeviceName.Assign(NS_ConvertUTF8toUTF16(name));
    mDeviceUUID.Assign(uuid);
    Init();
  }

  virtual void GetName(nsAString& aName) override;
  virtual void GetUUID(nsACString& aUUID) override;

  virtual nsresult Allocate(const dom::MediaTrackConstraints& aConstraints,
                            const MediaEnginePrefs& aPrefs,
                            const nsString& aDeviceId) override;
  virtual nsresult Deallocate() override;
  virtual nsresult Start(SourceMediaStream* aStream, TrackID aID) override;
  virtual nsresult Stop(SourceMediaStream* aSource, TrackID aID) override;
  virtual void SetDirectListeners(bool aHasDirectListeners) override {};
  virtual nsresult Config(bool aEchoOn, uint32_t aEcho,
                          bool aAgcOn, uint32_t aAGC,
                          bool aNoiseOn, uint32_t aNoise,
                          int32_t aPlayoutDelay) override;

  virtual void NotifyPull(MediaStreamGraph* aGraph,
                          SourceMediaStream* aSource,
                          TrackID aId,
                          StreamTime aDesiredTime) override;

  virtual bool IsFake() override {
    return false;
  }

  virtual const dom::MediaSourceEnum GetMediaSource() override {
    return dom::MediaSourceEnum::Microphone;
  }

  virtual nsresult TakePhoto(PhotoCallback* aCallback) override
  {
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  virtual uint32_t GetBestFitnessDistance(
      const nsTArray<const dom::MediaTrackConstraintSet*>& aConstraintSets,
      const nsString& aDeviceId) override;

  
  void Process(int channel, webrtc::ProcessingTypes type,
               int16_t audio10ms[], int length,
               int samplingFreq, bool isStereo) override;

  NS_DECL_THREADSAFE_ISUPPORTS

  virtual void Shutdown() override;

protected:
  ~MediaEngineWebRTCAudioSource() { Shutdown(); }

private:
  void Init();

  webrtc::VoiceEngine* mVoiceEngine;
  ScopedCustomReleasePtr<webrtc::VoEBase> mVoEBase;
  ScopedCustomReleasePtr<webrtc::VoEExternalMedia> mVoERender;
  ScopedCustomReleasePtr<webrtc::VoENetwork> mVoENetwork;
  ScopedCustomReleasePtr<webrtc::VoEAudioProcessing> mVoEProcessing;

  
  
  
  Monitor mMonitor;
  nsTArray<nsRefPtr<SourceMediaStream>> mSources; 
  nsCOMPtr<nsIThread> mThread;
  int mCapIndex;
  int mChannel;
  TrackID mTrackID;
  bool mInitDone;
  bool mStarted;

  nsString mDeviceName;
  nsCString mDeviceUUID;

  bool mEchoOn, mAgcOn, mNoiseOn;
  webrtc::EcModes  mEchoCancel;
  webrtc::AgcModes mAGC;
  webrtc::NsModes  mNoiseSuppress;
  int32_t mPlayoutDelay;

  NullTransport *mNullTransport;
};

class MediaEngineWebRTC : public MediaEngine
{
public:
  explicit MediaEngineWebRTC(MediaEnginePrefs& aPrefs);

  
  
  void Shutdown() override;

  virtual void EnumerateVideoDevices(dom::MediaSourceEnum,
                                     nsTArray<nsRefPtr<MediaEngineVideoSource>>*) override;
  virtual void EnumerateAudioDevices(dom::MediaSourceEnum,
                                     nsTArray<nsRefPtr<MediaEngineAudioSource>>*) override;
private:
  ~MediaEngineWebRTC() {
    Shutdown();
#if defined(MOZ_B2G_CAMERA) && defined(MOZ_WIDGET_GONK)
    AsyncLatencyLogger::Get()->Release();
#endif
    gFarendObserver = nullptr;
  }

  nsCOMPtr<nsIThread> mThread;

  Mutex mMutex;

  
  webrtc::VideoEngine* mScreenEngine;
  webrtc::VideoEngine* mBrowserEngine;
  webrtc::VideoEngine* mWinEngine;
  webrtc::VideoEngine* mAppEngine;
  webrtc::VideoEngine* mVideoEngine;
  webrtc::VoiceEngine* mVoiceEngine;

  
  webrtc::Config mAppEngineConfig;
  webrtc::Config mWinEngineConfig;
  webrtc::Config mScreenEngineConfig;
  webrtc::Config mBrowserEngineConfig;

  
  bool mVideoEngineInit;
  bool mAudioEngineInit;
  bool mScreenEngineInit;
  bool mBrowserEngineInit;
  bool mWinEngineInit;
  bool mAppEngineInit;
  bool mHasTabVideoSource;

  
  
  nsRefPtrHashtable<nsStringHashKey, MediaEngineVideoSource> mVideoSources;
  nsRefPtrHashtable<nsStringHashKey, MediaEngineWebRTCAudioSource> mAudioSources;
};

}

#endif 
