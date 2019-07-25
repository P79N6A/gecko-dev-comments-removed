



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
    const nsString& aErrorMsg, PRUint64 aWindowID)
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
  PRUint64 mWindowID;
};







class SuccessCallbackRunnable : public nsRunnable
{
public:
  SuccessCallbackRunnable(nsIDOMGetUserMediaSuccessCallback* aSuccess,
    nsIDOMFile* aFile, PRUint64 aWindowID)
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
  PRUint64 mWindowID;
};







class GetUserMediaStreamRunnable : public nsRunnable
{
public:
  GetUserMediaStreamRunnable(nsIDOMGetUserMediaSuccessCallback* aSuccess,
    MediaEngineSource* aSource, StreamListeners* aListeners,
    PRUint64 aWindowID, TrackID aTrackID)
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
  PRUint64 mWindowID;
  TrackID mTrackID;
};










class GetUserMediaRunnable : public nsRunnable
{
public:
  GetUserMediaRunnable(bool aAudio, bool aVideo, bool aPicture,
    nsIDOMGetUserMediaSuccessCallback* aSuccess,
    nsIDOMGetUserMediaErrorCallback* aError,
    StreamListeners* aListeners, PRUint64 aWindowID)
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

    PRUint32 count = videoSources.Length();
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

    PRUint32 count = videoSources.Length();
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

    PRUint32 count = audioSources.Length();
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
  PRUint64 mWindowID;

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
    if (aWindow->GetPopupControlState() <= openControlled) {
      return NS_ERROR_FAILURE;
    }
    nsCOMPtr<nsIPopupWindowManager> pm =
      do_GetService(NS_POPUPWINDOWMANAGER_CONTRACTID);
    if (!pm) {
      return NS_ERROR_FAILURE;
    }

    PRUint32 permission;
    nsCOMPtr<nsIDocument> doc = aWindow->GetExtantDoc();
    pm->TestPermission(doc->NodePrincipal(), &permission);
    if (aWindow && (permission == nsIPopupWindowManager::DENY_POPUP)) {
      nsCOMPtr<nsIDOMDocument> domDoc = aWindow->GetExtantDocument();
      nsGlobalWindow::FirePopupBlockedEvent(
        domDoc, aWindow, nsnull, EmptyString(), EmptyString()
      );
      return NS_ERROR_FAILURE;
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

  
  
  PRUint64 windowID = aWindow->WindowID();
  StreamListeners* listeners = mActiveWindows.Get(windowID);
  if (!listeners) {
    listeners = new StreamListeners;
    mActiveWindows.Put(windowID, listeners);
  }

  
  
  nsCOMPtr<nsIRunnable> gUMRunnable = new GetUserMediaRunnable(
    audio, video, picture, onSuccess, onError, listeners, windowID
  );

  
  if (!mMediaThread) {
    rv = NS_NewThread(getter_AddRefs(mMediaThread));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  mMediaThread->Dispatch(gUMRunnable, NS_DISPATCH_NORMAL);
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
MediaManager::OnNavigation(PRUint64 aWindowID)
{
  
  
  StreamListeners* listeners = mActiveWindows.Get(aWindowID);
  if (!listeners) {
    return;
  }

  PRUint32 length = listeners->Length();
  for (PRUint32 i = 0; i < length; i++) {
    nsRefPtr<GetUserMediaCallbackMediaStreamListener> listener =
      listeners->ElementAt(i);
    listener->Invalidate();
    listener = nsnull;
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
  sSingleton = nsnull;

  return NS_OK;
}

} 
