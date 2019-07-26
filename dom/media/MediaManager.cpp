



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
  ErrorCallbackRunnable(
    already_AddRefed<nsIDOMGetUserMediaSuccessCallback> aSuccess,
    already_AddRefed<nsIDOMGetUserMediaErrorCallback> aError,
    const nsString& aErrorMsg, uint64_t aWindowID)
    : mSuccess(aSuccess)
    , mError(aError)
    , mErrorMsg(aErrorMsg)
    , mWindowID(aWindowID) {}

  NS_IMETHOD
  Run()
  {
    
    NS_ASSERTION(NS_IsMainThread(), "Only call on main thread");

    nsCOMPtr<nsIDOMGetUserMediaSuccessCallback> success(mSuccess);
    nsCOMPtr<nsIDOMGetUserMediaErrorCallback> error(mError);

    WindowTable* activeWindows = MediaManager::Get()->GetActiveWindows();
    if (activeWindows->Get(mWindowID)) {
      error->OnError(mErrorMsg);
    }
    return NS_OK;
  }

private:
  already_AddRefed<nsIDOMGetUserMediaSuccessCallback> mSuccess;
  already_AddRefed<nsIDOMGetUserMediaErrorCallback> mError;
  const nsString mErrorMsg;
  uint64_t mWindowID;
};







class SuccessCallbackRunnable : public nsRunnable
{
public:
  SuccessCallbackRunnable(
    already_AddRefed<nsIDOMGetUserMediaSuccessCallback> aSuccess,
    already_AddRefed<nsIDOMGetUserMediaErrorCallback> aError,
    nsIDOMFile* aFile, uint64_t aWindowID)
    : mSuccess(aSuccess)
    , mError(aError)
    , mFile(aFile)
    , mWindowID(aWindowID) {}

  NS_IMETHOD
  Run()
  {
    
    NS_ASSERTION(NS_IsMainThread(), "Only call on main thread");

    nsCOMPtr<nsIDOMGetUserMediaSuccessCallback> success(mSuccess);
    nsCOMPtr<nsIDOMGetUserMediaErrorCallback> error(mError);

    WindowTable* activeWindows = MediaManager::Get()->GetActiveWindows();
    if (activeWindows->Get(mWindowID)) {
      
      success->OnSuccess(mFile);
    }
    return NS_OK;
  }

private:
  already_AddRefed<nsIDOMGetUserMediaSuccessCallback> mSuccess;
  already_AddRefed<nsIDOMGetUserMediaErrorCallback> mError;
  nsCOMPtr<nsIDOMFile> mFile;
  uint64_t mWindowID;
};






class DeviceSuccessCallbackRunnable: public nsRunnable
{
public:
  DeviceSuccessCallbackRunnable(
    already_AddRefed<nsIGetUserMediaDevicesSuccessCallback> aSuccess,
    already_AddRefed<nsIDOMGetUserMediaErrorCallback> aError,
    const nsTArray<nsCOMPtr<nsIMediaDevice> >& aDevices)
    : mSuccess(aSuccess)
    , mError(aError)
    , mDevices(aDevices) {}

  NS_IMETHOD
  Run()
  {
    NS_ASSERTION(NS_IsMainThread(), "Only call on main thread");

    nsCOMPtr<nsIGetUserMediaDevicesSuccessCallback> success(mSuccess);
    nsCOMPtr<nsIDOMGetUserMediaErrorCallback> error(mError);

    nsCOMPtr<nsIWritableVariant> devices =
      do_CreateInstance("@mozilla.org/variant;1");

    int32_t len = mDevices.Length();
    if (len == 0) {
      devices->SetAsEmptyArray();
      success->OnSuccess(devices);
      return NS_OK;
    }

    nsTArray<nsIMediaDevice*> tmp(len);
    for (int32_t i = 0; i < len; i++) {
      tmp.AppendElement(mDevices.ElementAt(i));
    }

    devices->SetAsArray(nsIDataType::VTYPE_INTERFACE,
                        &NS_GET_IID(nsIMediaDevice),
                        mDevices.Length(),
                        const_cast<void*>(
                          static_cast<const void*>(tmp.Elements())
                        ));

    success->OnSuccess(devices);
    return NS_OK;
  }

private:
  already_AddRefed<nsIGetUserMediaDevicesSuccessCallback> mSuccess;
  already_AddRefed<nsIDOMGetUserMediaErrorCallback> mError;
  nsTArray<nsCOMPtr<nsIMediaDevice> > mDevices;
};




