



#include "MediaEngine.h"
#include "mozilla/Services.h"
#include "mozilla/unused.h"
#include "nsIMediaManager.h"

#include "nsHashKeys.h"
#include "nsGlobalWindow.h"
#include "nsClassHashtable.h"
#include "nsRefPtrHashtable.h"
#include "nsIObserver.h"
#include "nsIPrefService.h"
#include "nsIPrefBranch.h"

#include "nsPIDOMWindow.h"
#include "nsIDOMNavigatorUserMedia.h"
#include "nsXULAppAPI.h"
#include "mozilla/Attributes.h"
#include "mozilla/Preferences.h"
#include "mozilla/StaticPtr.h"
#include "mozilla/dom/MediaStreamBinding.h"
#include "mozilla/dom/MediaStreamTrackBinding.h"
#include "prlog.h"
#include "DOMMediaStream.h"

#ifdef MOZ_WEBRTC
#include "mtransport/runnable_utils.h"
#endif

#ifdef MOZ_WIDGET_GONK
#include "DOMCameraManager.h"
#endif

namespace mozilla {
namespace dom {
struct MediaStreamConstraints;
class NavigatorUserMediaSuccessCallback;
class NavigatorUserMediaErrorCallback;
}

#ifdef PR_LOGGING
extern PRLogModuleInfo* GetMediaManagerLog();
#define MM_LOG(msg) PR_LOG(GetMediaManagerLog(), PR_LOG_DEBUG, msg)
#else
#define MM_LOG(msg)
#endif






class GetUserMediaCallbackMediaStreamListener : public MediaStreamListener
{
public:
  
  GetUserMediaCallbackMediaStreamListener(nsIThread *aThread,
    uint64_t aWindowID)
    : mMediaThread(aThread)
    , mWindowID(aWindowID)
    , mStopped(false)
    , mFinished(false)
    , mLock("mozilla::GUMCMSL")
    , mRemoved(false) {}

  ~GetUserMediaCallbackMediaStreamListener()
  {
    
    
  }

  void Activate(already_AddRefed<SourceMediaStream> aStream,
    MediaEngineSource* aAudioSource,
    MediaEngineSource* aVideoSource)
  {
    NS_ASSERTION(NS_IsMainThread(), "Only call on main thread");
    mStream = aStream;
    mAudioSource = aAudioSource;
    mVideoSource = aVideoSource;
    mLastEndTimeAudio = 0;
    mLastEndTimeVideo = 0;

    mStream->AddListener(this);
  }

  MediaStream *Stream() 
  {
    return mStream;
  }
  SourceMediaStream *GetSourceStream()
  {
    NS_ASSERTION(mStream,"Getting stream from never-activated GUMCMSListener");
    if (!mStream) {
      return nullptr;
    }
    return mStream->AsSourceStream();
  }

  void StopScreenWindowSharing();

  void StopTrack(TrackID aID, bool aIsAudio);

  
  
  bool CapturingVideo()
  {
    NS_ASSERTION(NS_IsMainThread(), "Only call on main thread");
    return mVideoSource && !mStopped &&
           mVideoSource->GetMediaSource() == MediaSourceType::Camera &&
           (!mVideoSource->IsFake() ||
            Preferences::GetBool("media.navigator.permission.fake"));
  }
  bool CapturingAudio()
  {
    NS_ASSERTION(NS_IsMainThread(), "Only call on main thread");
    return mAudioSource && !mStopped &&
           (!mAudioSource->IsFake() ||
            Preferences::GetBool("media.navigator.permission.fake"));
  }
  bool CapturingScreen()
  {
    NS_ASSERTION(NS_IsMainThread(), "Only call on main thread");
    return mVideoSource && !mStopped && !mVideoSource->IsAvailable() &&
           mVideoSource->GetMediaSource() == MediaSourceType::Screen;
  }
  bool CapturingWindow()
  {
    NS_ASSERTION(NS_IsMainThread(), "Only call on main thread");
    return mVideoSource && !mStopped && !mVideoSource->IsAvailable() &&
           mVideoSource->GetMediaSource() == MediaSourceType::Window;
  }
  bool CapturingApplication()
  {
    NS_ASSERTION(NS_IsMainThread(), "Only call on main thread");
    return mVideoSource && !mStopped && !mVideoSource->IsAvailable() &&
           mVideoSource->GetMediaSource() == MediaSourceType::Application;
  }

