



#include "MediaManager.h"

#include "MediaStreamGraph.h"
#include "nsIDOMFile.h"
#include "nsIEventTarget.h"
#include "nsIScriptGlobalObject.h"
#include "nsIPopupWindowManager.h"

#include "nsJSUtils.h"
#include "nsDOMFile.h"
#include "nsGlobalWindow.h"


#if defined(MOZ_WEBRTC)
#include "MediaEngineWebRTC.h"
#else
#include "MediaEngineDefault.h"
#endif

namespace mozilla {





class ErrorCallbackRunnable : public nsRunnable
{
public:
  ErrorCallbackRunnable(nsIDOMGetUserMediaErrorCallback* aError,
    const nsString& aErrorMsg, uint64_t aWindowID)
    : mError(aError)
    , mErrorMsg(aErrorMsg)
    , mWindowID(aWindowID) {}

  NS_IMETHOD
  Run()
  {
    
    WindowTable* activeWindows = MediaManager::Get()->GetActiveWindows();
    if (activeWindows->Get(mWindowID)) {
      mError->OnError(mErrorMsg);
    }
    return NS_OK;
  }

private:
  nsCOMPtr<nsIDOMGetUserMediaErrorCallback> mError;
  const nsString mErrorMsg;
  uint64_t mWindowID;
};







class SuccessCallbackRunnable : public nsRunnable
{
public:
  SuccessCallbackRunnable(nsIDOMGetUserMediaSuccessCallback* aSuccess,
    nsIDOMFile* aFile, uint64_t aWindowID)
    : mSuccess(aSuccess)
    , mFile(aFile)
    , mWindowID(aWindowID) {}

  NS_IMETHOD
  Run()
  {
    
    WindowTable* activeWindows = MediaManager::Get()->GetActiveWindows();
    if (activeWindows->Get(mWindowID)) {
      
      mSuccess->OnSuccess(mFile);
    }
    return NS_OK;
  }

private:
  nsCOMPtr<nsIDOMGetUserMediaSuccessCallback> mSuccess;
  nsCOMPtr<nsIDOMFile> mFile;
  uint64_t mWindowID;
};







class GetUserMediaStreamRunnable : public nsRunnable
{
public:
  GetUserMediaStreamRunnable(nsIDOMGetUserMediaSuccessCallback* aSuccess,
    MediaEngineSource* aSource, StreamListeners* aListeners,
    uint64_t aWindowID, TrackID aTrackID)
    : mSuccess(aSuccess)
    , mSource(aSource)
    , mListeners(aListeners)
    , mWindowID(aWindowID)
    , mTrackID(aTrackID) {}

  ~GetUserMediaStreamRunnable() {}

  NS_IMETHOD
  Run()
  {
    
    nsCOMPtr<nsDOMMediaStream> stream = nsDOMMediaStream::CreateInputStream();

    nsPIDOMWindow *window = static_cast<nsPIDOMWindow*>
      (nsGlobalWindow::GetInnerWindowWithId(mWindowID));

    if (window && window->GetExtantDoc()) {
      stream->CombineWithPrincipal(window->GetExtantDoc()->NodePrincipal());
    }

    
    
    
    GetUserMediaCallbackMediaStreamListener* listener =
      new GetUserMediaCallbackMediaStreamListener(mSource, stream, mTrackID);
    stream->GetStream()->AddListener(listener);

    
    mListeners->AppendElement(listener);

    
    WindowTable* activeWindows = MediaManager::Get()->GetActiveWindows();
    if (activeWindows->Get(mWindowID)) {
      mSuccess->OnSuccess(stream);
    }

    return NS_OK;
  }

private:
  nsCOMPtr<nsIDOMGetUserMediaSuccessCallback> mSuccess;
  nsRefPtr<MediaEngineSource> mSource;
  StreamListeners* mListeners;
  uint64_t mWindowID;
  TrackID mTrackID;
};










class GetUserMediaRunnable : public nsRunnable
{
public:
  GetUserMediaRunnable(bool aAudio, bool aVideo, bool aPicture,
    nsIDOMGetUserMediaSuccessCallback* aSuccess,
    nsIDOMGetUserMediaErrorCallback* aError,
    StreamListeners* aListeners, uint64_t aWindowID)
    : mAudio(aAudio)
    , mVideo(aVideo)
    , mPicture(aPicture)
    , mSuccess(aSuccess)
    , mError(aError)
    , mListeners(aListeners)
    , mWindowID(aWindowID) {}

