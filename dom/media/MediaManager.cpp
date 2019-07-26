



#include "MediaManager.h"

#include "MediaStreamGraph.h"
#include "nsIDOMFile.h"
#include "nsIEventTarget.h"
#include "nsIUUIDGenerator.h"
#include "nsIScriptGlobalObject.h"
#include "nsIPopupWindowManager.h"
#include "nsISupportsArray.h"
#include "nsIPrefService.h"
#include "nsIPrefBranch.h"


#include "prprf.h"

#include "nsJSUtils.h"
#include "nsDOMFile.h"
#include "nsGlobalWindow.h"

#include "mozilla/Preferences.h"


#include "MediaEngineDefault.h"
#if defined(MOZ_WEBRTC)
#include "MediaEngineWebRTC.h"
#endif

namespace mozilla {

#ifdef LOG
#undef LOG
#endif

#ifdef PR_LOGGING
PRLogModuleInfo*
GetMediaManagerLog()
{
  static PRLogModuleInfo *sLog;
  if (!sLog)
    sLog = PR_NewLogModule("MediaManager");
  return sLog;
}
#define LOG(msg) PR_LOG(GetMediaManagerLog(), PR_LOG_DEBUG, msg)
#else
#define LOG(msg)
#endif







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
    , mWindowID(aWindowID)
    , mManager(MediaManager::GetInstance()) {}

  NS_IMETHOD
  Run()
  {
    
    NS_ASSERTION(NS_IsMainThread(), "Only call on main thread");

    nsCOMPtr<nsIDOMGetUserMediaSuccessCallback> success(mSuccess);
    nsCOMPtr<nsIDOMGetUserMediaErrorCallback> error(mError);

    if (!(mManager->IsWindowStillActive(mWindowID))) {
      return NS_OK;
    }
    
    
    error->OnError(mErrorMsg);
    return NS_OK;
  }

private:
  already_AddRefed<nsIDOMGetUserMediaSuccessCallback> mSuccess;
  already_AddRefed<nsIDOMGetUserMediaErrorCallback> mError;
  const nsString mErrorMsg;
  uint64_t mWindowID;
  nsRefPtr<MediaManager> mManager; 
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
    , mWindowID(aWindowID)
    , mManager(MediaManager::GetInstance()) {}

  NS_IMETHOD
  Run()
  {
    
    NS_ASSERTION(NS_IsMainThread(), "Only call on main thread");

    nsCOMPtr<nsIDOMGetUserMediaSuccessCallback> success(mSuccess);
    nsCOMPtr<nsIDOMGetUserMediaErrorCallback> error(mError);

    if (!(mManager->IsWindowStillActive(mWindowID))) {
      return NS_OK;
    }
    
    
    success->OnSuccess(mFile);
    return NS_OK;
  }

private:
  already_AddRefed<nsIDOMGetUserMediaSuccessCallback> mSuccess;
  already_AddRefed<nsIDOMGetUserMediaErrorCallback> mError;
  nsCOMPtr<nsIDOMFile> mFile;
  uint64_t mWindowID;
  nsRefPtr<MediaManager> mManager; 
};






class DeviceSuccessCallbackRunnable: public nsRunnable
{
public:
  DeviceSuccessCallbackRunnable(
    already_AddRefed<nsIGetUserMediaDevicesSuccessCallback> aSuccess,
    already_AddRefed<nsIDOMGetUserMediaErrorCallback> aError,
    nsTArray<nsCOMPtr<nsIMediaDevice> >* aDevices)
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

    int32_t len = mDevices->Length();
    if (len == 0) {
      
      
      
      
      error->OnError(NS_LITERAL_STRING("NO_DEVICES_FOUND"));
      return NS_OK;
    }

    nsTArray<nsIMediaDevice*> tmp(len);
    for (int32_t i = 0; i < len; i++) {
      tmp.AppendElement(mDevices->ElementAt(i));
    }

    devices->SetAsArray(nsIDataType::VTYPE_INTERFACE,
                        &NS_GET_IID(nsIMediaDevice),
                        mDevices->Length(),
                        const_cast<void*>(
                          static_cast<const void*>(tmp.Elements())
                        ));

    success->OnSuccess(devices);
    return NS_OK;
  }

private:
  already_AddRefed<nsIGetUserMediaDevicesSuccessCallback> mSuccess;
  already_AddRefed<nsIDOMGetUserMediaErrorCallback> mError;
  nsAutoPtr<nsTArray<nsCOMPtr<nsIMediaDevice> > > mDevices;
};