  void SetStopped()
  {
    mStopped = true;
  }

  
  
  void Invalidate();

  void
  AudioConfig(bool aEchoOn, uint32_t aEcho,
              bool aAgcOn, uint32_t aAGC,
              bool aNoiseOn, uint32_t aNoise,
              int32_t aPlayoutDelay)
  {
    if (mAudioSource) {
#ifdef MOZ_WEBRTC
      
      RUN_ON_THREAD(mMediaThread,
                    WrapRunnable(nsRefPtr<MediaEngineSource>(mAudioSource), 
                                 &MediaEngineSource::Config,
                                 aEchoOn, aEcho, aAgcOn, aAGC, aNoiseOn, aNoise, aPlayoutDelay),
                    NS_DISPATCH_NORMAL);
#endif
    }
  }

  void
  Remove()
  {
    NS_ASSERTION(NS_IsMainThread(), "Only call on main thread");
    
    
    MutexAutoLock lock(mLock); 
    if (mStream && !mRemoved) {
      MM_LOG(("Listener removed on purpose, mFinished = %d", (int) mFinished));
      mRemoved = true; 
      
      if (!mStream->IsDestroyed()) {
        mStream->RemoveListener(this);
      }
    }
  }

  
  virtual void
  NotifyPull(MediaStreamGraph* aGraph, StreamTime aDesiredTime) MOZ_OVERRIDE
  {
    
    
    if (mAudioSource) {
      mAudioSource->NotifyPull(aGraph, mStream, kAudioTrack, aDesiredTime, mLastEndTimeAudio);
    }
    if (mVideoSource) {
      mVideoSource->NotifyPull(aGraph, mStream, kVideoTrack, aDesiredTime, mLastEndTimeVideo);
    }
  }

  virtual void
  NotifyEvent(MediaStreamGraph* aGraph,
              MediaStreamListener::MediaStreamGraphEvent aEvent) MOZ_OVERRIDE
  {
    switch (aEvent) {
      case EVENT_FINISHED:
        NotifyFinished(aGraph);
        break;
      case EVENT_REMOVED:
        NotifyRemoved(aGraph);
        break;
      case EVENT_HAS_DIRECT_LISTENERS:
        NotifyDirectListeners(aGraph, true);
        break;
      case EVENT_HAS_NO_DIRECT_LISTENERS:
        NotifyDirectListeners(aGraph, false);
        break;
      default:
        break;
    }
  }

  virtual void
  NotifyFinished(MediaStreamGraph* aGraph);

  virtual void
  NotifyRemoved(MediaStreamGraph* aGraph);

  virtual void
  NotifyDirectListeners(MediaStreamGraph* aGraph, bool aHasListeners);

private:
  
  nsCOMPtr<nsIThread> mMediaThread;
  uint64_t mWindowID;

  bool mStopped; 

  

  
  
  nsRefPtr<MediaEngineSource> mAudioSource; 
  nsRefPtr<MediaEngineSource> mVideoSource; 
  nsRefPtr<SourceMediaStream> mStream; 
  TrackTicks mLastEndTimeAudio;
  TrackTicks mLastEndTimeVideo;
  bool mFinished;

  
  Mutex mLock; 
  bool mRemoved;
};

class GetUserMediaNotificationEvent: public nsRunnable
{
  public:
    enum GetUserMediaStatus {
      STARTING,
      STOPPING,
      STOPPED_TRACK
    };
    GetUserMediaNotificationEvent(GetUserMediaCallbackMediaStreamListener* aListener,
                                  GetUserMediaStatus aStatus,
                                  bool aIsAudio, bool aIsVideo, uint64_t aWindowID)
    : mListener(aListener) , mStatus(aStatus) , mIsAudio(aIsAudio)
    , mIsVideo(aIsVideo), mWindowID(aWindowID) {}