NS_IMPL_THREADSAFE_ISUPPORTS1(MediaDevice, nsIMediaDevice)

NS_IMETHODIMP
MediaDevice::GetName(nsAString& aName)
{
  aName.Assign(mName);
  return NS_OK;
}

NS_IMETHODIMP
MediaDevice::GetType(nsAString& aType)
{
  aType.Assign(mType);
  return NS_OK;
}

MediaEngineSource*
MediaDevice::GetSource()
{
  return mSource;
}








class GetUserMediaStreamRunnable : public nsRunnable
{
public:
  GetUserMediaStreamRunnable(
    already_AddRefed<nsIDOMGetUserMediaSuccessCallback> aSuccess,
    already_AddRefed<nsIDOMGetUserMediaErrorCallback> aError,
    MediaEngineSource* aSource, StreamListeners* aListeners,
    uint64_t aWindowID, TrackID aTrackID)
    : mSuccess(aSuccess)
    , mError(aError)
    , mSource(aSource)
    , mListeners(aListeners)
    , mWindowID(aWindowID)
    , mTrackID(aTrackID) {}

  ~GetUserMediaStreamRunnable() {}

  NS_IMETHOD
  Run()
  {
    NS_ASSERTION(NS_IsMainThread(), "Only call on main thread");

    
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

    
    nsCOMPtr<nsIDOMGetUserMediaSuccessCallback> success(mSuccess);
    nsCOMPtr<nsIDOMGetUserMediaErrorCallback> error(mError);

    WindowTable* activeWindows = MediaManager::Get()->GetActiveWindows();
    if (activeWindows->Get(mWindowID)) {
      success->OnSuccess(stream);
    }

    return NS_OK;
  }

private:
  already_AddRefed<nsIDOMGetUserMediaSuccessCallback> mSuccess;
  already_AddRefed<nsIDOMGetUserMediaErrorCallback> mError;
  nsRefPtr<MediaEngineSource> mSource;
  StreamListeners* mListeners;
  uint64_t mWindowID;
  TrackID mTrackID;
};










class GetUserMediaRunnable : public nsRunnable
{
public:
  



  GetUserMediaRunnable(bool aAudio, bool aVideo, bool aPicture,
    already_AddRefed<nsIDOMGetUserMediaSuccessCallback> aSuccess,
    already_AddRefed<nsIDOMGetUserMediaErrorCallback> aError,
    StreamListeners* aListeners, uint64_t aWindowID, MediaDevice* aDevice)
    : mAudio(aAudio)
    , mVideo(aVideo)
    , mPicture(aPicture)
    , mSuccess(aSuccess)
    , mError(aError)
    , mListeners(aListeners)
    , mWindowID(aWindowID)
    , mDevice(aDevice)
    , mInited(true) {}

  GetUserMediaRunnable(bool aAudio, bool aVideo, bool aPicture,
    already_AddRefed<nsIDOMGetUserMediaSuccessCallback> aSuccess,
    already_AddRefed<nsIDOMGetUserMediaErrorCallback> aError,
    StreamListeners* aListeners, uint64_t aWindowID)
    : mAudio(aAudio)
    , mVideo(aVideo)
    , mPicture(aPicture)
    , mSuccess(aSuccess)
    , mError(aError)
    , mListeners(aListeners)
    , mWindowID(aWindowID)
    , mInited(false) {}

  ~GetUserMediaRunnable() {}

  
  enum {
    kVideoTrack = 1,
    kAudioTrack = 2
  };

  NS_IMETHOD
  Run()
  {
    NS_ASSERTION(!NS_IsMainThread(), "Don't call on main thread");

    mManager = MediaManager::Get();

    
    if (!mInited) {
      nsresult rv = SelectDevice();
      if (rv != NS_OK) {
        return rv;
      }
      mInited = true;
    }

    
    if (mPicture && (mAudio || mVideo)) {
      NS_DispatchToMainThread(new ErrorCallbackRunnable(
        mSuccess, mError, NS_LITERAL_STRING("NOT_SUPPORTED_ERR"), mWindowID
      ));
      return NS_OK;
    }

    
    if (mAudio && mVideo) {
      NS_DispatchToMainThread(new ErrorCallbackRunnable(
        mSuccess, mError, NS_LITERAL_STRING("NOT_IMPLEMENTED"), mWindowID
      ));
      return NS_OK;
    }

    if (mPicture) {
      ProcessGetUserMediaSnapshot(mDevice->GetSource(), 0);
      return NS_OK;
    }

    if (mVideo) {
      ProcessGetUserMedia(mDevice->GetSource(), kVideoTrack);
      return NS_OK;
    }

    if (mAudio) {
      ProcessGetUserMedia(mDevice->GetSource(), kAudioTrack);
      return NS_OK;
    }

    return NS_OK;
  }

