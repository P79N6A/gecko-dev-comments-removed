



#include "MediaEngine.h"
#include "mozilla/dom/ContentChild.h"
#include "mozilla/Services.h"
#include "mozilla/unused.h"
#include "nsIMediaManager.h"

#include "nsHashKeys.h"
#include "nsGlobalWindow.h"
#include "nsClassHashtable.h"
#include "nsRefPtrHashtable.h"
#include "nsObserverService.h"

#include "nsPIDOMWindow.h"
#include "nsIDOMNavigatorUserMedia.h"
#include "nsXULAppAPI.h"
#include "mozilla/Attributes.h"
#include "mozilla/StaticPtr.h"
#include "prlog.h"

#ifdef MOZ_WEBRTC
#include "mtransport/runnable_utils.h"
#endif

namespace mozilla {

#ifdef PR_LOGGING
extern PRLogModuleInfo* GetMediaManagerLog();
#define MM_LOG(msg) PR_LOG(GetMediaManagerLog(), PR_LOG_DEBUG, msg)
#else
#define MM_LOG(msg)
#endif

class GetUserMediaNotificationEvent: public nsRunnable
{
  public:
    enum GetUserMediaStatus {
      STARTING,
      STOPPING
    };
    GetUserMediaNotificationEvent(GetUserMediaStatus aStatus)
    : mStatus(aStatus) {}

    NS_IMETHOD
    Run()
    {
      nsCOMPtr<nsIObserverService> obs = mozilla::services::GetObserverService();
      if (!obs) {
        NS_WARNING("Could not get the Observer service for GetUserMedia recording notification.");
        return NS_ERROR_FAILURE;
      }
      if (mStatus) {
        obs->NotifyObservers(nullptr,
            "recording-device-events",
            NS_LITERAL_STRING("starting").get());
        
        
        if (XRE_GetProcessType() != GeckoProcessType_Default) {
          unused << mozilla::dom::ContentChild::GetSingleton()->SendRecordingDeviceEvents(NS_LITERAL_STRING("starting"));
        }
      } else {
        obs->NotifyObservers(nullptr,
            "recording-device-events",
            NS_LITERAL_STRING("shutdown").get());
        
        
        if (XRE_GetProcessType() != GeckoProcessType_Default) {
          unused << mozilla::dom::ContentChild::GetSingleton()->SendRecordingDeviceEvents(NS_LITERAL_STRING("shutdown"));
        }
      }
      return NS_OK;
    }

  protected:
    GetUserMediaStatus mStatus;
};






class GetUserMediaCallbackMediaStreamListener : public MediaStreamListener
{
public:
  
  GetUserMediaCallbackMediaStreamListener(nsIThread *aThread,
    uint64_t aWindowID)
    : mMediaThread(aThread)
    , mWindowID(aWindowID)
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
    return mVideoSource && mLastEndTimeVideo > 0 && !mFinished;
  }
  bool CapturingAudio()
  {
    return mAudioSource && mLastEndTimeAudio > 0 && !mFinished;
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

  

  
  
  nsRefPtr<MediaEngineSource> mAudioSource; 
  nsRefPtr<MediaEngineSource> mVideoSource; 
  nsRefPtr<SourceMediaStream> mStream; 
  TrackTicks mLastEndTimeAudio;
  TrackTicks mLastEndTimeVideo;
  bool mFinished;

  
  Mutex mLock; 
  bool mRemoved;
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
    MediaEngineSource* aAudioSource,
    MediaEngineSource* aVideoSource,
    bool aNeedsFinish)
    : mType(aType)
    , mAudioSource(aAudioSource)
    , mVideoSource(aVideoSource)
    , mListener(aListener)
    , mFinish(aNeedsFinish)
    {}

  ~MediaOperationRunnable()
  {
    
  }

  NS_IMETHOD
  Run()
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