    GetUserMediaNotificationEvent(GetUserMediaStatus aStatus,
                                  already_AddRefed<DOMMediaStream> aStream,
                                  DOMMediaStream::OnTracksAvailableCallback* aOnTracksAvailableCallback,
                                  bool aIsAudio, bool aIsVideo, uint64_t aWindowID,
                                  already_AddRefed<nsIDOMGetUserMediaErrorCallback> aError)
    : mStream(aStream), mOnTracksAvailableCallback(aOnTracksAvailableCallback),
      mStatus(aStatus), mIsAudio(aIsAudio), mIsVideo(aIsVideo), mWindowID(aWindowID),
      mError(aError) {}
    virtual ~GetUserMediaNotificationEvent()
    {

    }

    NS_IMETHOD Run() MOZ_OVERRIDE;

  protected:
    nsRefPtr<GetUserMediaCallbackMediaStreamListener> mListener; 
    nsRefPtr<DOMMediaStream> mStream;
    nsAutoPtr<DOMMediaStream::OnTracksAvailableCallback> mOnTracksAvailableCallback;
    GetUserMediaStatus mStatus;
    bool mIsAudio;
    bool mIsVideo;
    uint64_t mWindowID;
    nsRefPtr<nsIDOMGetUserMediaErrorCallback> mError;
};

typedef enum {
  MEDIA_START,
  MEDIA_STOP,
  MEDIA_STOP_TRACK,
  MEDIA_DIRECT_LISTENERS
} MediaOperation;

class MediaManager;
class GetUserMediaRunnable;






class ErrorCallbackRunnable : public nsRunnable
{
public:
  ErrorCallbackRunnable(
    nsCOMPtr<nsIDOMGetUserMediaSuccessCallback>& aSuccess,
    nsCOMPtr<nsIDOMGetUserMediaErrorCallback>& aError,
    const nsAString& aErrorMsg, uint64_t aWindowID);
  NS_IMETHOD Run();
private:
  ~ErrorCallbackRunnable();

  nsCOMPtr<nsIDOMGetUserMediaSuccessCallback> mSuccess;
  nsCOMPtr<nsIDOMGetUserMediaErrorCallback> mError;
  const nsString mErrorMsg;
  uint64_t mWindowID;
  nsRefPtr<MediaManager> mManager; 
};

class ReleaseMediaOperationResource : public nsRunnable
{
public:
  ReleaseMediaOperationResource(already_AddRefed<DOMMediaStream> aStream,
    DOMMediaStream::OnTracksAvailableCallback* aOnTracksAvailableCallback):
    mStream(aStream),
    mOnTracksAvailableCallback(aOnTracksAvailableCallback) {}
  NS_IMETHOD Run() MOZ_OVERRIDE {return NS_OK;}
private:
  nsRefPtr<DOMMediaStream> mStream;
  nsAutoPtr<DOMMediaStream::OnTracksAvailableCallback> mOnTracksAvailableCallback;
};




class MediaOperationRunnable : public nsRunnable
{
public:
  
  MediaOperationRunnable(MediaOperation aType,
    GetUserMediaCallbackMediaStreamListener* aListener,
    DOMMediaStream* aStream,
    DOMMediaStream::OnTracksAvailableCallback* aOnTracksAvailableCallback,
    MediaEngineSource* aAudioSource,
    MediaEngineSource* aVideoSource,
    bool aBool,
    uint64_t aWindowID,
    already_AddRefed<nsIDOMGetUserMediaErrorCallback> aError)
    : mType(aType)
    , mStream(aStream)
    , mOnTracksAvailableCallback(aOnTracksAvailableCallback)
    , mAudioSource(aAudioSource)
    , mVideoSource(aVideoSource)
    , mListener(aListener)
    , mBool(aBool)
    , mWindowID(aWindowID)
    , mError(aError)
  {}

  ~MediaOperationRunnable()
  {
    
  }

  nsresult returnAndCallbackError(nsresult rv, const char* errorLog)
  {
    MM_LOG(("%s , rv=%d", errorLog, rv));
    NS_DispatchToMainThread(new ReleaseMediaOperationResource(mStream.forget(),
          mOnTracksAvailableCallback.forget()));
    nsString log;

    log.AssignASCII(errorLog, strlen(errorLog));
    nsCOMPtr<nsIDOMGetUserMediaSuccessCallback> success;
    NS_DispatchToMainThread(new ErrorCallbackRunnable(success, mError,
      log, mWindowID));
    return NS_OK;
  }

