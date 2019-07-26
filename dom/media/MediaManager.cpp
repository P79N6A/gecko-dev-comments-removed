





#include "MediaManager.h"

#include "MediaStreamGraph.h"
#include "GetUserMediaRequest.h"
#include "nsHashPropertyBag.h"
#ifdef MOZ_WIDGET_GONK
#include "nsIAudioManager.h"
#endif
#include "nsIDOMFile.h"
#include "nsIEventTarget.h"
#include "nsIUUIDGenerator.h"
#include "nsIScriptGlobalObject.h"
#include "nsIPermissionManager.h"
#include "nsIPopupWindowManager.h"
#include "nsISupportsArray.h"
#include "nsIDocShell.h"
#include "nsIDocument.h"
#include "nsISupportsPrimitives.h"
#include "nsIInterfaceRequestorUtils.h"
#include "mozilla/Types.h"
#include "mozilla/PeerIdentity.h"
#include "mozilla/dom/ContentChild.h"
#include "mozilla/dom/MediaStreamBinding.h"
#include "mozilla/dom/MediaStreamTrackBinding.h"
#include "mozilla/dom/GetUserMediaRequestBinding.h"
#include "mozilla/Preferences.h"
#include "MediaTrackConstraints.h"

#include "Latency.h"


#include "prprf.h"

#include "nsJSUtils.h"
#include "nsDOMFile.h"
#include "nsGlobalWindow.h"

#include "mozilla/Preferences.h"


#include "MediaEngineDefault.h"
#if defined(MOZ_WEBRTC)
#include "MediaEngineWebRTC.h"
#endif

#ifdef MOZ_B2G
#include "MediaPermissionGonk.h"
#endif



#ifdef GetCurrentTime
#undef GetCurrentTime
#endif


template<>
struct nsIMediaDevice::COMTypeInfo<mozilla::VideoDevice, void> {
  static const nsIID kIID NS_HIDDEN;
};
const nsIID nsIMediaDevice::COMTypeInfo<mozilla::VideoDevice, void>::kIID = NS_IMEDIADEVICE_IID;
template<>
struct nsIMediaDevice::COMTypeInfo<mozilla::AudioDevice, void> {
  static const nsIID kIID NS_HIDDEN;
};
const nsIID nsIMediaDevice::COMTypeInfo<mozilla::AudioDevice, void>::kIID = NS_IMEDIADEVICE_IID;

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

using dom::MediaStreamConstraints;         
using dom::MediaTrackConstraintSet;        
using dom::MediaTrackConstraints;          
using dom::GetUserMediaRequest;
using dom::Sequence;
using dom::OwningBooleanOrMediaTrackConstraints;
using dom::SupportedAudioConstraints;
using dom::SupportedVideoConstraints;

ErrorCallbackRunnable::ErrorCallbackRunnable(
  nsCOMPtr<nsIDOMGetUserMediaSuccessCallback>& aSuccess,
  nsCOMPtr<nsIDOMGetUserMediaErrorCallback>& aError,
  const nsAString& aErrorMsg, uint64_t aWindowID)
  : mErrorMsg(aErrorMsg)
  , mWindowID(aWindowID)
  , mManager(MediaManager::GetInstance())
{
  mSuccess.swap(aSuccess);
  mError.swap(aError);
}

ErrorCallbackRunnable::~ErrorCallbackRunnable()
{
  MOZ_ASSERT(!mSuccess && !mError);
}

NS_IMETHODIMP
ErrorCallbackRunnable::Run()
{
  
  NS_ASSERTION(NS_IsMainThread(), "Only call on main thread");

  nsCOMPtr<nsIDOMGetUserMediaSuccessCallback> success = mSuccess.forget();
  nsCOMPtr<nsIDOMGetUserMediaErrorCallback> error = mError.forget();

  if (!(mManager->IsWindowStillActive(mWindowID))) {
    return NS_OK;
  }
  
  
  error->OnError(mErrorMsg);
  return NS_OK;
}







class SuccessCallbackRunnable : public nsRunnable
{
public:
  SuccessCallbackRunnable(
    nsCOMPtr<nsIDOMGetUserMediaSuccessCallback>& aSuccess,
    nsCOMPtr<nsIDOMGetUserMediaErrorCallback>& aError,
    nsIDOMFile* aFile, uint64_t aWindowID)
    : mFile(aFile)
    , mWindowID(aWindowID)
    , mManager(MediaManager::GetInstance())
  {
    mSuccess.swap(aSuccess);
    mError.swap(aError);
  }

  NS_IMETHOD
  Run()
  {
    
    NS_ASSERTION(NS_IsMainThread(), "Only call on main thread");

    nsCOMPtr<nsIDOMGetUserMediaSuccessCallback> success = mSuccess.forget();
    nsCOMPtr<nsIDOMGetUserMediaErrorCallback> error = mError.forget();

    if (!(mManager->IsWindowStillActive(mWindowID))) {
      return NS_OK;
    }
    
    
    success->OnSuccess(mFile);
    return NS_OK;
  }

private:
  nsCOMPtr<nsIDOMGetUserMediaSuccessCallback> mSuccess;
  nsCOMPtr<nsIDOMGetUserMediaErrorCallback> mError;
  nsCOMPtr<nsIDOMFile> mFile;
  uint64_t mWindowID;
  nsRefPtr<MediaManager> mManager; 
};






class DeviceSuccessCallbackRunnable: public nsRunnable
{
public:
  DeviceSuccessCallbackRunnable(
    uint64_t aWindowID,
    nsCOMPtr<nsIGetUserMediaDevicesSuccessCallback>& aSuccess,
    nsCOMPtr<nsIDOMGetUserMediaErrorCallback>& aError,
    nsTArray<nsCOMPtr<nsIMediaDevice> >* aDevices)
    : mDevices(aDevices)
    , mWindowID(aWindowID)
    , mManager(MediaManager::GetInstance())
  {
    mSuccess.swap(aSuccess);
    mError.swap(aError);
  }

