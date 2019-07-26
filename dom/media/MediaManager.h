



#include "MediaEngine.h"
#include "mozilla/Services.h"
#include "mozilla/unused.h"
#include "nsIMediaManager.h"

#include "nsHashKeys.h"
#include "nsGlobalWindow.h"
#include "nsClassHashtable.h"
#include "nsRefPtrHashtable.h"
#include "nsObserverService.h"
#include "nsIPrefService.h"
#include "nsIPrefBranch.h"

#include "nsPIDOMWindow.h"
#include "nsIDOMNavigatorUserMedia.h"
#include "nsXULAppAPI.h"
#include "mozilla/Attributes.h"
#include "mozilla/StaticPtr.h"
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
class MediaStreamConstraints;
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

  
  
  bool CapturingVideo()
  {
    NS_ASSERTION(NS_IsMainThread(), "Only call on main thread");
    return mVideoSource && !mVideoSource->IsFake() && !mStopped;
  }
  bool CapturingAudio()
  {
    NS_ASSERTION(NS_IsMainThread(), "Only call on main thread");
    return mAudioSource && !mAudioSource->IsFake() && !mStopped;
  }

  void SetStopped()
  {
    mStopped = true;
  }

  
  
  void Invalidate();

  void
  AudioConfig(bool aEchoOn, uint32_t aEcho,
              bool aAgcOn, uint32_t aAGC,
              bool aNoiseOn, uint32_t aNoise)
  {
    if (mAudioSource) {
#ifdef MOZ_WEBRTC
      
      RUN_ON_THREAD(mMediaThread,
                    WrapRunnable(nsRefPtr<MediaEngineSource>(mAudioSource), 
                                 &MediaEngineSource::Config,
                                 aEchoOn, aEcho, aAgcOn, aAGC, aNoiseOn, aNoise),
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
  NotifyFinished(MediaStreamGraph* aGraph) MOZ_OVERRIDE;

  virtual void
  NotifyRemoved(MediaStreamGraph* aGraph) MOZ_OVERRIDE;

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
      STOPPING
    };
    GetUserMediaNotificationEvent(GetUserMediaCallbackMediaStreamListener* aListener,
                                  GetUserMediaStatus aStatus)
    : mListener(aListener), mStatus(aStatus) {}

    GetUserMediaNotificationEvent(GetUserMediaStatus aStatus,
                                  already_AddRefed<DOMMediaStream> aStream,
                                  DOMMediaStream::OnTracksAvailableCallback* aOnTracksAvailableCallback)
    : mStream(aStream), mOnTracksAvailableCallback(aOnTracksAvailableCallback),
      mStatus(aStatus) {}
    virtual ~GetUserMediaNotificationEvent()
    {

    }

    NS_IMETHOD Run() MOZ_OVERRIDE;

  protected:
    nsRefPtr<GetUserMediaCallbackMediaStreamListener> mListener; 
    nsRefPtr<DOMMediaStream> mStream;
    nsAutoPtr<DOMMediaStream::OnTracksAvailableCallback> mOnTracksAvailableCallback;
    GetUserMediaStatus mStatus;
};

typedef enum {
  MEDIA_START,
  MEDIA_STOP
} MediaOperation;




class MediaOperationRunnable : public nsRunnable
{
public:
  
  MediaOperationRunnable(MediaOperation aType,
    GetUserMediaCallbackMediaStreamListener* aListener,
    DOMMediaStream* aStream,
    DOMMediaStream::OnTracksAvailableCallback* aOnTracksAvailableCallback,
    MediaEngineSource* aAudioSource,
    MediaEngineSource* aVideoSource,
    bool aNeedsFinish)
    : mType(aType)
    , mStream(aStream)
    , mOnTracksAvailableCallback(aOnTracksAvailableCallback)
    , mAudioSource(aAudioSource)
    , mVideoSource(aVideoSource)
    , mListener(aListener)
    , mFinish(aNeedsFinish)
    {}

  ~MediaOperationRunnable()
  {
    
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
              MM_LOG(("Starting audio failed, rv=%d",rv));
            }
          }
          if (mVideoSource) {
            rv = mVideoSource->Start(source, kVideoTrack);
            if (NS_SUCCEEDED(rv)) {
              expectedTracks |= DOMMediaStream::HINT_CONTENTS_VIDEO;
            } else {
              MM_LOG(("Starting video failed, rv=%d",rv));
            }
          }

          mOnTracksAvailableCallback->SetExpectedTracks(expectedTracks);

          MM_LOG(("started all sources"));
          
          
          
          nsRefPtr<GetUserMediaNotificationEvent> event =
            new GetUserMediaNotificationEvent(GetUserMediaNotificationEvent::STARTING,
                                              mStream.forget(),
                                              mOnTracksAvailableCallback.forget());
          NS_DispatchToMainThread(event, NS_DISPATCH_NORMAL);
        }
        break;

      case MEDIA_STOP:
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
          
          if (mFinish) {
            source->Finish();
          }
          nsRefPtr<GetUserMediaNotificationEvent> event =
            new GetUserMediaNotificationEvent(mListener, GetUserMediaNotificationEvent::STOPPING);

          NS_DispatchToMainThread(event, NS_DISPATCH_NORMAL);
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
  bool mFinish;
};