  nsresult
  SelectDevice()
  {
    uint32_t count;
    if (mPicture || mVideo) {
      nsTArray<nsRefPtr<MediaEngineVideoSource> > videoSources;
      mManager->GetBackend()->EnumerateVideoDevices(&videoSources);

      count = videoSources.Length();
      if (count <= 0) {
        NS_DispatchToMainThread(new ErrorCallbackRunnable(
          mSuccess, mError, NS_LITERAL_STRING("NO_DEVICES_FOUND"), mWindowID
        ));
        return NS_ERROR_FAILURE;
      }
      mDevice = new MediaDevice(videoSources[0]);
    } else {
      nsTArray<nsRefPtr<MediaEngineAudioSource> > audioSources;
      mManager->GetBackend()->EnumerateAudioDevices(&audioSources);

      count = audioSources.Length();
      if (count <= 0) {
        NS_DispatchToMainThread(new ErrorCallbackRunnable(
          mSuccess, mError, NS_LITERAL_STRING("NO_DEVICES_FOUND"), mWindowID
        ));
        return NS_ERROR_FAILURE;
      }
      mDevice = new MediaDevice(audioSources[0]);
    }

    return NS_OK;
  }

  



  void
  ProcessGetUserMedia(MediaEngineSource* aSource, TrackID aTrackID)
  {
    nsresult rv = aSource->Allocate();
    if (NS_FAILED(rv)) {
      NS_DispatchToMainThread(new ErrorCallbackRunnable(
        mSuccess, mError, NS_LITERAL_STRING("HARDWARE_UNAVAILABLE"), mWindowID
      ));
      return;
    }

    NS_DispatchToMainThread(new GetUserMediaStreamRunnable(
      mSuccess, mError, aSource, mListeners, mWindowID, aTrackID
    ));
    return;
  }

  



  void
  ProcessGetUserMediaSnapshot(MediaEngineSource* aSource, int aDuration)
  {
    nsresult rv = aSource->Allocate();
    if (NS_FAILED(rv)) {
      NS_DispatchToMainThread(new ErrorCallbackRunnable(
        mSuccess, mError, NS_LITERAL_STRING("HARDWARE_UNAVAILABLE"), mWindowID
      ));
      return;
    }

    


    nsCOMPtr<nsIDOMFile> file;
    aSource->Snapshot(aDuration, getter_AddRefs(file));
    aSource->Deallocate();

    NS_DispatchToMainThread(new SuccessCallbackRunnable(
      mSuccess, mError, file, mWindowID
    ));
    return;
  }

private:
  bool mAudio;
  bool mVideo;
  bool mPicture;

  already_AddRefed<nsIDOMGetUserMediaSuccessCallback> mSuccess;
  already_AddRefed<nsIDOMGetUserMediaErrorCallback> mError;
  StreamListeners* mListeners;
  uint64_t mWindowID;
  nsRefPtr<MediaDevice> mDevice;

  bool mInited;
  MediaManager* mManager;
};







class GetUserMediaDevicesRunnable : public nsRunnable
{
public:
  GetUserMediaDevicesRunnable(
    already_AddRefed<nsIGetUserMediaDevicesSuccessCallback> aSuccess,
    already_AddRefed<nsIDOMGetUserMediaErrorCallback> aError)
    : mSuccess(aSuccess)
    , mError(aError) {}
  ~GetUserMediaDevicesRunnable() {}

  NS_IMETHOD
  Run()
  {
    NS_ASSERTION(!NS_IsMainThread(), "Don't call on main thread");

    uint32_t audioCount, videoCount, total, i;
    MediaManager* manager = MediaManager::Get();

    nsTArray<nsRefPtr<MediaEngineVideoSource> > videoSources;
    manager->GetBackend()->EnumerateVideoDevices(&videoSources);
    videoCount = videoSources.Length();

    nsTArray<nsRefPtr<MediaEngineAudioSource> > audioSources;
    manager->GetBackend()->EnumerateAudioDevices(&audioSources);
    audioCount = videoSources.Length();

    total = videoCount + audioCount;

    nsTArray<nsCOMPtr<nsIMediaDevice> > *devices =
      new nsTArray<nsCOMPtr<nsIMediaDevice> >;

    for (i = 0; i < videoCount; i++) {
      devices->AppendElement(new MediaDevice(videoSources[i]));
    }
    for (i = 0; i < audioCount; i++) {
      devices->AppendElement(new MediaDevice(audioSources[i]));
    }

    NS_DispatchToMainThread(new DeviceSuccessCallbackRunnable(
      mSuccess, mError, *devices
    ));
    return NS_OK;
  }

private:
  already_AddRefed<nsIGetUserMediaDevicesSuccessCallback> mSuccess;
  already_AddRefed<nsIDOMGetUserMediaErrorCallback> mError;
};

