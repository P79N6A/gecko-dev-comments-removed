



#include "MediaManager.h"

#include "MediaStreamGraph.h"
#include "MediaEngineDefault.h"

#include "nsIDOMFile.h"
#include "nsIEventTarget.h"
#include "nsIScriptGlobalObject.h"
#include "nsIPopupWindowManager.h"

#include "nsJSUtils.h"
#include "nsDOMFile.h"
#include "nsGlobalWindow.h"

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

  SuccessCallbackRunnable(nsIDOMGetUserMediaSuccessCallback* aSuccess,
    nsIDOMMediaStream* aStream, PRUint64 aWindowID)
    : mSuccess(aSuccess)
    , mStream(aStream)
    , mWindowID(aWindowID) {}

  NS_IMETHOD
  Run()
  {
    
    WindowTable* activeWindows = MediaManager::Get()->GetActiveWindows();
    if (activeWindows->Get(mWindowID)) {
      
      if (mFile) {
        mSuccess->OnSuccess(mFile);
      } else if (mStream) {
        mSuccess->OnSuccess(mStream);
      }
    }
    return NS_OK;
  }

private:
  nsCOMPtr<nsIDOMGetUserMediaSuccessCallback> mSuccess;
  nsCOMPtr<nsIDOMFile> mFile;
  nsCOMPtr<nsIDOMMediaStream> mStream;
  PRUint64 mWindowID;
};





class GetUserMediaCallbackRunnable : public nsRunnable
{
public:
  GetUserMediaCallbackRunnable(MediaEngineSource* aSource, TrackID aId,
    nsIDOMGetUserMediaSuccessCallback* aSuccess,
    nsIDOMGetUserMediaErrorCallback* aError,
    PRUint64 aWindowID,
    StreamListeners* aListeners)
    : mSource(aSource)
    , mId(aId)
    , mSuccess(aSuccess)
    , mError(aError)
    , mWindowID(aWindowID)
    , mListeners(aListeners) {}

  NS_IMETHOD
  Run()
  {
    







    nsCOMPtr<nsDOMMediaStream> comStream = mSource->Allocate();
    if (!comStream) {
      NS_DispatchToMainThread(new ErrorCallbackRunnable(
        mError, NS_LITERAL_STRING("HARDWARE_UNAVAILABLE"), mWindowID
      ));
      return NS_OK;
    }

    
    
    
    GetUserMediaCallbackMediaStreamListener* listener =
      new GetUserMediaCallbackMediaStreamListener(mSource, comStream, mId);
    comStream->GetStream()->AddListener(listener);

    {
      MutexAutoLock lock(*(MediaManager::Get()->GetLock()));
      mListeners->AppendElement(listener);
    }

    
    NS_DispatchToMainThread(new SuccessCallbackRunnable(
      mSuccess, comStream.get(), mWindowID
    ));
    return NS_OK;
  }

private:
  nsCOMPtr<MediaEngineSource> mSource;
  TrackID mId;
  nsCOMPtr<nsIDOMGetUserMediaSuccessCallback> mSuccess;
  nsCOMPtr<nsIDOMGetUserMediaErrorCallback> mError;
  PRUint64 mWindowID;
  StreamListeners* mListeners;
};






class GetUserMediaSnapshotCallbackRunable : public nsRunnable
{
public:
  GetUserMediaSnapshotCallbackRunable(MediaEngineSource* aSource,
    PRUint32 aDuration,
    nsIDOMGetUserMediaSuccessCallback* aSuccessCallback,
    nsIDOMGetUserMediaErrorCallback* aErrorCallback,
    nsPIDOMWindow* aWindow)
    : mSource(aSource)
    , mDuration(aDuration)
    , mSuccessCallback(aSuccessCallback)
    , mErrorCallback(aErrorCallback)
    , mWindow(aWindow) {}