typedef nsTArray<nsRefPtr<GetUserMediaCallbackMediaStreamListener> > StreamListeners;
typedef nsClassHashtable<nsUint64HashKey, StreamListeners> WindowTable;

class MediaDevice : public nsIMediaDevice
{
public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIMEDIADEVICE

  MediaDevice(MediaEngineVideoSource* aSource);
  MediaDevice(MediaEngineAudioSource* aSource);
  virtual ~MediaDevice() {}

  MediaEngineSource* GetSource();
private:
  nsString mName;
  nsString mType;
  nsString mID;
  bool mHasFacingMode;
  dom::VideoFacingModeEnum mFacingMode;
  nsRefPtr<MediaEngineSource> mSource;
};

class MediaManager MOZ_FINAL : public nsIMediaManagerService,
                               public nsIObserver
{
public:
  static already_AddRefed<MediaManager> GetInstance();

  
  
  
  static MediaManager* Get();

  static nsIThread* GetThread() {
    return Get()->mMediaThread;
  }

  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIOBSERVER
  NS_DECL_NSIMEDIAMANAGERSERVICE

  MediaEngine* GetBackend(uint64_t aWindowId = 0);
  StreamListeners *GetWindowListeners(uint64_t aWindowId) {
    NS_ASSERTION(NS_IsMainThread(), "Only access windowlist on main thread");

    return mActiveWindows.Get(aWindowId);
  }
  void RemoveWindowID(uint64_t aWindowId) {
    mActiveWindows.Remove(aWindowId);
  }
  bool IsWindowStillActive(uint64_t aWindowId) {
    return !!GetWindowListeners(aWindowId);
  }
  
  void RemoveFromWindowList(uint64_t aWindowID,
    GetUserMediaCallbackMediaStreamListener *aListener);

  nsresult GetUserMedia(JSContext* aCx, bool aPrivileged,
    nsPIDOMWindow* aWindow,
    const dom::MediaStreamConstraints& aRawConstraints,
    nsIDOMGetUserMediaSuccessCallback* onSuccess,
    nsIDOMGetUserMediaErrorCallback* onError);

  nsresult GetUserMediaDevices(nsPIDOMWindow* aWindow,
    const dom::MediaStreamConstraintsInternal& aConstraints,
    nsIGetUserMediaDevicesSuccessCallback* onSuccess,
    nsIDOMGetUserMediaErrorCallback* onError);
  void OnNavigation(uint64_t aWindowID);

  MediaEnginePrefs mPrefs;

private:
  WindowTable *GetActiveWindows() {
    NS_ASSERTION(NS_IsMainThread(), "Only access windowlist on main thread");
    return &mActiveWindows;
  }

  void GetPref(nsIPrefBranch *aBranch, const char *aPref,
               const char *aData, int32_t *aVal);
  void GetPrefs(nsIPrefBranch *aBranch, const char *aData);

  
  MediaManager();

  ~MediaManager() {
    delete mBackend;
  }

  nsresult MediaCaptureWindowStateInternal(nsIDOMWindow* aWindow, bool* aVideo,
                                           bool* aAudio);

  void StopMediaStreams();

  
  WindowTable mActiveWindows;
  nsRefPtrHashtable<nsStringHashKey, nsRunnable> mActiveCallbacks;
  
  nsCOMPtr<nsIThread> mMediaThread;

  Mutex mMutex;
  
  MediaEngine* mBackend;

  static StaticRefPtr<MediaManager> sSingleton;

#ifdef MOZ_WIDGET_GONK
  nsRefPtr<nsDOMCameraManager> mCameraManager;
#endif
};

} 