  ~GetUserMediaRunnable() {}

  
  enum {
    kVideoTrack = 1,
    kAudioTrack = 2
  };

  NS_IMETHOD
  Run()
  {
    mManager = MediaManager::Get();

    
    if (mPicture && (mAudio || mVideo)) {
      NS_DispatchToMainThread(new ErrorCallbackRunnable(
        mError, NS_LITERAL_STRING("NOT_SUPPORTED_ERR"), mWindowID
      ));
      return NS_OK;
    }

    if (mPicture) {
      SendPicture();
      return NS_OK;
    }

    
    if (mAudio && mVideo) {
      NS_DispatchToMainThread(new ErrorCallbackRunnable(
        mError, NS_LITERAL_STRING("NOT_IMPLEMENTED"), mWindowID
      ));
      return NS_OK;
    }

    if (mVideo) {
      SendVideo();
      return NS_OK;
    }

    if (mAudio) {
      SendAudio();
      return NS_OK;
    }

    return NS_OK;
  }

  



  void
  ProcessGetUserMedia(MediaEngineSource* aSource, TrackID aTrackID)
  {
    







    nsresult rv = aSource->Allocate();
    if (NS_FAILED(rv)) {
      NS_DispatchToMainThread(new ErrorCallbackRunnable(
        mError, NS_LITERAL_STRING("HARDWARE_UNAVAILABLE"), mWindowID
      ));
      return;
    }

    NS_DispatchToMainThread(new GetUserMediaStreamRunnable(
      mSuccess.get(), aSource, mListeners, mWindowID, aTrackID
    ));
    return;
  }

  



  void
  ProcessGetUserMediaSnapshot(MediaEngineSource* aSource, int aDuration)
  {
    nsresult rv = aSource->Allocate();
    if (NS_FAILED(rv)) {
      NS_DispatchToMainThread(new ErrorCallbackRunnable(
        mError, NS_LITERAL_STRING("HARDWARE_UNAVAILABLE"), mWindowID
      ));
      return;
    }

    nsCOMPtr<nsIDOMFile> file;
    aSource->Snapshot(aDuration, getter_AddRefs(file));
    aSource->Deallocate();

    NS_DispatchToMainThread(new SuccessCallbackRunnable(
      mSuccess, file, mWindowID
    ));
    return;
  }

  
  void
  SendPicture()
  {
    nsTArray<nsRefPtr<MediaEngineVideoSource> > videoSources;
    mManager->GetBackend()->EnumerateVideoDevices(&videoSources);

    uint32_t count = videoSources.Length();
    if (!count) {
      NS_DispatchToMainThread(new ErrorCallbackRunnable(
        mError, NS_LITERAL_STRING("NO_DEVICES_FOUND"), mWindowID
      ));
      return;
    }

    
    
    MediaEngineVideoSource* videoSource = videoSources[0];
    ProcessGetUserMediaSnapshot(videoSource, 0 );
  }

  
  void
  SendVideo()
  {
    nsTArray<nsRefPtr<MediaEngineVideoSource> > videoSources;
    mManager->GetBackend()->EnumerateVideoDevices(&videoSources);

    uint32_t count = videoSources.Length();
    if (!count) {
      NS_DispatchToMainThread(new ErrorCallbackRunnable(
        mError, NS_LITERAL_STRING("NO_DEVICES_FOUND"), mWindowID
      ));
      return;
    }

    MediaEngineVideoSource* videoSource = videoSources[0];
    ProcessGetUserMedia(videoSource, kVideoTrack);
  }

  
  void
  SendAudio()
  {
    nsTArray<nsRefPtr<MediaEngineAudioSource> > audioSources;
    mManager->GetBackend()->EnumerateAudioDevices(&audioSources);

    uint32_t count = audioSources.Length();
    if (!count) {
      NS_DispatchToMainThread(new ErrorCallbackRunnable(
        mError, NS_LITERAL_STRING("NO_DEVICES_FOUND"), mWindowID
      ));
      return;
    }

    MediaEngineAudioSource* audioSource = audioSources[0];
    ProcessGetUserMedia(audioSource, kAudioTrack);
  }

private:
  bool mAudio;
  bool mVideo;
  bool mPicture;