class GetUserMediaListenerRemove: public nsRunnable
{
public:
  GetUserMediaListenerRemove(uint64_t aWindowID,
    GetUserMediaCallbackMediaStreamListener *aListener)
    : mWindowID(aWindowID)
    , mListener(aListener) {}

  NS_IMETHOD
  Run()
  {
    NS_ASSERTION(NS_IsMainThread(), "Only call on main thread");
    nsRefPtr<MediaManager> manager(MediaManager::GetInstance());
    manager->RemoveFromWindowList(mWindowID, mListener);
    return NS_OK;
  }

protected:
  uint64_t mWindowID;
  nsRefPtr<GetUserMediaCallbackMediaStreamListener> mListener;
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

NS_IMETHODIMP
MediaDevice::GetId(nsAString& aID)
{
  aID.Assign(mID);
  return NS_OK;
}

MediaEngineSource*
MediaDevice::GetSource()
{
  return mSource;
}





class nsDOMUserMediaStream : public DOMLocalMediaStream
{
public:
  static already_AddRefed<nsDOMUserMediaStream>
  CreateTrackUnionStream(uint32_t aHintContents)
  {
    nsRefPtr<nsDOMUserMediaStream> stream = new nsDOMUserMediaStream();
    stream->InitTrackUnionStream(aHintContents);
    return stream.forget();
  }

  virtual ~nsDOMUserMediaStream()
  {
    Stop();

    if (mPort) {
      mPort->Destroy();
    }
    if (mSourceStream) {
      mSourceStream->Destroy();
    }
  }

  NS_IMETHODIMP Stop()
  {
    if (mSourceStream) {
      mSourceStream->EndAllTrackAndFinish();
    }
    return NS_OK;
  }

  
  
  nsRefPtr<SourceMediaStream> mSourceStream;
  nsRefPtr<MediaInputPort> mPort;
};















class GetUserMediaStreamRunnable : public nsRunnable
{
public:
  GetUserMediaStreamRunnable(
    already_AddRefed<nsIDOMGetUserMediaSuccessCallback> aSuccess,
    already_AddRefed<nsIDOMGetUserMediaErrorCallback> aError,
    uint64_t aWindowID,
    GetUserMediaCallbackMediaStreamListener* aListener,
    MediaEngineSource* aAudioSource,
    MediaEngineSource* aVideoSource)
    : mSuccess(aSuccess)
    , mError(aError)
    , mAudioSource(aAudioSource)
    , mVideoSource(aVideoSource)
    , mWindowID(aWindowID)
    , mListener(aListener)
    , mManager(MediaManager::GetInstance()) {}

  ~GetUserMediaStreamRunnable() {}

