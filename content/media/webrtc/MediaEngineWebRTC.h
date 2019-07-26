



#ifndef MEDIAENGINEWEBRTC_H_
#define MEDIAENGINEWEBRTC_H_

#include "prcvar.h"
#include "prthread.h"
#include "nsIThread.h"
#include "nsIRunnable.h"

#include "mozilla/Mutex.h"
#include "mozilla/Monitor.h"
#include "nsCOMPtr.h"
#include "nsDOMFile.h"
#include "nsThreadUtils.h"
#include "DOMMediaStream.h"
#include "nsDirectoryServiceDefs.h"
#include "nsComponentManagerUtils.h"
#include "nsRefPtrHashtable.h"

#include "VideoUtils.h"
#include "MediaEngine.h"
#include "VideoSegment.h"
#include "AudioSegment.h"
#include "StreamBuffer.h"
#include "MediaStreamGraph.h"
#include "LoadMonitor.h"




#include "webrtc/voice_engine/include/voe_base.h"
#include "webrtc/voice_engine/include/voe_codec.h"
#include "webrtc/voice_engine/include/voe_hardware.h"
#include "webrtc/voice_engine/include/voe_network.h"
#include "webrtc/voice_engine/include/voe_audio_processing.h"
#include "webrtc/voice_engine/include/voe_volume_control.h"
#include "webrtc/voice_engine/include/voe_external_media.h"
#include "webrtc/voice_engine/include/voe_audio_processing.h"


#include "webrtc/video_engine/include/vie_base.h"
#include "webrtc/video_engine/include/vie_codec.h"
#include "webrtc/video_engine/include/vie_render.h"
#include "webrtc/video_engine/include/vie_capture.h"
#ifdef MOZ_B2G_CAMERA
#include "CameraControlListener.h"
#include "ICameraControl.h"
#include "ImageContainer.h"
#include "nsGlobalWindow.h"
#include "prprf.h"
#include "mozilla/Hal.h"
#endif

#include "NullTransport.h"

