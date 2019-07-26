



#ifndef MEDIAENGINEWEBRTC_H_
#define MEDIAENGINEWEBRTC_H_

#include "prcvar.h"
#include "prthread.h"
#include "nsIThread.h"
#include "nsIRunnable.h"

#include "mozilla/Mutex.h"
#include "nsCOMPtr.h"
#include "nsDOMFile.h"
#include "nsThreadUtils.h"
#include "DOMMediaStream.h"
#include "nsDirectoryServiceDefs.h"
#include "nsComponentManagerUtils.h"

#include "VideoUtils.h"
#include "MediaEngine.h"
#include "VideoSegment.h"
#include "AudioSegment.h"
#include "StreamBuffer.h"
#include "MediaStreamGraph.h"




#include "voice_engine/include/voe_base.h"
#include "voice_engine/include/voe_codec.h"
#include "voice_engine/include/voe_hardware.h"
#include "voice_engine/include/voe_network.h"
#include "voice_engine/include/voe_audio_processing.h"
#include "voice_engine/include/voe_volume_control.h"
#include "voice_engine/include/voe_external_media.h"
#include "voice_engine/include/voe_audio_processing.h"


#include "video_engine/include/vie_base.h"
#include "video_engine/include/vie_codec.h"
#include "video_engine/include/vie_render.h"
#include "video_engine/include/vie_capture.h"
#include "video_engine/include/vie_file.h"

#include "NullTransport.h"

namespace mozilla {




class MediaEngineWebRTCVideoSource : public MediaEngineVideoSource,
                                     public webrtc::ExternalRenderer,
                                     public nsRunnable
{
public:
  static const int DEFAULT_VIDEO_FPS = 60;
  static const int DEFAULT_VIDEO_MIN_FPS = 10;
  static const int DEFAULT_VIDEO_WIDTH = 640;
  static const int DEFAULT_VIDEO_HEIGHT = 480;

  
  virtual int FrameSizeChange(unsigned int, unsigned int, unsigned int);
  virtual int DeliverFrame(unsigned char*, int, uint32_t, int64_t);

  MediaEngineWebRTCVideoSource(webrtc::VideoEngine* aVideoEnginePtr,
    int aIndex,
    int aWidth = DEFAULT_VIDEO_WIDTH, int aHeight = DEFAULT_VIDEO_HEIGHT,
    int aFps = DEFAULT_VIDEO_FPS, int aMinFps = DEFAULT_VIDEO_MIN_FPS)
    : mVideoEngine(aVideoEnginePtr)
    , mCaptureIndex(aIndex)
    , mCapabilityChosen(false)
    , mWidth(aWidth)
    , mHeight(aHeight)
    , mMonitor("WebRTCCamera.Monitor")
    , mFps(aFps)
    , mMinFps(aMinFps)
    , mInitDone(false)
    , mInSnapshotMode(false)
    , mSnapshotPath(NULL) {
    MOZ_ASSERT(aVideoEnginePtr);
    mState = kReleased;
    Init();
  }
  ~MediaEngineWebRTCVideoSource() { Shutdown(); }

  virtual void GetName(nsAString&);
  virtual void GetUUID(nsAString&);
  virtual const MediaEngineVideoOptions *GetOptions();
  virtual nsresult Allocate();
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

  NS_DECL_ISUPPORTS

  
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

  

  webrtc::VideoEngine* mVideoEngine; 
  webrtc::ViEBase* mViEBase;
  webrtc::ViECapture* mViECapture;
  webrtc::ViERender* mViERender;
  webrtc::CaptureCapability mCapability; 

  int mCaptureIndex;
  bool mCapabilityChosen;
  int mWidth, mHeight;

  
  
  
  
  mozilla::ReentrantMonitor mMonitor; 
  nsTArray<SourceMediaStream *> mSources; 

  int mFps; 
  int mMinFps; 
  bool mInitDone;
  bool mInSnapshotMode;
  nsString* mSnapshotPath;

  nsRefPtr<layers::Image> mImage;
  nsRefPtr<layers::ImageContainer> mImageContainer;

  PRLock* mSnapshotLock;
  PRCondVar* mSnapshotCondVar;

  
  char mDeviceName[KMaxDeviceNameLength];
  char mUniqueId[KMaxUniqueIdLength];

  void ChooseCapability(uint32_t aWidth, uint32_t aHeight, uint32_t aMinFPS);
  MediaEngineVideoOptions mOpts;
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

  virtual nsresult Allocate();
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

  
  void Process(const int channel, const webrtc::ProcessingTypes type,
               WebRtc_Word16 audio10ms[], const int length,
               const int samplingFreq, const bool isStereo);

  NS_DECL_ISUPPORTS

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

  
  
  
  mozilla::ReentrantMonitor mMonitor;
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
  MediaEngineWebRTC()
  : mMutex("mozilla::MediaEngineWebRTC")
  , mVideoEngine(NULL)
  , mVoiceEngine(NULL)
  , mVideoEngineInit(false)
  , mAudioEngineInit(false)
  {
    mVideoSources.Init();
    mAudioSources.Init();
  }
  ~MediaEngineWebRTC() { Shutdown(); }

  
  
  void Shutdown();

  virtual void EnumerateVideoDevices(nsTArray<nsRefPtr<MediaEngineVideoSource> >*);
  virtual void EnumerateAudioDevices(nsTArray<nsRefPtr<MediaEngineAudioSource> >*);

private:
  Mutex mMutex;
  

  webrtc::VideoEngine* mVideoEngine;
  webrtc::VoiceEngine* mVoiceEngine;

  
  bool mVideoEngineInit;
  bool mAudioEngineInit;

  
  int mHeight, mWidth, mFPS, mMinFPS;

  
  
  nsRefPtrHashtable<nsStringHashKey, MediaEngineWebRTCVideoSource > mVideoSources;
  nsRefPtrHashtable<nsStringHashKey, MediaEngineWebRTCAudioSource > mAudioSources;
};

}

#endif 