  NS_IMETHOD
  Run()
  {
    NS_ASSERTION(NS_IsMainThread(), "Only call on main thread");

    
    
    StreamListeners* listeners = mManager->GetWindowListeners(mWindowID);
    if (!listeners) {
      
      return NS_OK;
    }

    
    uint32_t hints = (mAudioSource ? DOMMediaStream::HINT_CONTENTS_AUDIO : 0);
    hints |= (mVideoSource ? DOMMediaStream::HINT_CONTENTS_VIDEO : 0);

    nsRefPtr<nsDOMUserMediaStream> trackunion =
      nsDOMUserMediaStream::CreateTrackUnionStream(hints);
    if (!trackunion) {
      nsCOMPtr<nsIDOMGetUserMediaErrorCallback> error(mError);
      LOG(("Returning error for getUserMedia() - no stream"));
      error->OnError(NS_LITERAL_STRING("NO_STREAM"));
      return NS_OK;
    }

    MediaStreamGraph* gm = MediaStreamGraph::GetInstance();
    nsRefPtr<SourceMediaStream> stream = gm->CreateSourceStream(nullptr);

    
    trackunion->GetStream()->AsProcessedStream()->SetAutofinish(true);
    nsRefPtr<MediaInputPort> port = trackunion->GetStream()->AsProcessedStream()->
      AllocateInputPort(stream, MediaInputPort::FLAG_BLOCK_OUTPUT);
    trackunion->mSourceStream = stream;
    trackunion->mPort = port.forget();

    nsPIDOMWindow *window = static_cast<nsPIDOMWindow*>
      (nsGlobalWindow::GetInnerWindowWithId(mWindowID));
    if (window && window->GetExtantDoc()) {
      trackunion->CombineWithPrincipal(window->GetExtantDoc()->NodePrincipal());
    }

    
    
    
    
    mListener->Activate(stream.forget(), mAudioSource, mVideoSource);

    
    
    nsIThread *mediaThread = MediaManager::GetThread();
    nsRefPtr<MediaOperationRunnable> runnable(
      new MediaOperationRunnable(MEDIA_START, mListener,
                                 mAudioSource, mVideoSource, false));
    mediaThread->Dispatch(runnable, NS_DISPATCH_NORMAL);

#ifdef MOZ_WEBRTC
    
    nsresult rv;
    nsCOMPtr<nsIPrefService> prefs = do_GetService("@mozilla.org/preferences-service;1", &rv);
    if (NS_SUCCEEDED(rv)) {
      nsCOMPtr<nsIPrefBranch> branch = do_QueryInterface(prefs);

      if (branch) {
        int32_t aec = (int32_t) webrtc::kEcUnchanged;
        int32_t agc = (int32_t) webrtc::kAgcUnchanged;
        int32_t noise = (int32_t) webrtc::kNsUnchanged;
        bool aec_on = false, agc_on = false, noise_on = false;

        branch->GetBoolPref("media.peerconnection.aec_enabled", &aec_on);
        branch->GetIntPref("media.peerconnection.aec", &aec);
        branch->GetBoolPref("media.peerconnection.agc_enabled", &agc_on);
        branch->GetIntPref("media.peerconnection.agc", &agc);
        branch->GetBoolPref("media.peerconnection.noise_enabled", &noise_on);
        branch->GetIntPref("media.peerconnection.noise", &noise);

        mListener->AudioConfig(aec_on, (uint32_t) aec,
                               agc_on, (uint32_t) agc,
                               noise_on, (uint32_t) noise);
      }
    }
#endif

    
    nsCOMPtr<nsIDOMGetUserMediaSuccessCallback> success(mSuccess);
    nsCOMPtr<nsIDOMGetUserMediaErrorCallback> error(mError);

    if (!(mManager->IsWindowStillActive(mWindowID))) {
      return NS_OK;
    }
    
    
    LOG(("Returning success for getUserMedia()"));
    success->OnSuccess(static_cast<nsIDOMLocalMediaStream*>(trackunion));

    return NS_OK;
  }

private:
  already_AddRefed<nsIDOMGetUserMediaSuccessCallback> mSuccess;
  already_AddRefed<nsIDOMGetUserMediaErrorCallback> mError;
  nsRefPtr<MediaEngineSource> mAudioSource;
  nsRefPtr<MediaEngineSource> mVideoSource;
  uint64_t mWindowID;
  nsRefPtr<GetUserMediaCallbackMediaStreamListener> mListener;
  nsRefPtr<MediaManager> mManager; 
};










class GetUserMediaRunnable : public nsRunnable
{
public:
  



  GetUserMediaRunnable(bool aAudio, bool aVideo, bool aPicture,
    already_AddRefed<nsIDOMGetUserMediaSuccessCallback> aSuccess,
    already_AddRefed<nsIDOMGetUserMediaErrorCallback> aError,
    uint64_t aWindowID, GetUserMediaCallbackMediaStreamListener *aListener,
    MediaDevice* aAudioDevice, MediaDevice* aVideoDevice)
    : mAudio(aAudio)
    , mVideo(aVideo)
    , mPicture(aPicture)
    , mSuccess(aSuccess)
    , mError(aError)
    , mWindowID(aWindowID)
    , mListener(aListener)
    , mDeviceChosen(true)
    , mBackendChosen(false)
    , mManager(MediaManager::GetInstance())
    {
      if (mAudio) {
        mAudioDevice = aAudioDevice;
      }
      if (mVideo) {
        mVideoDevice = aVideoDevice;
      }
    }

  GetUserMediaRunnable(bool aAudio, bool aVideo, bool aPicture,
    already_AddRefed<nsIDOMGetUserMediaSuccessCallback> aSuccess,
    already_AddRefed<nsIDOMGetUserMediaErrorCallback> aError,
    uint64_t aWindowID, GetUserMediaCallbackMediaStreamListener *aListener)
    : mAudio(aAudio)
    , mVideo(aVideo)
    , mPicture(aPicture)
    , mSuccess(aSuccess)
    , mError(aError)
    , mWindowID(aWindowID)
    , mListener(aListener)
    , mDeviceChosen(false)
    , mBackendChosen(false)
    , mManager(MediaManager::GetInstance()) {}

  