nsRefPtr<MediaManager> MediaManager::sSingleton;

NS_IMPL_THREADSAFE_ISUPPORTS1(MediaManager, nsIObserver)






nsresult
MediaManager::GetUserMedia(bool aPrivileged, nsPIDOMWindow* aWindow,
  nsIMediaStreamOptions* aParams,
  nsIDOMGetUserMediaSuccessCallback* aOnSuccess,
  nsIDOMGetUserMediaErrorCallback* aOnError)
{
  NS_ASSERTION(NS_IsMainThread(), "Only call on main thread");

  NS_ENSURE_TRUE(aParams, NS_ERROR_NULL_POINTER);
  NS_ENSURE_TRUE(aWindow, NS_ERROR_NULL_POINTER);

  nsCOMPtr<nsIDOMGetUserMediaSuccessCallback> onSuccess(aOnSuccess);
  nsCOMPtr<nsIDOMGetUserMediaErrorCallback> onError(aOnError);

  
  nsresult rv;
  bool audio, video, picture;

  rv = aParams->GetPicture(&picture);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = aParams->GetAudio(&audio);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = aParams->GetVideo(&video);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIMediaDevice> device;
  rv = aParams->GetDevice(getter_AddRefs(device));
  NS_ENSURE_SUCCESS(rv, rv);

  
  if (device) {
    nsString type;
    device->GetType(type);
    if ((picture || video) && !type.EqualsLiteral("video")) {
      return NS_ERROR_FAILURE;
    }
    if (audio && !type.EqualsLiteral("audio")) {
      return NS_ERROR_FAILURE;
    }
  }

  
  nsString cameraType;
  rv = aParams->GetCamera(cameraType);
  NS_ENSURE_SUCCESS(rv, rv);

  






#if !defined(MOZ_WEBRTC)
  if (picture && !aPrivileged) {
    if (aWindow->GetPopupControlState() > openControlled) {
      nsCOMPtr<nsIPopupWindowManager> pm =
        do_GetService(NS_POPUPWINDOWMANAGER_CONTRACTID);
      if (!pm) {
        return NS_OK;
      }
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

  




  if (!aPrivileged && !picture) {
    
    
  }

  
  
  uint64_t windowID = aWindow->WindowID();
  StreamListeners* listeners = mActiveWindows.Get(windowID);
  if (!listeners) {
    listeners = new StreamListeners;
    mActiveWindows.Put(windowID, listeners);
  }

  






  nsCOMPtr<nsIRunnable> gUMRunnable;
  if (device) {
    gUMRunnable = new GetUserMediaRunnable(
      audio, video, picture, onSuccess.forget(), onError.forget(), listeners,
      windowID, static_cast<MediaDevice*>(device.get())
    );
  } else {
    gUMRunnable = new GetUserMediaRunnable(
      audio, video, picture, onSuccess.forget(), onError.forget(), listeners,
      windowID
    );
  }

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

nsresult
MediaManager::GetUserMediaDevices(nsPIDOMWindow* aWindow,
  nsIGetUserMediaDevicesSuccessCallback* aOnSuccess,
  nsIDOMGetUserMediaErrorCallback* aOnError)
{
  NS_ASSERTION(NS_IsMainThread(), "Only call on main thread");

  nsCOMPtr<nsIGetUserMediaDevicesSuccessCallback> onSuccess(aOnSuccess);
  nsCOMPtr<nsIDOMGetUserMediaErrorCallback> onError(aOnError);

  nsCOMPtr<nsIRunnable> gUMDRunnable = new GetUserMediaDevicesRunnable(
    onSuccess.forget(), onError.forget()
  );

  nsCOMPtr<nsIThread> deviceThread;
  nsresult rv = NS_NewThread(getter_AddRefs(deviceThread));
  NS_ENSURE_SUCCESS(rv, rv);


  deviceThread->Dispatch(gUMDRunnable, NS_DISPATCH_NORMAL);
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