          if (mAudioSource) {
            rv = mAudioSource->Start(source, kAudioTrack);
            if (NS_FAILED(rv)) {
              MM_LOG(("Starting audio failed, rv=%d",rv));
            }
          }
          if (mVideoSource) {
            rv = mVideoSource->Start(source, kVideoTrack);
            if (NS_FAILED(rv)) {
              MM_LOG(("Starting video failed, rv=%d",rv));
            }
          }

          MM_LOG(("started all sources"));
          nsRefPtr<GetUserMediaNotificationEvent> event =
            new GetUserMediaNotificationEvent(GetUserMediaNotificationEvent::STARTING);


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
            new GetUserMediaNotificationEvent(GetUserMediaNotificationEvent::STOPPING);

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
  NS_DECL_ISUPPORTS
  NS_DECL_NSIMEDIADEVICE

  MediaDevice(MediaEngineVideoSource* aSource) {
    mSource = aSource;
    mType.Assign(NS_LITERAL_STRING("video"));
    mSource->GetName(mName);
    mSource->GetUUID(mID);
  }
  MediaDevice(MediaEngineAudioSource* aSource) {
    mSource = aSource;
    mType.Assign(NS_LITERAL_STRING("audio"));
    mSource->GetName(mName);
    mSource->GetUUID(mID);
  }
  virtual ~MediaDevice() {}

  MediaEngineSource* GetSource();
private:
  nsString mName;
  nsString mType;
  nsString mID;
  nsRefPtr<MediaEngineSource> mSource;
};

class MediaManager MOZ_FINAL : public nsIMediaManagerService,
                               public nsIObserver
{
public:
  static already_AddRefed<MediaManager> GetInstance();

  
  
  
  static MediaManager* Get() {
    if (!sSingleton) {
      sSingleton = new MediaManager();

      NS_NewThread(getter_AddRefs(sSingleton->mMediaThread));
      MM_LOG(("New Media thread for gum"));

      NS_ASSERTION(NS_IsMainThread(), "Only create MediaManager on main thread");
      nsCOMPtr<nsIObserverService> obs = services::GetObserverService();
      if (obs) {
        obs->AddObserver(sSingleton, "xpcom-shutdown", false);
        obs->AddObserver(sSingleton, "getUserMedia:response:allow", false);
        obs->AddObserver(sSingleton, "getUserMedia:response:deny", false);
        obs->AddObserver(sSingleton, "getUserMedia:revoke", false);
      }
      
    }
    return sSingleton;
  }
  static nsIThread* GetThread() {
    return Get()->mMediaThread;
  }

  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER
  NS_DECL_NSIMEDIAMANAGERSERVICE

  MediaEngine* GetBackend();
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

  nsresult GetUserMedia(bool aPrivileged, nsPIDOMWindow* aWindow,
    nsIMediaStreamOptions* aParams,
    nsIDOMGetUserMediaSuccessCallback* onSuccess,
    nsIDOMGetUserMediaErrorCallback* onError);
  nsresult GetUserMediaDevices(nsPIDOMWindow* aWindow,
    nsIGetUserMediaDevicesSuccessCallback* onSuccess,
    nsIDOMGetUserMediaErrorCallback* onError);
  void OnNavigation(uint64_t aWindowID);

private:
  WindowTable *GetActiveWindows() {
    NS_ASSERTION(NS_IsMainThread(), "Only access windowlist on main thread");
    return &mActiveWindows;
  }

  
  MediaManager()
  : mMediaThread(nullptr)
  , mMutex("mozilla::MediaManager")
  , mBackend(nullptr) {
    mActiveWindows.Init();
    mActiveCallbacks.Init();
  }

  ~MediaManager() {
    delete mBackend;
  }

  nsresult MediaCaptureWindowStateInternal(nsIDOMWindow* aWindow, bool* aVideo,
                                           bool* aAudio);

  
  WindowTable mActiveWindows;
  nsRefPtrHashtable<nsStringHashKey, nsRunnable> mActiveCallbacks;
  
  nsCOMPtr<nsIThread> mMediaThread;

  Mutex mMutex;
  
  MediaEngine* mBackend;

  static StaticRefPtr<MediaManager> sSingleton;
};

} 