  GetUserMediaRunnable(bool aAudio, bool aVideo,
    already_AddRefed<nsIDOMGetUserMediaSuccessCallback> aSuccess,
    already_AddRefed<nsIDOMGetUserMediaErrorCallback> aError,
    uint64_t aWindowID, GetUserMediaCallbackMediaStreamListener *aListener,
    MediaEngine* aBackend)
    : mAudio(aAudio)
    , mVideo(aVideo)
    , mPicture(false)
    , mSuccess(aSuccess)
    , mError(aError)
    , mWindowID(aWindowID)
    , mListener(aListener)
    , mDeviceChosen(false)
    , mBackendChosen(true)
    , mBackend(aBackend)
    , mManager(MediaManager::GetInstance()) {}

  ~GetUserMediaRunnable() {
    if (mBackendChosen) {
      delete mBackend;
    }
  }

  NS_IMETHOD
  Run()
  {
    NS_ASSERTION(!NS_IsMainThread(), "Don't call on main thread");

    
    if (!mBackendChosen) {
      mBackend = mManager->GetBackend();
    }

    
    if (!mDeviceChosen) {
      nsresult rv = SelectDevice();
      if (rv != NS_OK) {
        return rv;
      }
    }

    
    if (mPicture && (mAudio || mVideo)) {
      NS_DispatchToMainThread(new ErrorCallbackRunnable(
        mSuccess, mError, NS_LITERAL_STRING("NOT_SUPPORTED_ERR"), mWindowID
      ));
      return NS_OK;
    }

    if (mPicture) {
      ProcessGetUserMediaSnapshot(mVideoDevice->GetSource(), 0);
      return NS_OK;
    }

    
    ProcessGetUserMedia((mAudio && mAudioDevice) ? mAudioDevice->GetSource() : nullptr,
                        (mVideo && mVideoDevice) ? mVideoDevice->GetSource() : nullptr);
    return NS_OK;
  }

  nsresult
  Denied()
  {
      
      
    if (NS_IsMainThread()) {
      
      
      nsCOMPtr<nsIDOMGetUserMediaErrorCallback> error(mError);
      error->OnError(NS_LITERAL_STRING("PERMISSION_DENIED"));

      
      nsRefPtr<MediaManager> manager(MediaManager::GetInstance());
      manager->RemoveFromWindowList(mWindowID, mListener);
    } else {
      
      
      NS_DispatchToMainThread(new ErrorCallbackRunnable(
        mSuccess, mError, NS_LITERAL_STRING("PERMISSION_DENIED"), mWindowID
      ));

      
      NS_DispatchToMainThread(new GetUserMediaListenerRemove(mWindowID, mListener));
    }

    return NS_OK;
  }

  nsresult
  SetAudioDevice(MediaDevice* aAudioDevice)
  {
    mAudioDevice = aAudioDevice;
    mDeviceChosen = true;
    return NS_OK;
  }

  nsresult
  SetVideoDevice(MediaDevice* aVideoDevice)
  {
    mVideoDevice = aVideoDevice;
    mDeviceChosen = true;
    return NS_OK;
  }

  nsresult
  SelectDevice()
  {
    bool found = false;
    uint32_t count;
    if (mPicture || mVideo) {
      nsTArray<nsRefPtr<MediaEngineVideoSource> > videoSources;
      mBackend->EnumerateVideoDevices(&videoSources);

      count = videoSources.Length();
      if (count <= 0) {
        NS_DispatchToMainThread(new ErrorCallbackRunnable(
          mSuccess, mError, NS_LITERAL_STRING("NO_DEVICES_FOUND"), mWindowID
        ));
        return NS_ERROR_FAILURE;
      }

      
      for (uint32_t i = 0; i < count; i++) {
        nsRefPtr<MediaEngineVideoSource> vSource = videoSources[i];
        if (vSource->IsAvailable()) {
          found = true;
          mVideoDevice = new MediaDevice(videoSources[i]);
          break;
        }
      }

      if (!found) {
        NS_DispatchToMainThread(new ErrorCallbackRunnable(
          mSuccess, mError, NS_LITERAL_STRING("HARDWARE_UNAVAILABLE"), mWindowID
        ));
        return NS_ERROR_FAILURE;
      }
      LOG(("Selected video device"));
    }

    found = false;
    if (mAudio) {
      nsTArray<nsRefPtr<MediaEngineAudioSource> > audioSources;
      mBackend->EnumerateAudioDevices(&audioSources);

      count = audioSources.Length();
      if (count <= 0) {
        NS_DispatchToMainThread(new ErrorCallbackRunnable(
          mSuccess, mError, NS_LITERAL_STRING("NO_DEVICES_FOUND"), mWindowID
        ));
        return NS_ERROR_FAILURE;
      }

      for (uint32_t i = 0; i < count; i++) {
        nsRefPtr<MediaEngineAudioSource> aSource = audioSources[i];
        if (aSource->IsAvailable()) {
          found = true;
          mAudioDevice = new MediaDevice(audioSources[i]);
          break;
        }
      }

      if (!found) {
        NS_DispatchToMainThread(new ErrorCallbackRunnable(
          mSuccess, mError, NS_LITERAL_STRING("HARDWARE_UNAVAILABLE"), mWindowID
        ));
        return NS_ERROR_FAILURE;
      }
      LOG(("Selected audio device"));
    }

    return NS_OK;
  }

  