  nsCOMPtr<nsIDOMGetUserMediaSuccessCallback> mSuccess;
  nsCOMPtr<nsIDOMGetUserMediaErrorCallback> mError;
  StreamListeners* mListeners;
  uint64_t mWindowID;

  MediaManager* mManager;
};


nsRefPtr<MediaManager> MediaManager::sSingleton;

NS_IMPL_ISUPPORTS1(MediaManager, nsIObserver)






nsresult
MediaManager::GetUserMedia(nsPIDOMWindow* aWindow, nsIMediaStreamOptions* aParams,
  nsIDOMGetUserMediaSuccessCallback* onSuccess,
  nsIDOMGetUserMediaErrorCallback* onError)
{
  NS_ENSURE_TRUE(aParams, NS_ERROR_NULL_POINTER);
  NS_ENSURE_TRUE(aWindow, NS_ERROR_NULL_POINTER);

  bool audio, video, picture;

  nsresult rv = aParams->GetPicture(&picture);
  NS_ENSURE_SUCCESS(rv, rv);

  






#if !defined(MOZ_WEBRTC)
  if (picture) {
    if (aWindow->GetPopupControlState() > openControlled) {
      nsCOMPtr<nsIPopupWindowManager> pm =
        do_GetService(NS_POPUPWINDOWMANAGER_CONTRACTID);
      if (!pm)
        return NS_OK;

      uint32_t permission;
      nsCOMPtr<nsIDocument> doc = aWindow->GetExtantDoc();
      pm->TestPermission(doc->NodePrincipal(), &permission);
      if ((permission == nsIPopupWindowManager::DENY_POPUP)) {
        nsCOMPtr<nsIDOMDocument> domDoc = aWindow->GetExtantDocument();
        nsGlobalWindow::FirePopupBlockedEvent(
          domDoc, aWindow, nullptr, EmptyString(), EmptyString()
                                              );
        return NS_OK;
      }
    }
  }
#endif

  rv = aParams->GetAudio(&audio);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = aParams->GetVideo(&video);
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsString cameraType;
  rv = aParams->GetCamera(cameraType);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  uint64_t windowID = aWindow->WindowID();
  StreamListeners* listeners = mActiveWindows.Get(windowID);
  if (!listeners) {
    listeners = new StreamListeners;
    mActiveWindows.Put(windowID, listeners);
  }

  
  
  nsCOMPtr<nsIRunnable> gUMRunnable = new GetUserMediaRunnable(
    audio, video, picture, onSuccess, onError, listeners, windowID
  );

  if (picture) {
    
    NS_DispatchToMainThread(gUMRunnable);
  } else {
    
    if (!mMediaThread) {
      rv = NS_NewThread(getter_AddRefs(mMediaThread));
      NS_ENSURE_SUCCESS(rv, rv);
    }

    mMediaThread->Dispatch(gUMRunnable, NS_DISPATCH_NORMAL);
  }
  return NS_OK;
}

MediaEngine*
MediaManager::GetBackend()
{
  
  
  if (!mBackend) {
#if defined(MOZ_WEBRTC)
    mBackend = new MediaEngineWebRTC();
#else
    mBackend = new MediaEngineDefault();
#endif
  }

  return mBackend;
}

WindowTable*
MediaManager::GetActiveWindows()
{
  return &mActiveWindows;
}

void
MediaManager::OnNavigation(uint64_t aWindowID)
{
  
  
  StreamListeners* listeners = mActiveWindows.Get(aWindowID);
  if (!listeners) {
    return;
  }

  uint32_t length = listeners->Length();
  for (uint32_t i = 0; i < length; i++) {
    nsRefPtr<GetUserMediaCallbackMediaStreamListener> listener =
      listeners->ElementAt(i);
    listener->Invalidate();
    listener = nullptr;
  }
  listeners->Clear();

  mActiveWindows.Remove(aWindowID);
}

nsresult
MediaManager::Observe(nsISupports* aSubject, const char* aTopic,
  const PRUnichar* aData)
{
  if (strcmp(aTopic, "xpcom-shutdown")) {
    return NS_OK;
  }

  nsCOMPtr<nsIObserverService> obs = mozilla::services::GetObserverService();
  obs->RemoveObserver(this, "xpcom-shutdown");

  
  mActiveWindows.Clear();
  sSingleton = nullptr;

  return NS_OK;
}

} 