  NS_IMETHOD
  Run() MOZ_OVERRIDE
  {
    SourceMediaStream *source = mListener->GetSourceStream();
    
    
    if (!source) 
      return NS_OK;

    switch (mType) {
      case MEDIA_START:
        {
          NS_ASSERTION(!NS_IsMainThread(), "Never call on main thread");
          nsresult rv;

          source->SetPullEnabled(true);

          DOMMediaStream::TrackTypeHints expectedTracks = 0;
          if (mAudioSource) {
            rv = mAudioSource->Start(source, kAudioTrack);
            if (NS_SUCCEEDED(rv)) {
              expectedTracks |= DOMMediaStream::HINT_CONTENTS_AUDIO;
            } else {
              return returnAndCallbackError(rv, "Starting audio failed");
            }
          }
          if (mVideoSource) {
            rv = mVideoSource->Start(source, kVideoTrack);
            if (NS_SUCCEEDED(rv)) {
              expectedTracks |= DOMMediaStream::HINT_CONTENTS_VIDEO;
            } else {
              return returnAndCallbackError(rv, "Starting video failed");
            }
          }

          mOnTracksAvailableCallback->SetExpectedTracks(expectedTracks);

          MM_LOG(("started all sources"));
          
          
          
          nsIRunnable *event =
            new GetUserMediaNotificationEvent(GetUserMediaNotificationEvent::STARTING,
                                              mStream.forget(),
                                              mOnTracksAvailableCallback.forget(),
                                              mAudioSource != nullptr,
                                              mVideoSource != nullptr,
                                              mWindowID, mError.forget());
          
          
          NS_DispatchToMainThread(event);
        }
        break;

      case MEDIA_STOP:
      case MEDIA_STOP_TRACK:
        {
          NS_ASSERTION(!NS_IsMainThread(), "Never call on main thread");
          if (mAudioSource) {
            mAudioSource->Stop(source, kAudioTrack);
            mAudioSource->Deallocate();
          }
          if (mVideoSource) {
            mVideoSource->Stop(source, kVideoTrack);
            mVideoSource->Deallocate();
          }
          
          if (mBool) {
            source->Finish();
          }

          nsIRunnable *event =
            new GetUserMediaNotificationEvent(mListener,
                                              mType == MEDIA_STOP ?
                                              GetUserMediaNotificationEvent::STOPPING :
                                              GetUserMediaNotificationEvent::STOPPED_TRACK,
                                              mAudioSource != nullptr,
                                              mVideoSource != nullptr,
                                              mWindowID);
          
          
          NS_DispatchToMainThread(event);
        }
        break;

      case MEDIA_DIRECT_LISTENERS:
        {
          NS_ASSERTION(!NS_IsMainThread(), "Never call on main thread");
          if (mVideoSource) {
            mVideoSource->SetDirectListeners(mBool);
          }
        }
        break;

      default:
        MOZ_ASSERT(false,"invalid MediaManager operation");
        break;
    }
    return NS_OK;
  }

private:
  MediaOperation mType;
  nsRefPtr<DOMMediaStream> mStream;
  nsAutoPtr<DOMMediaStream::OnTracksAvailableCallback> mOnTracksAvailableCallback;
  nsRefPtr<MediaEngineSource> mAudioSource; 
  nsRefPtr<MediaEngineSource> mVideoSource; 
  nsRefPtr<GetUserMediaCallbackMediaStreamListener> mListener; 
  bool mBool;
  uint64_t mWindowID;
  nsCOMPtr<nsIDOMGetUserMediaErrorCallback> mError;
};

typedef nsTArray<nsRefPtr<GetUserMediaCallbackMediaStreamListener> > StreamListeners;
typedef nsClassHashtable<nsUint64HashKey, StreamListeners> WindowTable;

class MediaDevice : public nsIMediaDevice
{
public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIMEDIADEVICE

  static MediaDevice* Create(MediaEngineVideoSource* source);
  static MediaDevice* Create(MediaEngineAudioSource* source);

protected:
  virtual ~MediaDevice() {}
  explicit MediaDevice(MediaEngineSource* aSource);
  nsString mName;
  nsString mID;
  bool mHasFacingMode;
  dom::VideoFacingModeEnum mFacingMode;
  MediaSourceType mMediaSource;
  nsRefPtr<MediaEngineSource> mSource;
};

class VideoDevice : public MediaDevice
{
public:
  explicit VideoDevice(MediaEngineVideoSource* aSource);
  NS_IMETHOD GetType(nsAString& aType);
  MediaEngineVideoSource* GetSource();
};

class AudioDevice : public MediaDevice
{
public:
  explicit AudioDevice(MediaEngineAudioSource* aSource);
  NS_IMETHOD GetType(nsAString& aType);
  MediaEngineAudioSource* GetSource();
};

class MediaManager MOZ_FINAL : public nsIMediaManagerService,
                               public nsIObserver
{
public:
  static already_AddRefed<MediaManager> GetInstance();

  
  
  
  static MediaManager* Get();

  static bool Exists()
  {
    return !!sSingleton;
  }

  static nsIThread* GetThread() {
    return Get()->mMediaThread;
  }

  static nsresult NotifyRecordingStatusChange(nsPIDOMWindow* aWindow,
                                              const nsString& aMsg,
                                              const bool& aIsAudio,
                                              const bool& aIsVideo);

  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIOBSERVER
  NS_DECL_NSIMEDIAMANAGERSERVICE

  MediaEngine* GetBackend(uint64_t aWindowId = 0);
  StreamListeners *GetWindowListeners(uint64_t aWindowId) {
    NS_ASSERTION(NS_IsMainThread(), "Only access windowlist on main thread");

    return mActiveWindows.Get(aWindowId);
  }
  void RemoveWindowID(uint64_t aWindowId);
  bool IsWindowStillActive(uint64_t aWindowId) {
    return !!GetWindowListeners(aWindowId);
  }
  
  void RemoveFromWindowList(uint64_t aWindowID,
    GetUserMediaCallbackMediaStreamListener *aListener);

  nsresult GetUserMedia(bool aPrivileged,
    nsPIDOMWindow* aWindow,
    const dom::MediaStreamConstraints& aRawConstraints,
    nsIDOMGetUserMediaSuccessCallback* onSuccess,
    nsIDOMGetUserMediaErrorCallback* onError);

  nsresult GetUserMediaDevices(nsPIDOMWindow* aWindow,
    const dom::MediaStreamConstraints& aConstraints,
    nsIGetUserMediaDevicesSuccessCallback* onSuccess,
    nsIDOMGetUserMediaErrorCallback* onError,
    uint64_t aInnerWindowID = 0);
  void OnNavigation(uint64_t aWindowID);

  MediaEnginePrefs mPrefs;

private:
  WindowTable *GetActiveWindows() {
    NS_ASSERTION(NS_IsMainThread(), "Only access windowlist on main thread");
    return &mActiveWindows;
  }

  void GetPref(nsIPrefBranch *aBranch, const char *aPref,
               const char *aData, int32_t *aVal);
  void GetPrefBool(nsIPrefBranch *aBranch, const char *aPref,
                   const char *aData, bool *aVal);
  void GetPrefs(nsIPrefBranch *aBranch, const char *aData);

  
  MediaManager();

  ~MediaManager() {}

  nsresult MediaCaptureWindowStateInternal(nsIDOMWindow* aWindow, bool* aVideo,
                                           bool* aAudio, bool *aScreenShare,
                                           bool* aWindowShare, bool *aAppShare);

  void StopScreensharing(uint64_t aWindowID);
  void StopScreensharing(nsPIDOMWindow *aWindow);

  void StopMediaStreams();

  
  WindowTable mActiveWindows;
  nsRefPtrHashtable<nsStringHashKey, GetUserMediaRunnable> mActiveCallbacks;
  nsClassHashtable<nsUint64HashKey, nsTArray<nsString>> mCallIds;
  
  nsCOMPtr<nsIThread> mMediaThread;

  Mutex mMutex;
  
  RefPtr<MediaEngine> mBackend;

  static StaticRefPtr<MediaManager> sSingleton;

#ifdef MOZ_B2G_CAMERA
  nsRefPtr<nsDOMCameraManager> mCameraManager;
#endif
};

} 