  void
  ProcessGetUserMedia(MediaEngineSource* aAudioSource, MediaEngineSource* aVideoSource)
  {
    nsresult rv;
    if (aAudioSource) {
      rv = aAudioSource->Allocate();
      if (NS_FAILED(rv)) {
        LOG(("Failed to allocate audiosource %d",rv));
        NS_DispatchToMainThread(new ErrorCallbackRunnable(
                                  mSuccess, mError, NS_LITERAL_STRING("HARDWARE_UNAVAILABLE"), mWindowID
                                                          ));
        return;
      }
    }
    if (aVideoSource) {
      rv = aVideoSource->Allocate();
      if (NS_FAILED(rv)) {
        LOG(("Failed to allocate videosource %d\n",rv));
        if (aAudioSource) {
          aAudioSource->Deallocate();
        }
        NS_DispatchToMainThread(new ErrorCallbackRunnable(
          mSuccess, mError, NS_LITERAL_STRING("HARDWARE_UNAVAILABLE"), mWindowID
                                                          ));
        return;
      }
    }

    NS_DispatchToMainThread(new GetUserMediaStreamRunnable(
      mSuccess, mError, mWindowID, mListener, aAudioSource, aVideoSource
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
  uint64_t mWindowID;
  nsRefPtr<GetUserMediaCallbackMediaStreamListener> mListener;
  nsRefPtr<MediaDevice> mAudioDevice;
  nsRefPtr<MediaDevice> mVideoDevice;

  bool mDeviceChosen;
  bool mBackendChosen;

  MediaEngine* mBackend;
  nsRefPtr<MediaManager> mManager; 
};







class GetUserMediaDevicesRunnable : public nsRunnable
{
public:
  GetUserMediaDevicesRunnable(
    already_AddRefed<nsIGetUserMediaDevicesSuccessCallback> aSuccess,
    already_AddRefed<nsIDOMGetUserMediaErrorCallback> aError)
    : mSuccess(aSuccess)
    , mError(aError)
    , mManager(MediaManager::GetInstance())
    {}
  ~GetUserMediaDevicesRunnable() {}

  NS_IMETHOD
  Run()
  {
    NS_ASSERTION(!NS_IsMainThread(), "Don't call on main thread");

    uint32_t audioCount, videoCount, i;

    nsTArray<nsRefPtr<MediaEngineVideoSource> > videoSources;
    mManager->GetBackend()->EnumerateVideoDevices(&videoSources);
    videoCount = videoSources.Length();

    nsTArray<nsRefPtr<MediaEngineAudioSource> > audioSources;
    mManager->GetBackend()->EnumerateAudioDevices(&audioSources);
    audioCount = audioSources.Length();

    nsTArray<nsCOMPtr<nsIMediaDevice> > *devices =
      new nsTArray<nsCOMPtr<nsIMediaDevice> >;

    





    for (i = 0; i < videoCount; i++) {
      MediaEngineVideoSource *vSource = videoSources[i];
      devices->AppendElement(new MediaDevice(vSource));
    }
    for (i = 0; i < audioCount; i++) {
      MediaEngineAudioSource *aSource = audioSources[i];
      devices->AppendElement(new MediaDevice(aSource));
    }

    NS_DispatchToMainThread(new DeviceSuccessCallbackRunnable(
      mSuccess, mError, devices 
    ));
    return NS_OK;
  }

private:
  already_AddRefed<nsIGetUserMediaDevicesSuccessCallback> mSuccess;
  already_AddRefed<nsIDOMGetUserMediaErrorCallback> mError;
  nsRefPtr<MediaManager> mManager;
};

NS_IMPL_THREADSAFE_ISUPPORTS2(MediaManager, nsIMediaManagerService, nsIObserver)

 StaticRefPtr<MediaManager> MediaManager::sSingleton;

 already_AddRefed<MediaManager>
MediaManager::GetInstance()
{
  
  nsRefPtr<MediaManager> service = MediaManager::Get();
  return service.forget();
}






nsresult
MediaManager::GetUserMedia(bool aPrivileged, nsPIDOMWindow* aWindow,
  nsIMediaStreamOptions* aParams,
  nsIDOMGetUserMediaSuccessCallback* aOnSuccess,
  nsIDOMGetUserMediaErrorCallback* aOnError)
{
  NS_ASSERTION(NS_IsMainThread(), "Only call on main thread");

  NS_ENSURE_TRUE(aParams, NS_ERROR_NULL_POINTER);
  NS_ENSURE_TRUE(aWindow, NS_ERROR_NULL_POINTER);
  NS_ENSURE_TRUE(aOnError, NS_ERROR_NULL_POINTER);
  NS_ENSURE_TRUE(aOnSuccess, NS_ERROR_NULL_POINTER);

  nsCOMPtr<nsIDOMGetUserMediaSuccessCallback> onSuccess(aOnSuccess);
  nsCOMPtr<nsIDOMGetUserMediaErrorCallback> onError(aOnError);

  
  nsresult rv;
  bool fake, audio, video, picture;

  rv = aParams->GetFake(&fake);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = aParams->GetPicture(&picture);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = aParams->GetAudio(&audio);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = aParams->GetVideo(&video);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIMediaDevice> audiodevice;
  rv = aParams->GetAudioDevice(getter_AddRefs(audiodevice));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIMediaDevice> videodevice;
  rv = aParams->GetVideoDevice(getter_AddRefs(videodevice));
  NS_ENSURE_SUCCESS(rv, rv);

  
  if (audiodevice) {
    nsString type;
    audiodevice->GetType(type);
    if (audio && !type.EqualsLiteral("audio")) {
      return NS_ERROR_FAILURE;
    }
  }
  if (videodevice) {
    nsString type;
    videodevice->GetType(type);
    if ((picture || video) && !type.EqualsLiteral("video")) {
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

  static bool created = false;
  if (!created) {
    
    
    (void) MediaManager::Get();
  }

  
  
  uint64_t windowID = aWindow->WindowID();
  nsRefPtr<GetUserMediaRunnable> gUMRunnable;
  
  
  StreamListeners* listeners = GetActiveWindows()->Get(windowID);
  if (!listeners) {
    listeners = new StreamListeners;
    GetActiveWindows()->Put(windowID, listeners);
  }
  
  nsIThread *mediaThread = MediaManager::GetThread();

  
  GetUserMediaCallbackMediaStreamListener* listener =
    new GetUserMediaCallbackMediaStreamListener(mediaThread, windowID);

  
  listeners->AppendElement(listener);

  
  if (Preferences::GetBool("media.navigator.permission.disabled", false)) {
    aPrivileged = true;
  }

  








  if (fake) {
    
    gUMRunnable = new GetUserMediaRunnable(
      audio, video, onSuccess.forget(), onError.forget(), windowID, listener,
      new MediaEngineDefault()
                                           );
  } else if (audiodevice || videodevice) {
    
    gUMRunnable = new GetUserMediaRunnable(
      audio, video, picture, onSuccess.forget(), onError.forget(), windowID, listener,
      static_cast<MediaDevice*>(audiodevice.get()),
      static_cast<MediaDevice*>(videodevice.get())
                                           );
  } else {
    
    gUMRunnable = new GetUserMediaRunnable(
      audio, video, picture, onSuccess.forget(), onError.forget(), windowID, listener
                                           );
  }

#ifdef ANDROID
  if (picture) {
    
    NS_DispatchToMainThread(gUMRunnable);
    return NS_OK;
  }
#endif
  
  if (aPrivileged || fake) {
    mMediaThread->Dispatch(gUMRunnable, NS_DISPATCH_NORMAL);
  } else {
    
    
    
    nsresult rv;
    nsCOMPtr<nsIUUIDGenerator> uuidgen =
      do_GetService("@mozilla.org/uuid-generator;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    
    nsID id;
    rv = uuidgen->GenerateUUIDInPlace(&id);
    NS_ENSURE_SUCCESS(rv, rv);

    char buffer[NSID_LENGTH];
    id.ToProvidedString(buffer);
    NS_ConvertUTF8toUTF16 callID(buffer);

    
    mActiveCallbacks.Put(callID, gUMRunnable);

    
    nsAutoString data;
    data.Append(NS_LITERAL_STRING("{\"windowID\":"));

    
    char windowBuffer[32];
    PR_snprintf(windowBuffer, sizeof(windowBuffer), "%llu",
                aWindow->GetOuterWindow()->WindowID());
    data.Append(NS_ConvertUTF8toUTF16(windowBuffer));

    data.Append(NS_LITERAL_STRING(", \"callID\":\""));
    data.Append(callID);
    data.Append(NS_LITERAL_STRING("\"}"));

    nsCOMPtr<nsIObserverService> obs = services::GetObserverService();
    obs->NotifyObservers(aParams, "getUserMedia:request", data.get());
  }

  return NS_OK;
}

nsresult
MediaManager::GetUserMediaDevices(nsPIDOMWindow* aWindow,
  nsIGetUserMediaDevicesSuccessCallback* aOnSuccess,
  nsIDOMGetUserMediaErrorCallback* aOnError)
{
  NS_ASSERTION(NS_IsMainThread(), "Only call on main thread");

  NS_ENSURE_TRUE(aOnError, NS_ERROR_NULL_POINTER);
  NS_ENSURE_TRUE(aOnSuccess, NS_ERROR_NULL_POINTER);

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
  
  
  
  MutexAutoLock lock(mMutex);
  if (!mBackend) {
#if defined(MOZ_WEBRTC)
    mBackend = new MediaEngineWebRTC();
#else
    mBackend = new MediaEngineDefault();
#endif
  }
  return mBackend;
}

void
MediaManager::OnNavigation(uint64_t aWindowID)
{
  NS_ASSERTION(NS_IsMainThread(), "OnNavigation called off main thread");

  
  

  
  
  StreamListeners* listeners = GetWindowListeners(aWindowID);
  if (!listeners) {
    return;
  }

  uint32_t length = listeners->Length();
  for (uint32_t i = 0; i < length; i++) {
    nsRefPtr<GetUserMediaCallbackMediaStreamListener> listener =
      listeners->ElementAt(i);
    if (listener->Stream()) { 
      listener->Invalidate();
    }
    listener->Remove();
  }
  listeners->Clear();

  RemoveWindowID(aWindowID);
  
}

void
MediaManager::RemoveFromWindowList(uint64_t aWindowID,
  GetUserMediaCallbackMediaStreamListener *aListener)
{
  NS_ASSERTION(NS_IsMainThread(), "RemoveFromWindowList called off main thread");

  
  aListener->Remove(); 

  StreamListeners* listeners = GetWindowListeners(aWindowID);
  if (!listeners) {
    return;
  }
  listeners->RemoveElement(aListener);
  if (listeners->Length() == 0) {
    RemoveWindowID(aWindowID);
    
  }
}

nsresult
MediaManager::Observe(nsISupports* aSubject, const char* aTopic,
  const PRUnichar* aData)
{
  NS_ASSERTION(NS_IsMainThread(), "Observer invoked off the main thread");
  nsCOMPtr<nsIObserverService> obs = mozilla::services::GetObserverService();

  if (!strcmp(aTopic, "xpcom-shutdown")) {
    obs->RemoveObserver(this, "xpcom-shutdown");
    obs->RemoveObserver(this, "getUserMedia:response:allow");
    obs->RemoveObserver(this, "getUserMedia:response:deny");
    obs->RemoveObserver(this, "getUserMedia:revoke");

    
    {
      MutexAutoLock lock(mMutex);
      GetActiveWindows()->Clear();
      mActiveCallbacks.Clear();
      sSingleton = nullptr;
    }

    return NS_OK;
  }

  if (!strcmp(aTopic, "getUserMedia:response:allow")) {
    nsString key(aData);
    nsRefPtr<nsRunnable> runnable;
    if (!mActiveCallbacks.Get(key, getter_AddRefs(runnable))) {
      return NS_OK;
    }
    mActiveCallbacks.Remove(key);

    if (aSubject) {
      
      
      GetUserMediaRunnable* gUMRunnable =
        static_cast<GetUserMediaRunnable*>(runnable.get());

      nsCOMPtr<nsISupportsArray> array(do_QueryInterface(aSubject));
      MOZ_ASSERT(array);
      uint32_t len = 0;
      array->Count(&len);
      MOZ_ASSERT(len);
      if (!len) {
        gUMRunnable->Denied(); 
        return NS_OK;
      }
      for (uint32_t i = 0; i < len; i++) {
        nsCOMPtr<nsISupports> supports;
        array->GetElementAt(i,getter_AddRefs(supports));
        nsCOMPtr<nsIMediaDevice> device(do_QueryInterface(supports));
        MOZ_ASSERT(device); 
        if (device) {
          nsString type;
          device->GetType(type);
          if (type.EqualsLiteral("video")) {
            gUMRunnable->SetVideoDevice(static_cast<MediaDevice*>(device.get()));
          } else if (type.EqualsLiteral("audio")) {
            gUMRunnable->SetAudioDevice(static_cast<MediaDevice*>(device.get()));
          } else {
            NS_WARNING("Unknown device type in getUserMedia");
          }
        }
      }
    }

    
    mMediaThread->Dispatch(runnable, NS_DISPATCH_NORMAL);
    return NS_OK;
  }

  if (!strcmp(aTopic, "getUserMedia:response:deny")) {
    nsString key(aData);
    nsRefPtr<nsRunnable> runnable;
    if (!mActiveCallbacks.Get(key, getter_AddRefs(runnable))) {
      return NS_OK;
    }
    mActiveCallbacks.Remove(key);

    GetUserMediaRunnable* gUMRunnable =
      static_cast<GetUserMediaRunnable*>(runnable.get());
    gUMRunnable->Denied();
    return NS_OK;
  }

  if (!strcmp(aTopic, "getUserMedia:revoke")) {
    nsresult rv;
    uint64_t windowID = nsString(aData).ToInteger64(&rv);
    MOZ_ASSERT(NS_SUCCEEDED(rv));
    if (NS_SUCCEEDED(rv)) {
      LOG(("Revoking MediaCapture access for window %llu",windowID));
      OnNavigation(windowID);
    }

    return NS_OK;
  }

  return NS_OK;
}

static PLDHashOperator
WindowsHashToArrayFunc (const uint64_t& aId,
                        StreamListeners* aData,
                        void *userArg)
{
    nsISupportsArray *array =
        static_cast<nsISupportsArray *>(userArg);
    nsPIDOMWindow *window = static_cast<nsPIDOMWindow*>
      (nsGlobalWindow::GetInnerWindowWithId(aId));
    (void) aData;

    MOZ_ASSERT(window);
    if (window) {
      array->AppendElement(window);
    }
    return PL_DHASH_NEXT;
}


nsresult
MediaManager::GetActiveMediaCaptureWindows(nsISupportsArray **aArray)
{
  MOZ_ASSERT(aArray);
  nsISupportsArray *array;
  nsresult rv = NS_NewISupportsArray(&array); 
  if (NS_FAILED(rv))
    return rv;

  mActiveWindows.EnumerateRead(WindowsHashToArrayFunc, array);

  *aArray = array;
  return NS_OK;
}


void
GetUserMediaCallbackMediaStreamListener::Invalidate()
{

  nsRefPtr<MediaOperationRunnable> runnable;
  
  
  
  
  runnable = new MediaOperationRunnable(MEDIA_STOP,
                                        this, mAudioSource, mVideoSource,
                                        mFinished);
  mMediaThread->Dispatch(runnable, NS_DISPATCH_NORMAL);
}


void
GetUserMediaCallbackMediaStreamListener::NotifyFinished(MediaStreamGraph* aGraph)
{
  mFinished = true;
  Invalidate(); 
  NS_DispatchToMainThread(new GetUserMediaListenerRemove(mWindowID, this));
}




void
GetUserMediaCallbackMediaStreamListener::NotifyRemoved(MediaStreamGraph* aGraph)
{
  {
    MutexAutoLock lock(mLock); 
    MM_LOG(("Listener removed by DOM Destroy(), mFinished = %d", (int) mFinished));
    mRemoved = true;
  }
  if (!mFinished) {
    NotifyFinished(aGraph);
  }
}

} 