  NS_IMETHOD
  Run()
  {
    mWindowID = mWindow->WindowID();

    
    
    

    if (mWindow->GetPopupControlState() <= openControlled) {
      return NS_OK;
    }
    
    nsCOMPtr<nsIPopupWindowManager> pm =
      do_GetService(NS_POPUPWINDOWMANAGER_CONTRACTID);
    if (!pm) {
      return NS_OK;
    }

    PRUint32 permission;
    nsCOMPtr<nsIDocument> doc = mWindow->GetExtantDoc();
    pm->TestPermission(doc->GetDocumentURI(), &permission);
    if (permission == nsIPopupWindowManager::DENY_POPUP) {
      nsCOMPtr<nsIDOMDocument> domDoc = mWindow->GetExtantDocument();
      nsGlobalWindow::FirePopupBlockedEvent(
        domDoc, mWindow, nsnull, EmptyString(), EmptyString()
      );
      return NS_OK;
    }

    nsCOMPtr<nsDOMMediaStream> comStream = mSource->Allocate();
    if (!comStream) {
      NS_DispatchToMainThread(new ErrorCallbackRunnable(
        mErrorCallback, NS_LITERAL_STRING("HARDWARE_UNAVAILABLE"), mWindowID
      ));
      return NS_OK;
    }

    nsCOMPtr<nsIDOMFile> file;
    mSource->Snapshot(mDuration, getter_AddRefs(file));
    mSource->Deallocate();

    NS_DispatchToMainThread(new SuccessCallbackRunnable(
      mSuccessCallback, file, mWindowID
    ));
    return NS_OK;
  }

private:
  nsCOMPtr<MediaEngineSource> mSource;
  PRUint32 mDuration;
  nsCOMPtr<nsIDOMGetUserMediaSuccessCallback> mSuccessCallback;
  nsCOMPtr<nsIDOMGetUserMediaErrorCallback>  mErrorCallback;
  nsCOMPtr<nsPIDOMWindow> mWindow;

  PRUint64 mWindowID;
};









class GetUserMediaRunnable : public nsRunnable
{
public:
  GetUserMediaRunnable(bool aAudio, bool aVideo, bool aPicture,
    nsIDOMGetUserMediaSuccessCallback* aSuccess,
    nsIDOMGetUserMediaErrorCallback* aError,
    nsPIDOMWindow* aWindow, StreamListeners* aListeners)
    : mAudio(aAudio)
    , mVideo(aVideo)
    , mPicture(aPicture)
    , mSuccess(aSuccess)
    , mError(aError)
    , mWindow(aWindow)
    , mListeners(aListeners) {}

  ~GetUserMediaRunnable() {}

  
  enum {
    kVideoTrack = 1,
    kAudioTrack = 2
  };

  NS_IMETHOD
  Run()
  {
    mManager = MediaManager::Get();
    mWindowID = mWindow->WindowID();

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
    MediaEngineVideoSource* videoSource = videoSources[count - 1];
    NS_DispatchToMainThread(new GetUserMediaSnapshotCallbackRunable(
      videoSource, 0 , mSuccess, mError, mWindow
    ));
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

    MediaEngineVideoSource* videoSource = videoSources[count - 1];
    NS_DispatchToMainThread(new GetUserMediaCallbackRunnable(
      videoSource, kVideoTrack, mSuccess, mError, mWindowID, mListeners
    ));
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

    MediaEngineAudioSource* audioSource = audioSources[count - 1];
    NS_DispatchToMainThread(new GetUserMediaCallbackRunnable(
      audioSource, kAudioTrack, mSuccess, mError, mWindowID, mListeners
    ));
  }

private:
  bool mAudio;
  bool mVideo;
  bool mPicture;

  nsCOMPtr<nsIDOMGetUserMediaSuccessCallback> mSuccess;
  nsCOMPtr<nsIDOMGetUserMediaErrorCallback> mError;
  nsCOMPtr<nsPIDOMWindow> mWindow;
  StreamListeners* mListeners;

  MediaManager* mManager;
  PRUint64 mWindowID;
};


nsRefPtr<MediaManager> MediaManager::sSingleton;

NS_IMPL_ISUPPORTS1(MediaManager, nsIObserver)






nsresult
MediaManager::GetUserMedia(nsPIDOMWindow* aWindow, nsIMediaStreamOptions* aParams,
  nsIDOMGetUserMediaSuccessCallback* onSuccess,
  nsIDOMGetUserMediaErrorCallback* onError)
{
  NS_ENSURE_TRUE(aParams, NS_ERROR_NULL_POINTER);

  bool audio, video, picture;
  nsresult rv = aParams->GetPicture(&picture);
  NS_ENSURE_SUCCESS(rv, rv);

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
    audio, video, picture, onSuccess, onError, aWindow, listeners
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
    mBackend = new MediaEngineDefault();
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

  MutexAutoLock lock(*mLock);
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