namespace mozilla {

#ifdef MOZ_B2G_CAMERA
class CameraAllocateRunnable;
class GetCameraNameRunnable;
#endif


















class MediaEngineWebRTCVideoSource : public MediaEngineVideoSource
                                   , public nsRunnable
#ifdef MOZ_B2G_CAMERA
                                   , public CameraControlListener
                                   , public mozilla::hal::ScreenConfigurationObserver
#else
                                   , public webrtc::ExternalRenderer
#endif
{
public:
#ifdef MOZ_B2G_CAMERA
  MediaEngineWebRTCVideoSource(int aIndex)
    : mCameraControl(nullptr)
    , mCallbackMonitor("WebRTCCamera.CallbackMonitor")
    , mRotation(0)
    , mBackCamera(false)
    , mCaptureIndex(aIndex)
    , mMonitor("WebRTCCamera.Monitor")
    , mWidth(0)
    , mHeight(0)
    , mInitDone(false)
    , mInSnapshotMode(false)
    , mSnapshotPath(nullptr)
  {
    mState = kReleased;
    Init();
  }
#else

  virtual int FrameSizeChange(unsigned int, unsigned int, unsigned int);
  virtual int DeliverFrame(unsigned char*,int, uint32_t , int64_t,
                           void *handle);
  




  virtual bool IsTextureSupported() { return false; }

  MediaEngineWebRTCVideoSource(webrtc::VideoEngine* aVideoEnginePtr, int aIndex)
    : mVideoEngine(aVideoEnginePtr)
    , mCaptureIndex(aIndex)
    , mFps(-1)
    , mMinFps(-1)
    , mMonitor("WebRTCCamera.Monitor")
    , mWidth(0)
    , mHeight(0)
    , mInitDone(false)
    , mInSnapshotMode(false)
    , mSnapshotPath(nullptr) {
    MOZ_ASSERT(aVideoEnginePtr);
    mState = kReleased;
    Init();
  }
#endif

  ~MediaEngineWebRTCVideoSource() { Shutdown(); }

  virtual void GetName(nsAString&);
  virtual void GetUUID(nsAString&);
  virtual nsresult Allocate(const MediaEnginePrefs &aPrefs);
  virtual nsresult Deallocate();
  virtual nsresult Start(SourceMediaStream*, TrackID);
  virtual nsresult Stop(SourceMediaStream*, TrackID);
  virtual nsresult Snapshot(uint32_t aDuration, nsIDOMFile** aFile);
  virtual nsresult Config(bool aEchoOn, uint32_t aEcho,
                          bool aAgcOn, uint32_t aAGC,
                          bool aNoiseOn, uint32_t aNoise) { return NS_OK; };
  virtual void NotifyPull(MediaStreamGraph* aGraph,
                          SourceMediaStream *aSource,
                          TrackID aId,
                          StreamTime aDesiredTime,
                          TrackTicks &aLastEndTime);

  virtual bool IsFake() {
    return false;
  }

#ifndef MOZ_B2G_CAMERA
  NS_DECL_THREADSAFE_ISUPPORTS
#else
  
  
  NS_DECL_ISUPPORTS_INHERITED

  void OnHardwareStateChange(HardwareState aState);
  bool OnNewPreviewFrame(layers::Image* aImage, uint32_t aWidth, uint32_t aHeight);
  void OnError(CameraErrorContext aContext, CameraError aError);
  void OnTakePictureComplete(uint8_t* aData, uint32_t aLength, const nsAString& aMimeType);

  void AllocImpl();
  void DeallocImpl();
  void StartImpl(webrtc::CaptureCapability aCapability);
  void StopImpl();
  void SnapshotImpl();
  void RotateImage(layers::Image* aImage, uint32_t aWidth, uint32_t aHeight);
  void Notify(const mozilla::hal::ScreenConfiguration& aConfiguration);
#endif

  
  NS_IMETHODIMP
  Run()
  {
    nsCOMPtr<nsIFile> tmp;
    nsresult rv = NS_GetSpecialDirectory(NS_OS_TEMP_DIR, getter_AddRefs(tmp));
    NS_ENSURE_SUCCESS(rv, rv);

    tmp->Append(NS_LITERAL_STRING("webrtc_snapshot.jpeg"));
    rv = tmp->CreateUnique(nsIFile::NORMAL_FILE_TYPE, 0600);
    NS_ENSURE_SUCCESS(rv, rv);

    mSnapshotPath = new nsString();
    rv = tmp->GetPath(*mSnapshotPath);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
  }

private:
  static const unsigned int KMaxDeviceNameLength = 128;
  static const unsigned int KMaxUniqueIdLength = 256;

  
  void Init();
  void Shutdown();

  
#ifdef MOZ_B2G_CAMERA
  nsRefPtr<ICameraControl> mCameraControl;
  mozilla::ReentrantMonitor mCallbackMonitor; 
  nsRefPtr<nsIDOMFile> mLastCapture;
  int mRotation;
  int mCameraAngle; 
  bool mBackCamera;
#else
  webrtc::VideoEngine* mVideoEngine; 
  webrtc::ViEBase* mViEBase;
  webrtc::ViECapture* mViECapture;
  webrtc::ViERender* mViERender;
#endif
  webrtc::CaptureCapability mCapability; 

  int mCaptureIndex;
  int mFps; 
  int mMinFps; 

  
  
  
  
  Monitor mMonitor; 
  int mWidth, mHeight;
  nsRefPtr<layers::Image> mImage;
  nsRefPtr<layers::ImageContainer> mImageContainer;

  nsTArray<SourceMediaStream *> mSources; 

  bool mInitDone;
  bool mInSnapshotMode;
  nsString* mSnapshotPath;

  nsString mDeviceName;
  nsString mUniqueId;

  void ChooseCapability(const MediaEnginePrefs &aPrefs);
};

class MediaEngineWebRTCAudioSource : public MediaEngineAudioSource,
                                     public webrtc::VoEMediaProcess
{
public:
  MediaEngineWebRTCAudioSource(webrtc::VoiceEngine* aVoiceEnginePtr, int aIndex,
    const char* name, const char* uuid)
    : mVoiceEngine(aVoiceEnginePtr)
    , mMonitor("WebRTCMic.Monitor")
    , mCapIndex(aIndex)
    , mChannel(-1)
    , mInitDone(false)
    , mEchoOn(false), mAgcOn(false), mNoiseOn(false)
    , mEchoCancel(webrtc::kEcDefault)
    , mAGC(webrtc::kAgcDefault)
    , mNoiseSuppress(webrtc::kNsDefault)
    , mNullTransport(nullptr) {
    MOZ_ASSERT(aVoiceEnginePtr);
    mState = kReleased;
    mDeviceName.Assign(NS_ConvertUTF8toUTF16(name));
    mDeviceUUID.Assign(NS_ConvertUTF8toUTF16(uuid));
    Init();
  }
  ~MediaEngineWebRTCAudioSource() { Shutdown(); }

  virtual void GetName(nsAString&);
  virtual void GetUUID(nsAString&);

  virtual nsresult Allocate(const MediaEnginePrefs &aPrefs);
  virtual nsresult Deallocate();
  virtual nsresult Start(SourceMediaStream*, TrackID);
  virtual nsresult Stop(SourceMediaStream*, TrackID);
  virtual nsresult Snapshot(uint32_t aDuration, nsIDOMFile** aFile);
  virtual nsresult Config(bool aEchoOn, uint32_t aEcho,
                          bool aAgcOn, uint32_t aAGC,
                          bool aNoiseOn, uint32_t aNoise);

  virtual void NotifyPull(MediaStreamGraph* aGraph,
                          SourceMediaStream *aSource,
                          TrackID aId,
                          StreamTime aDesiredTime,
                          TrackTicks &aLastEndTime);

  virtual bool IsFake() {
    return false;
  }

  
  void Process(int channel, webrtc::ProcessingTypes type,
               int16_t audio10ms[], int length,
               int samplingFreq, bool isStereo);

  NS_DECL_THREADSAFE_ISUPPORTS

private:
  static const unsigned int KMaxDeviceNameLength = 128;
  static const unsigned int KMaxUniqueIdLength = 256;

  void Init();
  void Shutdown();

  webrtc::VoiceEngine* mVoiceEngine;
  webrtc::VoEBase* mVoEBase;
  webrtc::VoEExternalMedia* mVoERender;
  webrtc::VoENetwork*  mVoENetwork;
  webrtc::VoEAudioProcessing *mVoEProcessing;

  
  
  
  Monitor mMonitor;
  nsTArray<SourceMediaStream *> mSources; 

  int mCapIndex;
  int mChannel;
  TrackID mTrackID;
  bool mInitDone;

  nsString mDeviceName;
  nsString mDeviceUUID;

  bool mEchoOn, mAgcOn, mNoiseOn;
  webrtc::EcModes  mEchoCancel;
  webrtc::AgcModes mAGC;
  webrtc::NsModes  mNoiseSuppress;

  NullTransport *mNullTransport;
};

class MediaEngineWebRTC : public MediaEngine
{
public:
  MediaEngineWebRTC(MediaEnginePrefs &aPrefs);
  ~MediaEngineWebRTC() {
    Shutdown();
#ifdef MOZ_B2G_CAMERA
    AsyncLatencyLogger::Get()->Release();
#endif
  }

  
  
  void Shutdown();

  virtual void EnumerateVideoDevices(nsTArray<nsRefPtr<MediaEngineVideoSource> >*);
  virtual void EnumerateAudioDevices(nsTArray<nsRefPtr<MediaEngineAudioSource> >*);

private:
  Mutex mMutex;
  

  webrtc::VideoEngine* mVideoEngine;
  webrtc::VoiceEngine* mVoiceEngine;

  
  bool mVideoEngineInit;
  bool mAudioEngineInit;
  bool mHasTabVideoSource;

  
  
  nsRefPtrHashtable<nsStringHashKey, MediaEngineWebRTCVideoSource > mVideoSources;
  nsRefPtrHashtable<nsStringHashKey, MediaEngineWebRTCAudioSource > mAudioSources;

  nsRefPtr<LoadMonitor> mLoadMonitor;
};

}

#endif 
