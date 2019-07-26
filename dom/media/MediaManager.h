



#include "MediaEngine.h"
#include "mozilla/Services.h"
#include "nsIMediaManager.h"

#include "nsHashKeys.h"
#include "nsGlobalWindow.h"
#include "nsClassHashtable.h"
#include "nsRefPtrHashtable.h"
#include "nsObserverService.h"

#include "nsPIDOMWindow.h"
#include "nsIDOMNavigatorUserMedia.h"
#include "mozilla/Attributes.h"
#include "mozilla/StaticPtr.h"
#include "prlog.h"

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
      } else {
        obs->NotifyObservers(nullptr,
            "recording-device-events",
            NS_LITERAL_STRING("shutdown").get());
      }
      return NS_OK;
    }

  protected:
    GetUserMediaStatus mStatus;
};

typedef enum {
  MEDIA_START,
  MEDIA_STOP
} MediaOperation;

class GetUserMediaCallbackMediaStreamListener;




class MediaOperationRunnable : public nsRunnable
{
public:
  MediaOperationRunnable(MediaOperation aType,
    nsDOMMediaStream* aStream,
    MediaEngineSource* aAudioSource,
    MediaEngineSource* aVideoSource)
    : mType(aType)
    , mAudioSource(aAudioSource)
    , mVideoSource(aVideoSource)
    , mStream(aStream)
    {}

  MediaOperationRunnable(MediaOperation aType,
    SourceMediaStream* aStream,
    MediaEngineSource* aAudioSource,
    MediaEngineSource* aVideoSource)
    : mType(aType)
    , mAudioSource(aAudioSource)
    , mVideoSource(aVideoSource)
    , mStream(nullptr)
    , mSourceStream(aStream)
    {}

  ~MediaOperationRunnable()
  {
    
    
    if (mStream) {
      nsCOMPtr<nsIThread> mainThread = do_GetMainThread();
      nsDOMMediaStream *stream;
      mStream.forget(&stream);
      NS_ProxyRelease(mainThread, stream, true);
    }
  }

  NS_IMETHOD
  Run()
  {
    
    
    if (mStream) {
      mSourceStream = mStream->GetStream()->AsSourceStream();
    }
    switch (mType) {
      case MEDIA_START:
        {
          NS_ASSERTION(!NS_IsMainThread(), "Never call on main thread");
          nsresult rv;

          mSourceStream->SetPullEnabled(true);

          if (mAudioSource) {
            rv = mAudioSource->Start(mSourceStream, kAudioTrack);
            if (NS_FAILED(rv)) {
              MM_LOG(("Starting audio failed, rv=%d",rv));
            }
          }
          if (mVideoSource) {
            rv = mVideoSource->Start(mSourceStream, kVideoTrack);
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
            mAudioSource->Stop(mSourceStream, kAudioTrack);
            mAudioSource->Deallocate();
          }
          if (mVideoSource) {
            mVideoSource->Stop(mSourceStream, kVideoTrack);
            mVideoSource->Deallocate();
          }
          
          mSourceStream->Finish();

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
  nsRefPtr<nsDOMMediaStream> mStream;       
  SourceMediaStream *mSourceStream;
};






class GetUserMediaCallbackMediaStreamListener : public MediaStreamListener
{
public:
  GetUserMediaCallbackMediaStreamListener(nsIThread *aThread,
    nsDOMMediaStream* aStream,
    MediaEngineSource* aAudioSource,
    MediaEngineSource* aVideoSource)
    : mMediaThread(aThread)
    , mAudioSource(aAudioSource)
    , mVideoSource(aVideoSource)
    , mStream(aStream)
    , mSourceStream(aStream->GetStream()->AsSourceStream())
    , mLastEndTimeAudio(0)
    , mLastEndTimeVideo(0) {}

  ~GetUserMediaCallbackMediaStreamListener()
  {
    
    if (mStream) {
      nsCOMPtr<nsIThread> mainThread = do_GetMainThread();
      nsDOMMediaStream *stream;
      mStream.forget(&stream);
      
      NS_ProxyRelease(mainThread, stream, false);
    }
  }

  void
  Invalidate()
  {
    nsRefPtr<MediaOperationRunnable> runnable;

    
    
    
    
    
    
    
    
    
    runnable = new MediaOperationRunnable(MEDIA_STOP,
                                          mSourceStream, mAudioSource, mVideoSource);
    mMediaThread->Dispatch(runnable, NS_DISPATCH_NORMAL);

    return;
  }

  void
  Remove()
  {
    NS_ASSERTION(NS_IsMainThread(), "Only call on main thread");
    
    mStream->GetStream()->RemoveListener(this);
  }

  
  void
  NotifyPull(MediaStreamGraph* aGraph, StreamTime aDesiredTime)
  {
    
    
    if (mAudioSource) {
      mAudioSource->NotifyPull(aGraph, mSourceStream, kAudioTrack, aDesiredTime, mLastEndTimeAudio);
    }
    if (mVideoSource) {
      mVideoSource->NotifyPull(aGraph, mSourceStream, kVideoTrack, aDesiredTime, mLastEndTimeVideo);
    }
  }

  void
  NotifyFinished(MediaStreamGraph* aGraph)
  {
    Invalidate();
    
  }

private:
  nsCOMPtr<nsIThread> mMediaThread;
  nsRefPtr<MediaEngineSource> mAudioSource;
  nsRefPtr<MediaEngineSource> mVideoSource;
  nsRefPtr<nsDOMMediaStream> mStream;
  SourceMediaStream *mSourceStream; 
  TrackTicks mLastEndTimeAudio;
  TrackTicks mLastEndTimeVideo;
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
      obs->AddObserver(sSingleton, "xpcom-shutdown", false);
      obs->AddObserver(sSingleton, "getUserMedia:response:allow", false);
      obs->AddObserver(sSingleton, "getUserMedia:response:deny", false);
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
  bool IsWindowStillActive(uint64_t aWindowId) {
    return !!GetWindowListeners(aWindowId);
  }

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

  
  WindowTable mActiveWindows;
  nsRefPtrHashtable<nsStringHashKey, nsRunnable> mActiveCallbacks;
  
  nsCOMPtr<nsIThread> mMediaThread;

  Mutex mMutex;
  
  MediaEngine* mBackend;

  static StaticRefPtr<MediaManager> sSingleton;
};

} 