  NS_IMETHOD
  Run()
  {
    NS_ASSERTION(NS_IsMainThread(), "Only call on main thread");

    
    if (!mManager->IsWindowStillActive(mWindowID)) {
      return NS_OK;
    }

    nsCOMPtr<nsIWritableVariant> devices =
      do_CreateInstance("@mozilla.org/variant;1");

    int32_t len = mDevices->Length();
    if (len == 0) {
      
      
      
      
      mError->OnError(NS_LITERAL_STRING("NO_DEVICES_FOUND"));
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

    mSuccess->OnSuccess(devices);
    return NS_OK;
  }

private:
  nsCOMPtr<nsIGetUserMediaDevicesSuccessCallback> mSuccess;
  nsCOMPtr<nsIDOMGetUserMediaErrorCallback> mError;
  nsAutoPtr<nsTArray<nsCOMPtr<nsIMediaDevice> > > mDevices;
  uint64_t mWindowID;
  nsRefPtr<MediaManager> mManager;
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




NS_IMPL_ISUPPORTS(MediaDevice, nsIMediaDevice)

MediaDevice* MediaDevice::Create(MediaEngineVideoSource* source) {
  return new VideoDevice(source);
}

MediaDevice* MediaDevice::Create(MediaEngineAudioSource* source) {
  return new AudioDevice(source);
}

MediaDevice::MediaDevice(MediaEngineSource* aSource)
  : mHasFacingMode(false)
  , mSource(aSource) {
  mSource->GetName(mName);
  mSource->GetUUID(mID);
}

VideoDevice::VideoDevice(MediaEngineVideoSource* aSource)
  : MediaDevice(aSource) {
#ifdef MOZ_B2G_CAMERA
  if (mName.EqualsLiteral("back")) {
    mHasFacingMode = true;
    mFacingMode = dom::VideoFacingModeEnum::Environment;
  } else if (mName.EqualsLiteral("front")) {
    mHasFacingMode = true;
    mFacingMode = dom::VideoFacingModeEnum::User;
  }
#endif 

  
  if (mName.Find(NS_LITERAL_STRING("Face")) != -1) {
    mHasFacingMode = true;
    mFacingMode = dom::VideoFacingModeEnum::User;
  }
}

AudioDevice::AudioDevice(MediaEngineAudioSource* aSource)
  : MediaDevice(aSource) {}

NS_IMETHODIMP
MediaDevice::GetName(nsAString& aName)
{
  aName.Assign(mName);
  return NS_OK;
}

NS_IMETHODIMP
MediaDevice::GetType(nsAString& aType)
{
  return NS_OK;
}

NS_IMETHODIMP
VideoDevice::GetType(nsAString& aType)
{
  aType.AssignLiteral("video");
  return NS_OK;
}

NS_IMETHODIMP
AudioDevice::GetType(nsAString& aType)
{
  aType.AssignLiteral("audio");
  return NS_OK;
}

NS_IMETHODIMP
MediaDevice::GetId(nsAString& aID)
{
  aID.Assign(mID);
  return NS_OK;
}

NS_IMETHODIMP
MediaDevice::GetFacingMode(nsAString& aFacingMode)
{
  if (mHasFacingMode) {
    aFacingMode.Assign(NS_ConvertUTF8toUTF16(
        dom::VideoFacingModeEnumValues::strings[uint32_t(mFacingMode)].value));
  } else {
    aFacingMode.Truncate(0);
  }
  return NS_OK;
}

MediaEngineVideoSource*
VideoDevice::GetSource()
{
  return static_cast<MediaEngineVideoSource*>(&*mSource);
}

MediaEngineAudioSource*
AudioDevice::GetSource()
{
  return static_cast<MediaEngineAudioSource*>(&*mSource);
}





class nsDOMUserMediaStream : public DOMLocalMediaStream
{
public:
  static already_AddRefed<nsDOMUserMediaStream>
  CreateTrackUnionStream(nsIDOMWindow* aWindow,
                         MediaEngineSource *aAudioSource,
                         MediaEngineSource *aVideoSource)
  {
    DOMMediaStream::TrackTypeHints hints =
      (aAudioSource ? DOMMediaStream::HINT_CONTENTS_AUDIO : 0) |
      (aVideoSource ? DOMMediaStream::HINT_CONTENTS_VIDEO : 0);

    nsRefPtr<nsDOMUserMediaStream> stream = new nsDOMUserMediaStream(aAudioSource);
    stream->InitTrackUnionStream(aWindow, hints);
    return stream.forget();
  }

  nsDOMUserMediaStream(MediaEngineSource *aAudioSource) :
    mAudioSource(aAudioSource),
    mEchoOn(true),
    mAgcOn(false),
    mNoiseOn(true),
#ifdef MOZ_WEBRTC
    mEcho(webrtc::kEcDefault),
    mAgc(webrtc::kAgcDefault),
    mNoise(webrtc::kNsDefault),
#else
    mEcho(0),
    mAgc(0),
    mNoise(0),
#endif
    mPlayoutDelay(20)
  {}

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

  virtual void Stop()
  {
    if (mSourceStream) {
      mSourceStream->EndAllTrackAndFinish();
    }
  }

  
  virtual bool AddDirectListener(MediaStreamDirectListener *aListener) MOZ_OVERRIDE
  {
    if (mSourceStream) {
      mSourceStream->AddDirectListener(aListener);
      return true; 
    }
    return false;
  }

  virtual void
  AudioConfig(bool aEchoOn, uint32_t aEcho,
              bool aAgcOn, uint32_t aAgc,
              bool aNoiseOn, uint32_t aNoise,
              int32_t aPlayoutDelay)
  {
    mEchoOn = aEchoOn;
    mEcho = aEcho;
    mAgcOn = aAgcOn;
    mAgc = aAgc;
    mNoiseOn = aNoiseOn;
    mNoise = aNoise;
    mPlayoutDelay = aPlayoutDelay;
  }

  virtual void RemoveDirectListener(MediaStreamDirectListener *aListener) MOZ_OVERRIDE
  {
    if (mSourceStream) {
      mSourceStream->RemoveDirectListener(aListener);
    }
  }

  
  virtual void SetTrackEnabled(TrackID aID, bool aEnabled) MOZ_OVERRIDE
  {
    
    

    
    
    GetStream()->AsProcessedStream()->ForwardTrackEnabled(aID, aEnabled);
  }

  
  
  nsRefPtr<SourceMediaStream> mSourceStream;
  nsRefPtr<MediaInputPort> mPort;
  nsRefPtr<MediaEngineSource> mAudioSource; 
  bool mEchoOn;
  bool mAgcOn;
  bool mNoiseOn;
  uint32_t mEcho;
  uint32_t mAgc;
  uint32_t mNoise;
  uint32_t mPlayoutDelay;
};















class GetUserMediaStreamRunnable : public nsRunnable
{
public:
  GetUserMediaStreamRunnable(
    nsCOMPtr<nsIDOMGetUserMediaSuccessCallback>& aSuccess,
    nsCOMPtr<nsIDOMGetUserMediaErrorCallback>& aError,
    uint64_t aWindowID,
    GetUserMediaCallbackMediaStreamListener* aListener,
    MediaEngineSource* aAudioSource,
    MediaEngineSource* aVideoSource,
    PeerIdentity* aPeerIdentity)
    : mAudioSource(aAudioSource)
    , mVideoSource(aVideoSource)
    , mWindowID(aWindowID)
    , mListener(aListener)
    , mPeerIdentity(aPeerIdentity)
    , mManager(MediaManager::GetInstance())
  {
    mSuccess.swap(aSuccess);
    mError.swap(aError);
  }

  ~GetUserMediaStreamRunnable() {}

  class TracksAvailableCallback : public DOMMediaStream::OnTracksAvailableCallback
  {
  public:
    TracksAvailableCallback(MediaManager* aManager,
                            nsIDOMGetUserMediaSuccessCallback* aSuccess,
                            uint64_t aWindowID,
                            DOMMediaStream* aStream)
      : mWindowID(aWindowID), mSuccess(aSuccess), mManager(aManager),
        mStream(aStream) {}
    virtual void NotifyTracksAvailable(DOMMediaStream* aStream) MOZ_OVERRIDE
    {
      
      if (!(mManager->IsWindowStillActive(mWindowID))) {
        return;
      }

      
      
      aStream->SetLogicalStreamStartTime(aStream->GetStream()->GetCurrentTime());

      
      
      LOG(("Returning success for getUserMedia()"));
      mSuccess->OnSuccess(aStream);
    }
    uint64_t mWindowID;
    nsCOMPtr<nsIDOMGetUserMediaSuccessCallback> mSuccess;
    nsRefPtr<MediaManager> mManager;
    
    
    
    
    
    
    
    
    nsRefPtr<DOMMediaStream> mStream;
  };

  NS_IMETHOD
  Run()
  {
#ifdef MOZ_WEBRTC
    int32_t aec = (int32_t) webrtc::kEcUnchanged;
    int32_t agc = (int32_t) webrtc::kAgcUnchanged;
    int32_t noise = (int32_t) webrtc::kNsUnchanged;
#else
    int32_t aec = 0, agc = 0, noise = 0;
#endif
    bool aec_on = false, agc_on = false, noise_on = false;
    int32_t playout_delay = 0;

    NS_ASSERTION(NS_IsMainThread(), "Only call on main thread");
    nsPIDOMWindow *window = static_cast<nsPIDOMWindow*>
      (nsGlobalWindow::GetInnerWindowWithId(mWindowID));

    
    
    StreamListeners* listeners = mManager->GetWindowListeners(mWindowID);
    if (!listeners || !window || !window->GetExtantDoc()) {
      
      return NS_OK;
    }

#ifdef MOZ_WEBRTC
    
    nsresult rv;
    nsCOMPtr<nsIPrefService> prefs = do_GetService("@mozilla.org/preferences-service;1", &rv);
    if (NS_SUCCEEDED(rv)) {
      nsCOMPtr<nsIPrefBranch> branch = do_QueryInterface(prefs);

      if (branch) {
        branch->GetBoolPref("media.getusermedia.aec_enabled", &aec_on);
        branch->GetIntPref("media.getusermedia.aec", &aec);
        branch->GetBoolPref("media.getusermedia.agc_enabled", &agc_on);
        branch->GetIntPref("media.getusermedia.agc", &agc);
        branch->GetBoolPref("media.getusermedia.noise_enabled", &noise_on);
        branch->GetIntPref("media.getusermedia.noise", &noise);
        branch->GetIntPref("media.getusermedia.playout_delay", &playout_delay);
      }
    }
#endif
    
    nsRefPtr<nsDOMUserMediaStream> trackunion =
      nsDOMUserMediaStream::CreateTrackUnionStream(window, mAudioSource,
                                                   mVideoSource);
    if (!trackunion) {
      nsCOMPtr<nsIDOMGetUserMediaErrorCallback> error = mError.forget();
      LOG(("Returning error for getUserMedia() - no stream"));
      error->OnError(NS_LITERAL_STRING("NO_STREAM"));
      return NS_OK;
    }
    trackunion->AudioConfig(aec_on, (uint32_t) aec,
                            agc_on, (uint32_t) agc,
                            noise_on, (uint32_t) noise,
                            playout_delay);


    MediaStreamGraph* gm = MediaStreamGraph::GetInstance();
    nsRefPtr<SourceMediaStream> stream = gm->CreateSourceStream(nullptr);

    
    trackunion->GetStream()->AsProcessedStream()->SetAutofinish(true);
    nsRefPtr<MediaInputPort> port = trackunion->GetStream()->AsProcessedStream()->
      AllocateInputPort(stream, MediaInputPort::FLAG_BLOCK_OUTPUT);
    trackunion->mSourceStream = stream;
    trackunion->mPort = port.forget();
    
    
    AsyncLatencyLogger::Get(true);
    LogLatency(AsyncLatencyLogger::MediaStreamCreate,
               reinterpret_cast<uint64_t>(stream.get()),
               reinterpret_cast<int64_t>(trackunion->GetStream()));

    nsCOMPtr<nsIPrincipal> principal;
    if (mPeerIdentity) {
      principal = do_CreateInstance("@mozilla.org/nullprincipal;1");
      trackunion->SetPeerIdentity(mPeerIdentity.forget());
    } else {
      principal = window->GetExtantDoc()->NodePrincipal();
    }
    trackunion->CombineWithPrincipal(principal);

    
    
    
    
    mListener->Activate(stream.forget(), mAudioSource, mVideoSource);

    
    TracksAvailableCallback* tracksAvailableCallback =
      new TracksAvailableCallback(mManager, mSuccess, mWindowID, trackunion);

    mListener->AudioConfig(aec_on, (uint32_t) aec,
                           agc_on, (uint32_t) agc,
                           noise_on, (uint32_t) noise,
                           playout_delay);

    
    
    
    
    nsIThread *mediaThread = MediaManager::GetThread();
    nsRefPtr<MediaOperationRunnable> runnable(
      new MediaOperationRunnable(MEDIA_START, mListener, trackunion,
                                 tracksAvailableCallback,
                                 mAudioSource, mVideoSource, false, mWindowID,
                                 mError.forget()));
    mediaThread->Dispatch(runnable, NS_DISPATCH_NORMAL);

    
    mError = nullptr;
    return NS_OK;
  }

private:
  nsCOMPtr<nsIDOMGetUserMediaSuccessCallback> mSuccess;
  nsCOMPtr<nsIDOMGetUserMediaErrorCallback> mError;
  nsRefPtr<MediaEngineSource> mAudioSource;
  nsRefPtr<MediaEngineSource> mVideoSource;
  uint64_t mWindowID;
  nsRefPtr<GetUserMediaCallbackMediaStreamListener> mListener;
  nsAutoPtr<PeerIdentity> mPeerIdentity;
  nsRefPtr<MediaManager> mManager; 
};

static bool
IsOn(const OwningBooleanOrMediaTrackConstraints &aUnion) {
  return !aUnion.IsBoolean() || aUnion.GetAsBoolean();
}

static const MediaTrackConstraints&
GetInvariant(const OwningBooleanOrMediaTrackConstraints &aUnion) {
  static const MediaTrackConstraints empty;
  return aUnion.IsMediaTrackConstraints() ?
      aUnion.GetAsMediaTrackConstraints() : empty;
}








static bool SatisfyConstraintSet(const MediaEngineVideoSource *,
                                 const MediaTrackConstraintSet &aConstraints,
                                 nsIMediaDevice &aCandidate)
{
  if (aConstraints.mFacingMode.WasPassed()) {
    nsString s;
    aCandidate.GetFacingMode(s);
    if (!s.EqualsASCII(dom::VideoFacingModeEnumValues::strings[
        uint32_t(aConstraints.mFacingMode.Value())].value)) {
      return false;
    }
  }
  
  return true;
}

static bool SatisfyConstraintSet(const MediaEngineAudioSource *,
                                 const MediaTrackConstraintSet &aConstraints,
                                 nsIMediaDevice &aCandidate)
{
  
  return true;
}

typedef nsTArray<nsCOMPtr<nsIMediaDevice> > SourceSet;



template<class SourceType, class ConstraintsType>
static SourceSet *
  GetSources(MediaEngine *engine,
             ConstraintsType &aConstraints,
             void (MediaEngine::* aEnumerate)(nsTArray<nsRefPtr<SourceType> >*),
             const char* media_device_name = nullptr)
{
  ScopedDeletePtr<SourceSet> result(new SourceSet);

  const SourceType * const type = nullptr;
  nsString deviceName;
  
  SourceSet candidateSet;
  {
    nsTArray<nsRefPtr<SourceType> > sources;
    (engine->*aEnumerate)(&sources);
    





    for (uint32_t len = sources.Length(), i = 0; i < len; i++) {
      sources[i]->GetName(deviceName);
      if (media_device_name && strlen(media_device_name) > 0)  {
        if (deviceName.EqualsASCII(media_device_name)) {
          candidateSet.AppendElement(MediaDevice::Create(sources[i]));
          break;
        }
      } else {
        candidateSet.AppendElement(MediaDevice::Create(sources[i]));
      }
    }
  }

  

  auto& c = aConstraints;
  if (c.mUnsupportedRequirement) {
    
    
    
    
    
    
    return result.forget();
  }

  

  for (uint32_t i = 0; i < candidateSet.Length();) {
    
    if (!SatisfyConstraintSet(type, c.mRequired, *candidateSet[i])) {
      candidateSet.RemoveElementAt(i);
    } else {
      ++i;
    }
  }

  
  
  
  
  
  if (c.mNonrequired.Length()) {
    if (!c.mAdvanced.WasPassed()) {
      c.mAdvanced.Construct();
    }
    c.mAdvanced.Value().MoveElementsFrom(c.mNonrequired);
  }

  
  
  
  
  
  
  
  
  
  
  
  
  

  SourceSet tailSet;

  if (c.mAdvanced.WasPassed()) {
    auto &array = c.mAdvanced.Value();

    for (int i = 0; i < int(array.Length()); i++) {
      SourceSet rejects;
      for (uint32_t j = 0; j < candidateSet.Length();) {
        if (!SatisfyConstraintSet(type, array[i], *candidateSet[j])) {
          rejects.AppendElement(candidateSet[j]);
          candidateSet.RemoveElementAt(j);
        } else {
          ++j;
        }
      }
      (candidateSet.Length()? tailSet : candidateSet).MoveElementsFrom(rejects);
    }
  }

  

  result->MoveElementsFrom(candidateSet);
  result->MoveElementsFrom(tailSet);
  return result.forget();
}










class GetUserMediaRunnable : public nsRunnable
{
public:
  GetUserMediaRunnable(
    const MediaStreamConstraints& aConstraints,
    already_AddRefed<nsIDOMGetUserMediaSuccessCallback> aSuccess,
    already_AddRefed<nsIDOMGetUserMediaErrorCallback> aError,
    uint64_t aWindowID, GetUserMediaCallbackMediaStreamListener *aListener,
    MediaEnginePrefs &aPrefs)
    : mConstraints(aConstraints)
    , mSuccess(aSuccess)
    , mError(aError)
    , mWindowID(aWindowID)
    , mListener(aListener)
    , mPrefs(aPrefs)
    , mDeviceChosen(false)
    , mBackend(nullptr)
    , mManager(MediaManager::GetInstance())
  {}

  



  GetUserMediaRunnable(
    const MediaStreamConstraints& aConstraints,
    already_AddRefed<nsIDOMGetUserMediaSuccessCallback> aSuccess,
    already_AddRefed<nsIDOMGetUserMediaErrorCallback> aError,
    uint64_t aWindowID, GetUserMediaCallbackMediaStreamListener *aListener,
    MediaEnginePrefs &aPrefs,
    MediaEngine* aBackend)
    : mConstraints(aConstraints)
    , mSuccess(aSuccess)
    , mError(aError)
    , mWindowID(aWindowID)
    , mListener(aListener)
    , mPrefs(aPrefs)
    , mDeviceChosen(false)
    , mBackend(aBackend)
    , mManager(MediaManager::GetInstance())
  {}

  ~GetUserMediaRunnable() {
  }

  void
  Fail(const nsAString& aMessage) {
    nsRefPtr<ErrorCallbackRunnable> runnable =
      new ErrorCallbackRunnable(mSuccess, mError, aMessage, mWindowID);
    
    MOZ_ASSERT(!mSuccess);
    MOZ_ASSERT(!mError);

    NS_DispatchToMainThread(runnable);
  }

  NS_IMETHOD
  Run()
  {
    NS_ASSERTION(!NS_IsMainThread(), "Don't call on main thread");
    MOZ_ASSERT(mSuccess);
    MOZ_ASSERT(mError);

    MediaEngine* backend = mBackend;
    
    if (!backend) {
      backend = mManager->GetBackend(mWindowID);
    }

    
    if (!mDeviceChosen) {
      nsresult rv = SelectDevice(backend);
      if (rv != NS_OK) {
        return rv;
      }
    }

    
    if (mConstraints.mPicture &&
        (IsOn(mConstraints.mAudio) || IsOn(mConstraints.mVideo))) {
      Fail(NS_LITERAL_STRING("NOT_SUPPORTED_ERR"));
      return NS_OK;
    }

    if (mConstraints.mPicture) {
      ProcessGetUserMediaSnapshot(mVideoDevice->GetSource(), 0);
      return NS_OK;
    }

    
    ProcessGetUserMedia(((IsOn(mConstraints.mAudio) && mAudioDevice) ?
                         mAudioDevice->GetSource() : nullptr),
                        ((IsOn(mConstraints.mVideo) && mVideoDevice) ?
                         mVideoDevice->GetSource() : nullptr));
    return NS_OK;
  }

  nsresult
  Denied(const nsAString& aErrorMsg)
  {
    MOZ_ASSERT(mSuccess);
    MOZ_ASSERT(mError);

    
    
    if (NS_IsMainThread()) {
      
      
      nsCOMPtr<nsIDOMGetUserMediaSuccessCallback> success = mSuccess.forget();
      nsCOMPtr<nsIDOMGetUserMediaErrorCallback> error = mError.forget();
      error->OnError(aErrorMsg);

      
      nsRefPtr<MediaManager> manager(MediaManager::GetInstance());
      manager->RemoveFromWindowList(mWindowID, mListener);
    } else {
      
      
      Fail(aErrorMsg);

      
      NS_DispatchToMainThread(new GetUserMediaListenerRemove(mWindowID, mListener));
    }

    MOZ_ASSERT(!mSuccess);
    MOZ_ASSERT(!mError);

    return NS_OK;
  }

  nsresult
  SetContraints(const MediaStreamConstraints& aConstraints)
  {
    mConstraints = aConstraints;
    return NS_OK;
  }

  nsresult
  SetAudioDevice(AudioDevice* aAudioDevice)
  {
    mAudioDevice = aAudioDevice;
    mDeviceChosen = true;
    return NS_OK;
  }

  nsresult
  SetVideoDevice(VideoDevice* aVideoDevice)
  {
    mVideoDevice = aVideoDevice;
    mDeviceChosen = true;
    return NS_OK;
  }

  nsresult
  SelectDevice(MediaEngine* backend)
  {
    MOZ_ASSERT(mSuccess);
    MOZ_ASSERT(mError);
    if (mConstraints.mPicture || IsOn(mConstraints.mVideo)) {
      VideoTrackConstraintsN constraints(GetInvariant(mConstraints.mVideo));
      ScopedDeletePtr<SourceSet> sources (GetSources(backend, constraints,
          &MediaEngine::EnumerateVideoDevices));

      if (!sources->Length()) {
        Fail(NS_LITERAL_STRING("NO_DEVICES_FOUND"));
        return NS_ERROR_FAILURE;
      }
      
      mVideoDevice = do_QueryObject((*sources)[0]);
      LOG(("Selected video device"));
    }

    if (IsOn(mConstraints.mAudio)) {
      AudioTrackConstraintsN constraints(GetInvariant(mConstraints.mAudio));
      ScopedDeletePtr<SourceSet> sources (GetSources(backend, constraints,
          &MediaEngine::EnumerateAudioDevices));

      if (!sources->Length()) {
        Fail(NS_LITERAL_STRING("NO_DEVICES_FOUND"));
        return NS_ERROR_FAILURE;
      }
      
      mAudioDevice = do_QueryObject((*sources)[0]);
      LOG(("Selected audio device"));
    }

    return NS_OK;
  }

  



  void
  ProcessGetUserMedia(MediaEngineAudioSource* aAudioSource,
                      MediaEngineVideoSource* aVideoSource)
  {
    MOZ_ASSERT(mSuccess);
    MOZ_ASSERT(mError);
    nsresult rv;
    if (aAudioSource) {
      rv = aAudioSource->Allocate(GetInvariant(mConstraints.mAudio), mPrefs);
      if (NS_FAILED(rv)) {
        LOG(("Failed to allocate audiosource %d",rv));
        Fail(NS_LITERAL_STRING("HARDWARE_UNAVAILABLE"));
        return;
      }
    }
    if (aVideoSource) {
      rv = aVideoSource->Allocate(GetInvariant(mConstraints.mVideo), mPrefs);
      if (NS_FAILED(rv)) {
        LOG(("Failed to allocate videosource %d\n",rv));
        if (aAudioSource) {
          aAudioSource->Deallocate();
        }
        Fail(NS_LITERAL_STRING("HARDWARE_UNAVAILABLE"));
        return;
      }
    }
    PeerIdentity* peerIdentity = nullptr;
    if (!mConstraints.mPeerIdentity.IsEmpty()) {
      peerIdentity = new PeerIdentity(mConstraints.mPeerIdentity);
    }

    NS_DispatchToMainThread(new GetUserMediaStreamRunnable(
      mSuccess, mError, mWindowID, mListener, aAudioSource, aVideoSource,
      peerIdentity
    ));

    MOZ_ASSERT(!mSuccess);
    MOZ_ASSERT(!mError);

    return;
  }

  



  void
  ProcessGetUserMediaSnapshot(MediaEngineVideoSource* aSource, int aDuration)
  {
    MOZ_ASSERT(mSuccess);
    MOZ_ASSERT(mError);
    nsresult rv = aSource->Allocate(GetInvariant(mConstraints.mVideo), mPrefs);
    if (NS_FAILED(rv)) {
      Fail(NS_LITERAL_STRING("HARDWARE_UNAVAILABLE"));
      return;
    }

    


    nsCOMPtr<nsIDOMFile> file;
    aSource->Snapshot(aDuration, getter_AddRefs(file));
    aSource->Deallocate();

    NS_DispatchToMainThread(new SuccessCallbackRunnable(
      mSuccess, mError, file, mWindowID
    ));

    MOZ_ASSERT(!mSuccess);
    MOZ_ASSERT(!mError);

    return;
  }

private:
  MediaStreamConstraints mConstraints;

  nsCOMPtr<nsIDOMGetUserMediaSuccessCallback> mSuccess;
  nsCOMPtr<nsIDOMGetUserMediaErrorCallback> mError;
  uint64_t mWindowID;
  nsRefPtr<GetUserMediaCallbackMediaStreamListener> mListener;
  nsRefPtr<AudioDevice> mAudioDevice;
  nsRefPtr<VideoDevice> mVideoDevice;
  MediaEnginePrefs mPrefs;

  bool mDeviceChosen;

  RefPtr<MediaEngine> mBackend;
  nsRefPtr<MediaManager> mManager; 
};







class GetUserMediaDevicesRunnable : public nsRunnable
{
public:
  GetUserMediaDevicesRunnable(
    const MediaStreamConstraints& aConstraints,
    already_AddRefed<nsIGetUserMediaDevicesSuccessCallback> aSuccess,
    already_AddRefed<nsIDOMGetUserMediaErrorCallback> aError,
    uint64_t aWindowId, nsACString& aAudioLoopbackDev,
    nsACString& aVideoLoopbackDev)
    : mConstraints(aConstraints)
    , mSuccess(aSuccess)
    , mError(aError)
    , mManager(MediaManager::GetInstance())
    , mWindowId(aWindowId)
    , mLoopbackAudioDevice(aAudioLoopbackDev)
    , mLoopbackVideoDevice(aVideoLoopbackDev) {}

  NS_IMETHOD
  Run()
  {
    NS_ASSERTION(!NS_IsMainThread(), "Don't call on main thread");

    nsRefPtr<MediaEngine> backend;
    if (mConstraints.mFake)
      backend = new MediaEngineDefault();
    else
      backend = mManager->GetBackend(mWindowId);

    ScopedDeletePtr<SourceSet> final(new SourceSet);
    if (IsOn(mConstraints.mVideo)) {
      VideoTrackConstraintsN constraints(GetInvariant(mConstraints.mVideo));
      ScopedDeletePtr<SourceSet> s(GetSources(backend, constraints,
          &MediaEngine::EnumerateVideoDevices,
          mLoopbackVideoDevice.get()));
      final->MoveElementsFrom(*s);
    }
    if (IsOn(mConstraints.mAudio)) {
      AudioTrackConstraintsN constraints(GetInvariant(mConstraints.mAudio));
      ScopedDeletePtr<SourceSet> s (GetSources(backend, constraints,
          &MediaEngine::EnumerateAudioDevices,
          mLoopbackAudioDevice.get()));
      final->MoveElementsFrom(*s);
    }
    NS_DispatchToMainThread(new DeviceSuccessCallbackRunnable(mWindowId,
                                                              mSuccess, mError,
                                                              final.forget()));
    
    MOZ_ASSERT(!mSuccess && !mError);
    return NS_OK;
  }

private:
  MediaStreamConstraints mConstraints;
  nsCOMPtr<nsIGetUserMediaDevicesSuccessCallback> mSuccess;
  nsCOMPtr<nsIDOMGetUserMediaErrorCallback> mError;
  nsRefPtr<MediaManager> mManager;
  uint64_t mWindowId;
  const nsString mCallId;
  
  
  
  nsCString mLoopbackAudioDevice;
  nsCString mLoopbackVideoDevice;
};

MediaManager::MediaManager()
  : mMediaThread(nullptr)
  , mMutex("mozilla::MediaManager")
  , mBackend(nullptr) {
  mPrefs.mWidth  = 0; 
  mPrefs.mHeight = 0; 
  mPrefs.mFPS    = MediaEngine::DEFAULT_VIDEO_FPS;
  mPrefs.mMinFPS = MediaEngine::DEFAULT_VIDEO_MIN_FPS;

  nsresult rv;
  nsCOMPtr<nsIPrefService> prefs = do_GetService("@mozilla.org/preferences-service;1", &rv);
  if (NS_SUCCEEDED(rv)) {
    nsCOMPtr<nsIPrefBranch> branch = do_QueryInterface(prefs);
    if (branch) {
      GetPrefs(branch, nullptr);
    }
  }
  LOG(("%s: default prefs: %dx%d @%dfps (min %d)", __FUNCTION__,
       mPrefs.mWidth, mPrefs.mHeight, mPrefs.mFPS, mPrefs.mMinFPS));
}

NS_IMPL_ISUPPORTS(MediaManager, nsIMediaManagerService, nsIObserver)

 StaticRefPtr<MediaManager> MediaManager::sSingleton;




  MediaManager*
MediaManager::Get() {
  if (!sSingleton) {
    sSingleton = new MediaManager();

    NS_NewNamedThread("MediaManager", getter_AddRefs(sSingleton->mMediaThread));
    LOG(("New Media thread for gum"));

    NS_ASSERTION(NS_IsMainThread(), "Only create MediaManager on main thread");
    nsCOMPtr<nsIObserverService> obs = services::GetObserverService();
    if (obs) {
      obs->AddObserver(sSingleton, "xpcom-shutdown", false);
      obs->AddObserver(sSingleton, "getUserMedia:response:allow", false);
      obs->AddObserver(sSingleton, "getUserMedia:response:deny", false);
      obs->AddObserver(sSingleton, "getUserMedia:revoke", false);
      obs->AddObserver(sSingleton, "phone-state-changed", false);
    }
    
    nsCOMPtr<nsIPrefBranch> prefs = do_GetService(NS_PREFSERVICE_CONTRACTID);
    if (prefs) {
      prefs->AddObserver("media.navigator.video.default_width", sSingleton, false);
      prefs->AddObserver("media.navigator.video.default_height", sSingleton, false);
      prefs->AddObserver("media.navigator.video.default_fps", sSingleton, false);
      prefs->AddObserver("media.navigator.video.default_minfps", sSingleton, false);
    }
  }
  return sSingleton;
}

 already_AddRefed<MediaManager>
MediaManager::GetInstance()
{
  
  nsRefPtr<MediaManager> service = MediaManager::Get();
  return service.forget();
}

 nsresult
MediaManager::NotifyRecordingStatusChange(nsPIDOMWindow* aWindow,
                                          const nsString& aMsg,
                                          const bool& aIsAudio,
                                          const bool& aIsVideo)
{
  NS_ENSURE_ARG(aWindow);

  nsCOMPtr<nsIObserverService> obs = services::GetObserverService();
  if (!obs) {
    NS_WARNING("Could not get the Observer service for GetUserMedia recording notification.");
    return NS_ERROR_FAILURE;
  }

  nsRefPtr<nsHashPropertyBag> props = new nsHashPropertyBag();
  props->SetPropertyAsBool(NS_LITERAL_STRING("isAudio"), aIsAudio);
  props->SetPropertyAsBool(NS_LITERAL_STRING("isVideo"), aIsVideo);

  bool isApp = false;
  nsString requestURL;

  if (nsCOMPtr<nsIDocShell> docShell = aWindow->GetDocShell()) {
    nsresult rv = docShell->GetIsApp(&isApp);
    NS_ENSURE_SUCCESS(rv, rv);

    if (isApp) {
      rv = docShell->GetAppManifestURL(requestURL);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  if (!isApp) {
    nsCString pageURL;
    nsCOMPtr<nsIURI> docURI = aWindow->GetDocumentURI();
    NS_ENSURE_TRUE(docURI, NS_ERROR_FAILURE);

    nsresult rv = docURI->GetSpec(pageURL);
    NS_ENSURE_SUCCESS(rv, rv);

    requestURL = NS_ConvertUTF8toUTF16(pageURL);
  }

  props->SetPropertyAsBool(NS_LITERAL_STRING("isApp"), isApp);
  props->SetPropertyAsAString(NS_LITERAL_STRING("requestURL"), requestURL);

  obs->NotifyObservers(static_cast<nsIPropertyBag2*>(props),
                       "recording-device-events",
                       aMsg.get());

  
  
  if (XRE_GetProcessType() != GeckoProcessType_Default) {
    unused <<
      dom::ContentChild::GetSingleton()->SendRecordingDeviceEvents(aMsg,
                                                                   requestURL,
                                                                   aIsAudio,
                                                                   aIsVideo);
  }

  return NS_OK;
}






nsresult
MediaManager::GetUserMedia(bool aPrivileged,
  nsPIDOMWindow* aWindow, const MediaStreamConstraints& aConstraints,
  nsIDOMGetUserMediaSuccessCallback* aOnSuccess,
  nsIDOMGetUserMediaErrorCallback* aOnError)
{
  NS_ASSERTION(NS_IsMainThread(), "Only call on main thread");

  NS_ENSURE_TRUE(aWindow, NS_ERROR_NULL_POINTER);
  NS_ENSURE_TRUE(aOnError, NS_ERROR_NULL_POINTER);
  NS_ENSURE_TRUE(aOnSuccess, NS_ERROR_NULL_POINTER);

  nsCOMPtr<nsIDOMGetUserMediaSuccessCallback> onSuccess(aOnSuccess);
  nsCOMPtr<nsIDOMGetUserMediaErrorCallback> onError(aOnError);

  MediaStreamConstraints c(aConstraints); 

  






#if !defined(MOZ_WEBRTC)
  if (c.mPicture && !aPrivileged) {
    if (aWindow->GetPopupControlState() > openControlled) {
      nsCOMPtr<nsIPopupWindowManager> pm =
        do_GetService(NS_POPUPWINDOWMANAGER_CONTRACTID);
      if (!pm) {
        return NS_OK;
      }
      uint32_t permission;
      nsCOMPtr<nsIDocument> doc = aWindow->GetExtantDoc();
      pm->TestPermission(doc->NodePrincipal(), &permission);
      if (permission == nsIPopupWindowManager::DENY_POPUP) {
        nsGlobalWindow::FirePopupBlockedEvent(
          doc, aWindow, nullptr, EmptyString(), EmptyString()
        );
        return NS_OK;
      }
    }
  }
#endif

  static bool created = false;
  if (!created) {
    
    
    (void) MediaManager::Get();
#ifdef MOZ_B2G
    
    (void) MediaPermissionManager::GetInstance();
#endif 
  }

  
  
  uint64_t windowID = aWindow->WindowID();
  
  
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
  if (!Preferences::GetBool("media.navigator.video.enabled", true)) {
    c.mVideo.SetAsBoolean() = false;
  }

#if defined(ANDROID) || defined(MOZ_WIDGET_GONK)
  
  if (c.mVideo.IsMediaTrackConstraints()) {
    auto& tc = c.mVideo.GetAsMediaTrackConstraints();
    if (!tc.mRequire.WasPassed() &&
        tc.mMandatory.mFacingMode.WasPassed() && !tc.mFacingMode.WasPassed()) {
      tc.mFacingMode.Construct(tc.mMandatory.mFacingMode.Value());
      tc.mRequire.Construct().AppendElement(NS_LITERAL_STRING("facingMode"));
    }
    if (tc.mOptional.WasPassed() && !tc.mAdvanced.WasPassed()) {
      tc.mAdvanced.Construct();
      for (uint32_t i = 0; i < tc.mOptional.Value().Length(); i++) {
        if (tc.mOptional.Value()[i].mFacingMode.WasPassed()) {
          MediaTrackConstraintSet n;
          n.mFacingMode.Construct(tc.mOptional.Value()[i].mFacingMode.Value());
          tc.mAdvanced.Value().AppendElement(n);
        }
      }
    }
  }
#endif

  
  nsRefPtr<GetUserMediaRunnable> runnable;
  if (c.mFake) {
    
    runnable = new GetUserMediaRunnable(c, onSuccess.forget(),
      onError.forget(), windowID, listener, mPrefs, new MediaEngineDefault());
  } else {
    
    runnable = new GetUserMediaRunnable(c, onSuccess.forget(),
      onError.forget(), windowID, listener, mPrefs);
  }

#ifdef MOZ_B2G_CAMERA
  if (mCameraManager == nullptr) {
    mCameraManager = nsDOMCameraManager::CreateInstance(aWindow);
  }
#endif

#if defined(ANDROID) && !defined(MOZ_WIDGET_GONK)
  if (c.mPicture) {
    
    NS_DispatchToMainThread(runnable);
    return NS_OK;
  }
#endif
  
  if (aPrivileged ||
      (c.mFake && !Preferences::GetBool("media.navigator.permission.fake"))) {
    mMediaThread->Dispatch(runnable, NS_DISPATCH_NORMAL);
  } else {
    bool isHTTPS = false;
    nsIURI* docURI = aWindow->GetDocumentURI();
    if (docURI) {
      docURI->SchemeIs("https", &isHTTPS);
    }

    
    nsresult rv;
    nsCOMPtr<nsIPermissionManager> permManager =
      do_GetService(NS_PERMISSIONMANAGER_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    uint32_t audioPerm = nsIPermissionManager::UNKNOWN_ACTION;
    if (IsOn(c.mAudio)) {
      rv = permManager->TestExactPermissionFromPrincipal(
        aWindow->GetExtantDoc()->NodePrincipal(), "microphone", &audioPerm);
      NS_ENSURE_SUCCESS(rv, rv);
      if (audioPerm == nsIPermissionManager::PROMPT_ACTION) {
        audioPerm = nsIPermissionManager::UNKNOWN_ACTION;
      }
      if (audioPerm == nsIPermissionManager::ALLOW_ACTION) {
        if (!isHTTPS) {
          audioPerm = nsIPermissionManager::UNKNOWN_ACTION;
        }
      }
    }

    uint32_t videoPerm = nsIPermissionManager::UNKNOWN_ACTION;
    if (IsOn(c.mVideo)) {
      rv = permManager->TestExactPermissionFromPrincipal(
        aWindow->GetExtantDoc()->NodePrincipal(), "camera", &videoPerm);
      NS_ENSURE_SUCCESS(rv, rv);
      if (videoPerm == nsIPermissionManager::PROMPT_ACTION) {
        videoPerm = nsIPermissionManager::UNKNOWN_ACTION;
      }
      if (videoPerm == nsIPermissionManager::ALLOW_ACTION) {
        if (!isHTTPS) {
          videoPerm = nsIPermissionManager::UNKNOWN_ACTION;
        }
      }
    }

    if ((!IsOn(c.mAudio) || audioPerm != nsIPermissionManager::UNKNOWN_ACTION) &&
        (!IsOn(c.mVideo) || videoPerm != nsIPermissionManager::UNKNOWN_ACTION)) {
      
      if (IsOn(c.mAudio) && audioPerm == nsIPermissionManager::DENY_ACTION) {
        c.mAudio.SetAsBoolean() = false;
        runnable->SetContraints(c);
      }
      if (IsOn(c.mVideo) && videoPerm == nsIPermissionManager::DENY_ACTION) {
        c.mVideo.SetAsBoolean() = false;
        runnable->SetContraints(c);
      }

      if (!IsOn(c.mAudio) && !IsOn(c.mVideo)) {
        return runnable->Denied(NS_LITERAL_STRING("PERMISSION_DENIED"));
      }

      return mMediaThread->Dispatch(runnable, NS_DISPATCH_NORMAL);
    }

    
    
    
    nsCOMPtr<nsIUUIDGenerator> uuidgen =
      do_GetService("@mozilla.org/uuid-generator;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    
    nsID id;
    rv = uuidgen->GenerateUUIDInPlace(&id);
    NS_ENSURE_SUCCESS(rv, rv);

    char buffer[NSID_LENGTH];
    id.ToProvidedString(buffer);
    NS_ConvertUTF8toUTF16 callID(buffer);

    
    mActiveCallbacks.Put(callID, runnable);

    
    nsTArray<nsString>* array;
    if (!mCallIds.Get(windowID, &array)) {
      array = new nsTArray<nsString>();
      array->AppendElement(callID);
      mCallIds.Put(windowID, array);
    } else {
      array->AppendElement(callID);
    }
    nsCOMPtr<nsIObserverService> obs = services::GetObserverService();
    nsRefPtr<GetUserMediaRequest> req = new GetUserMediaRequest(aWindow,
                                                                callID, c, isHTTPS);
    obs->NotifyObservers(req, "getUserMedia:request", nullptr);
  }

  return NS_OK;
}

nsresult
MediaManager::GetUserMediaDevices(nsPIDOMWindow* aWindow,
  const MediaStreamConstraints& aConstraints,
  nsIGetUserMediaDevicesSuccessCallback* aOnSuccess,
  nsIDOMGetUserMediaErrorCallback* aOnError,
  uint64_t aInnerWindowID)
{
  NS_ASSERTION(NS_IsMainThread(), "Only call on main thread");

  NS_ENSURE_TRUE(aOnError, NS_ERROR_NULL_POINTER);
  NS_ENSURE_TRUE(aOnSuccess, NS_ERROR_NULL_POINTER);

  nsCOMPtr<nsIGetUserMediaDevicesSuccessCallback> onSuccess(aOnSuccess);
  nsCOMPtr<nsIDOMGetUserMediaErrorCallback> onError(aOnError);

  
  nsAdoptingCString loopbackAudioDevice =
    Preferences::GetCString("media.audio_loopback_dev");
  nsAdoptingCString loopbackVideoDevice =
    Preferences::GetCString("media.video_loopback_dev");

  nsCOMPtr<nsIRunnable> gUMDRunnable = new GetUserMediaDevicesRunnable(
    aConstraints, onSuccess.forget(), onError.forget(),
    (aInnerWindowID ? aInnerWindowID : aWindow->WindowID()),
    loopbackAudioDevice, loopbackVideoDevice);

  mMediaThread->Dispatch(gUMDRunnable, NS_DISPATCH_NORMAL);
  return NS_OK;
}

MediaEngine*
MediaManager::GetBackend(uint64_t aWindowId)
{
  
  
  
  MutexAutoLock lock(mMutex);
  if (!mBackend) {
#if defined(MOZ_WEBRTC)
    mBackend = new MediaEngineWebRTC(mPrefs);
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

  
  

  nsTArray<nsString>* callIds;
  if (mCallIds.Get(aWindowID, &callIds)) {
    for (int i = 0, len = callIds->Length(); i < len; ++i) {
      mActiveCallbacks.Remove((*callIds)[i]);
    }
    mCallIds.Remove(aWindowID);
  }

  
  
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
    

    
    nsPIDOMWindow *window = static_cast<nsPIDOMWindow*>
      (nsGlobalWindow::GetInnerWindowWithId(aWindowID));
    if (window) {
      nsPIDOMWindow *outer = window->GetOuterWindow();
      if (outer) {
        uint64_t outerID = outer->WindowID();

        
        char windowBuffer[32];
        PR_snprintf(windowBuffer, sizeof(windowBuffer), "%llu", outerID);
        nsString data = NS_ConvertUTF8toUTF16(windowBuffer);

        nsCOMPtr<nsIObserverService> obs = services::GetObserverService();
        obs->NotifyObservers(nullptr, "recording-window-ended", data.get());
        LOG(("Sent recording-window-ended for window %llu (outer %llu)",
             aWindowID, outerID));
      } else {
        LOG(("No outer window for inner %llu", aWindowID));
      }
    } else {
      LOG(("No inner window for %llu", aWindowID));
    }
  }
}

void
MediaManager::GetPref(nsIPrefBranch *aBranch, const char *aPref,
                      const char *aData, int32_t *aVal)
{
  int32_t temp;
  if (aData == nullptr || strcmp(aPref,aData) == 0) {
    if (NS_SUCCEEDED(aBranch->GetIntPref(aPref, &temp))) {
      *aVal = temp;
    }
  }
}

void
MediaManager::GetPrefBool(nsIPrefBranch *aBranch, const char *aPref,
                          const char *aData, bool *aVal)
{
  bool temp;
  if (aData == nullptr || strcmp(aPref,aData) == 0) {
    if (NS_SUCCEEDED(aBranch->GetBoolPref(aPref, &temp))) {
      *aVal = temp;
    }
  }
}

void
MediaManager::GetPrefs(nsIPrefBranch *aBranch, const char *aData)
{
  GetPref(aBranch, "media.navigator.video.default_width", aData, &mPrefs.mWidth);
  GetPref(aBranch, "media.navigator.video.default_height", aData, &mPrefs.mHeight);
  GetPref(aBranch, "media.navigator.video.default_fps", aData, &mPrefs.mFPS);
  GetPref(aBranch, "media.navigator.video.default_minfps", aData, &mPrefs.mMinFPS);
}

nsresult
MediaManager::Observe(nsISupports* aSubject, const char* aTopic,
  const char16_t* aData)
{
  NS_ASSERTION(NS_IsMainThread(), "Observer invoked off the main thread");
  nsCOMPtr<nsIObserverService> obs = services::GetObserverService();

  if (!strcmp(aTopic, NS_PREFBRANCH_PREFCHANGE_TOPIC_ID)) {
    nsCOMPtr<nsIPrefBranch> branch( do_QueryInterface(aSubject) );
    if (branch) {
      GetPrefs(branch,NS_ConvertUTF16toUTF8(aData).get());
      LOG(("%s: %dx%d @%dfps (min %d)", __FUNCTION__,
           mPrefs.mWidth, mPrefs.mHeight, mPrefs.mFPS, mPrefs.mMinFPS));
    }
  } else if (!strcmp(aTopic, "xpcom-shutdown")) {
    obs->RemoveObserver(this, "xpcom-shutdown");
    obs->RemoveObserver(this, "getUserMedia:response:allow");
    obs->RemoveObserver(this, "getUserMedia:response:deny");
    obs->RemoveObserver(this, "getUserMedia:revoke");

    nsCOMPtr<nsIPrefBranch> prefs = do_GetService(NS_PREFSERVICE_CONTRACTID);
    if (prefs) {
      prefs->RemoveObserver("media.navigator.video.default_width", this);
      prefs->RemoveObserver("media.navigator.video.default_height", this);
      prefs->RemoveObserver("media.navigator.video.default_fps", this);
      prefs->RemoveObserver("media.navigator.video.default_minfps", this);
    }

    
    {
      MutexAutoLock lock(mMutex);
      GetActiveWindows()->Clear();
      mActiveCallbacks.Clear();
      mCallIds.Clear();
      LOG(("Releasing MediaManager singleton and thread"));
      sSingleton = nullptr;
    }

    return NS_OK;

  } else if (!strcmp(aTopic, "getUserMedia:response:allow")) {
    nsString key(aData);
    nsRefPtr<GetUserMediaRunnable> runnable;
    if (!mActiveCallbacks.Get(key, getter_AddRefs(runnable))) {
      return NS_OK;
    }
    mActiveCallbacks.Remove(key);

    if (aSubject) {
      
      
      nsCOMPtr<nsISupportsArray> array(do_QueryInterface(aSubject));
      MOZ_ASSERT(array);
      uint32_t len = 0;
      array->Count(&len);
      MOZ_ASSERT(len);
      if (!len) {
        
        runnable->Denied(NS_LITERAL_STRING("PERMISSION_DENIED"));
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
            runnable->SetVideoDevice(static_cast<VideoDevice*>(device.get()));
          } else if (type.EqualsLiteral("audio")) {
            runnable->SetAudioDevice(static_cast<AudioDevice*>(device.get()));
          } else {
            NS_WARNING("Unknown device type in getUserMedia");
          }
        }
      }
    }

    
    mMediaThread->Dispatch(runnable, NS_DISPATCH_NORMAL);
    return NS_OK;

  } else if (!strcmp(aTopic, "getUserMedia:response:deny")) {
    nsString errorMessage(NS_LITERAL_STRING("PERMISSION_DENIED"));

    if (aSubject) {
      nsCOMPtr<nsISupportsString> msg(do_QueryInterface(aSubject));
      MOZ_ASSERT(msg);
      msg->GetData(errorMessage);
      if (errorMessage.IsEmpty())
        errorMessage.AssignLiteral("UNKNOWN_ERROR");
    }

    nsString key(aData);
    nsRefPtr<GetUserMediaRunnable> runnable;
    if (!mActiveCallbacks.Get(key, getter_AddRefs(runnable))) {
      return NS_OK;
    }
    mActiveCallbacks.Remove(key);
    runnable->Denied(errorMessage);
    return NS_OK;

  } else if (!strcmp(aTopic, "getUserMedia:revoke")) {
    nsresult rv;
    uint64_t windowID = nsString(aData).ToInteger64(&rv);
    MOZ_ASSERT(NS_SUCCEEDED(rv));
    if (NS_SUCCEEDED(rv)) {
      LOG(("Revoking MediaCapture access for window %llu",windowID));
      OnNavigation(windowID);
    }

    return NS_OK;
  }
#ifdef MOZ_WIDGET_GONK
  else if (!strcmp(aTopic, "phone-state-changed")) {
    nsString state(aData);
    nsresult rv;
    uint32_t phoneState = state.ToInteger(&rv);

    if (NS_SUCCEEDED(rv) && phoneState == nsIAudioManager::PHONE_STATE_IN_CALL) {
      StopMediaStreams();
    }
    return NS_OK;
  }
#endif

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

  MOZ_ASSERT(window);
  if (window) {
    
    
    
    bool capturing = false;
    if (aData) {
      uint32_t length = aData->Length();
      for (uint32_t i = 0; i < length; ++i) {
        nsRefPtr<GetUserMediaCallbackMediaStreamListener> listener =
          aData->ElementAt(i);
        if (listener->CapturingVideo() || listener->CapturingAudio()) {
          capturing = true;
          break;
        }
      }
    }

    if (capturing)
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

NS_IMETHODIMP
MediaManager::MediaCaptureWindowState(nsIDOMWindow* aWindow, bool* aVideo,
                                      bool* aAudio)
{
  NS_ASSERTION(NS_IsMainThread(), "Only call on main thread");
  *aVideo = false;
  *aAudio = false;

  nsresult rv = MediaCaptureWindowStateInternal(aWindow, aVideo, aAudio);
#ifdef DEBUG
  nsCOMPtr<nsPIDOMWindow> piWin = do_QueryInterface(aWindow);
  LOG(("%s: window %lld capturing %s %s", __FUNCTION__, piWin ? piWin->WindowID() : -1,
       *aVideo ? "video" : "", *aAudio ? "audio" : ""));
#endif
  return rv;
}

nsresult
MediaManager::MediaCaptureWindowStateInternal(nsIDOMWindow* aWindow, bool* aVideo,
                                              bool* aAudio)
{
  
  

  
  
  
  nsCOMPtr<nsPIDOMWindow> piWin = do_QueryInterface(aWindow);
  if (piWin) {
    if (piWin->GetCurrentInnerWindow() || piWin->IsInnerWindow()) {
      uint64_t windowID;
      if (piWin->GetCurrentInnerWindow()) {
        windowID = piWin->GetCurrentInnerWindow()->WindowID();
      } else {
        windowID = piWin->WindowID();
      }
      StreamListeners* listeners = GetActiveWindows()->Get(windowID);
      if (listeners) {
        uint32_t length = listeners->Length();
        for (uint32_t i = 0; i < length; ++i) {
          nsRefPtr<GetUserMediaCallbackMediaStreamListener> listener =
            listeners->ElementAt(i);
          if (listener->CapturingVideo()) {
            *aVideo = true;
          }
          if (listener->CapturingAudio()) {
            *aAudio = true;
          }
          if (*aAudio && *aVideo) {
            return NS_OK; 
          }
        }
      }
    }

    
    nsCOMPtr<nsIDocShell> docShell = piWin->GetDocShell();
    if (docShell) {
      int32_t i, count;
      docShell->GetChildCount(&count);
      for (i = 0; i < count; ++i) {
        nsCOMPtr<nsIDocShellTreeItem> item;
        docShell->GetChildAt(i, getter_AddRefs(item));
        nsCOMPtr<nsPIDOMWindow> win = do_GetInterface(item);

        MediaCaptureWindowStateInternal(win, aVideo, aAudio);
        if (*aAudio && *aVideo) {
          return NS_OK; 
        }
      }
    }
  }
  return NS_OK;
}

void
MediaManager::StopMediaStreams()
{
  nsCOMPtr<nsISupportsArray> array;
  GetActiveMediaCaptureWindows(getter_AddRefs(array));
  uint32_t len;
  array->Count(&len);
  for (uint32_t i = 0; i < len; i++) {
    nsCOMPtr<nsISupports> window;
    array->GetElementAt(i, getter_AddRefs(window));
    nsCOMPtr<nsPIDOMWindow> win(do_QueryInterface(window));
    if (win) {
      OnNavigation(win->WindowID());
    }
  }
}


void
GetUserMediaCallbackMediaStreamListener::Invalidate()
{

  nsRefPtr<MediaOperationRunnable> runnable;
  
  
  
  
  runnable = new MediaOperationRunnable(MEDIA_STOP,
                                        this, nullptr, nullptr,
                                        mAudioSource, mVideoSource,
                                        mFinished, mWindowID, nullptr);
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

NS_IMETHODIMP
GetUserMediaNotificationEvent::Run()
{
  NS_ASSERTION(NS_IsMainThread(), "Only call on main thread");
  
  
  
  
  nsRefPtr<DOMMediaStream> stream = mStream.forget();

  nsString msg;
  switch (mStatus) {
  case STARTING:
    msg = NS_LITERAL_STRING("starting");
    stream->OnTracksAvailable(mOnTracksAvailableCallback.forget());
    break;
  case STOPPING:
    msg = NS_LITERAL_STRING("shutdown");
    if (mListener) {
      mListener->SetStopped();
    }
    break;
  }

  nsCOMPtr<nsPIDOMWindow> window = nsGlobalWindow::GetInnerWindowWithId(mWindowID);
  NS_ENSURE_TRUE(window, NS_ERROR_FAILURE);

  return MediaManager::NotifyRecordingStatusChange(window, msg, mIsAudio, mIsVideo);
}

} 
