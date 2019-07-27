





#include "MediaManager.h"

#include "MediaStreamGraph.h"
#include "mozilla/dom/MediaStreamTrack.h"
#include "GetUserMediaRequest.h"
#include "nsHashPropertyBag.h"
#ifdef MOZ_WIDGET_GONK
#include "nsIAudioManager.h"
#endif
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
#include "nsIIDNService.h"
#include "nsNetUtil.h"
#include "nsPrincipal.h"
#include "nsICryptoHash.h"
#include "nsICryptoHMAC.h"
#include "nsIKeyModule.h"
#include "nsAppDirectoryServiceDefs.h"
#include "nsIInputStream.h"
#include "nsILineInputStream.h"
#include "mozilla/Types.h"
#include "mozilla/PeerIdentity.h"
#include "mozilla/dom/ContentChild.h"
#include "mozilla/dom/File.h"
#include "mozilla/dom/MediaStreamBinding.h"
#include "mozilla/dom/MediaStreamTrackBinding.h"
#include "mozilla/dom/GetUserMediaRequestBinding.h"
#include "mozilla/Preferences.h"
#include "mozilla/Base64.h"
#include "mozilla/ipc/BackgroundChild.h"
#include "mozilla/media/MediaChild.h"
#include "MediaTrackConstraints.h"
#include "VideoUtils.h"
#include "Latency.h"
#include "nsProxyRelease.h"
#include "nsNullPrincipal.h"


#include "prprf.h"

#include "nsJSUtils.h"
#include "nsGlobalWindow.h"
#include "nsIUUIDGenerator.h"
#include "nspr.h"
#include "nss.h"
#include "pk11pub.h"


#include "MediaEngineDefault.h"
#if defined(MOZ_WEBRTC)
#include "MediaEngineWebRTC.h"
#include "browser_logging/WebRtcLog.h"
#endif

#ifdef MOZ_B2G
#include "MediaPermissionGonk.h"
#endif

#if defined(XP_MACOSX)
#include "nsCocoaFeatures.h"
#endif
#if defined (XP_WIN)
#include "mozilla/WindowsVersion.h"
#endif



#ifdef GetCurrentTime
#undef GetCurrentTime
#endif


template<>
struct nsIMediaDevice::COMTypeInfo<mozilla::VideoDevice, void> {
  static const nsIID kIID;
};
const nsIID nsIMediaDevice::COMTypeInfo<mozilla::VideoDevice, void>::kIID = NS_IMEDIADEVICE_IID;
template<>
struct nsIMediaDevice::COMTypeInfo<mozilla::AudioDevice, void> {
  static const nsIID kIID;
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
using dom::MediaStreamError;
using dom::GetUserMediaRequest;
using dom::Sequence;
using dom::OwningBooleanOrMediaTrackConstraints;
using dom::SupportedAudioConstraints;
using dom::SupportedVideoConstraints;

static bool
HostInDomain(const nsCString &aHost, const nsCString &aPattern)
{
  int32_t patternOffset = 0;
  int32_t hostOffset = 0;

  
  if (aPattern.Length() > 2 && aPattern[0] == '*' && aPattern[1] == '.') {
    patternOffset = 2;

    
    hostOffset = aHost.FindChar('.') + 1;

    if (hostOffset <= 1) {
      
      return false;
    }
  }

  nsDependentCString hostRoot(aHost, hostOffset);
  return hostRoot.EqualsIgnoreCase(aPattern.BeginReading() + patternOffset);
}

static bool
HostHasPermission(nsIURI &docURI)
{
  nsresult rv;

  bool isHttps;
  rv = docURI.SchemeIs("https",&isHttps);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return false;
  }
  if (!isHttps) {
    return false;
  }

  nsAdoptingCString hostName;
  docURI.GetAsciiHost(hostName); 
  nsAdoptingCString domainWhiteList =
    Preferences::GetCString("media.getusermedia.screensharing.allowed_domains");
  domainWhiteList.StripWhitespace();

  if (domainWhiteList.IsEmpty() || hostName.IsEmpty()) {
    return false;
  }

  
  nsCOMPtr<nsIIDNService> idnService
    = do_GetService("@mozilla.org/network/idn-service;1", &rv);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return false;
  }

  uint32_t begin = 0;
  uint32_t end = 0;
  nsCString domainName;
  




  do {
    end = domainWhiteList.FindChar(',', begin);
    if (end == (uint32_t)-1) {
      
      end = domainWhiteList.Length();
    }

    rv = idnService->ConvertUTF8toACE(Substring(domainWhiteList, begin, end - begin),
                                      domainName);
    if (NS_SUCCEEDED(rv)) {
      if (HostInDomain(hostName, domainName)) {
        return true;
      }
    } else {
      NS_WARNING("Failed to convert UTF-8 host to ASCII");
    }

    begin = end + 1;
  } while (end < domainWhiteList.Length());

  return false;
}






template<class SuccessCallbackType>
class ErrorCallbackRunnable : public nsRunnable
{
public:
  ErrorCallbackRunnable(
    nsCOMPtr<SuccessCallbackType>& aOnSuccess,
    nsCOMPtr<nsIDOMGetUserMediaErrorCallback>& aOnFailure,
    MediaMgrError& aError,
    uint64_t aWindowID)
    : mError(&aError)
    , mWindowID(aWindowID)
    , mManager(MediaManager::GetInstance())
  {
    mOnSuccess.swap(aOnSuccess);
    mOnFailure.swap(aOnFailure);
  }

  NS_IMETHODIMP
  Run()
  {
    NS_ASSERTION(NS_IsMainThread(), "Only call on main thread");

    nsCOMPtr<SuccessCallbackType> onSuccess = mOnSuccess.forget();
    nsCOMPtr<nsIDOMGetUserMediaErrorCallback> onFailure = mOnFailure.forget();

    
    if (!(mManager->IsWindowStillActive(mWindowID))) {
      return NS_OK;
    }
    
    
    nsGlobalWindow* window = nsGlobalWindow::GetInnerWindowWithId(mWindowID);
    if (window) {
      nsRefPtr<MediaStreamError> error = new MediaStreamError(window, *mError);
      onFailure->OnError(error);
    }
    return NS_OK;
  }
private:
  ~ErrorCallbackRunnable()
  {
    MOZ_ASSERT(!mOnSuccess && !mOnFailure);
  }

  nsCOMPtr<SuccessCallbackType> mOnSuccess;
  nsCOMPtr<nsIDOMGetUserMediaErrorCallback> mOnFailure;
  nsRefPtr<MediaMgrError> mError;
  uint64_t mWindowID;
  nsRefPtr<MediaManager> mManager; 
};







class SuccessCallbackRunnable : public nsRunnable
{
public:
  SuccessCallbackRunnable(
    nsCOMPtr<nsIDOMGetUserMediaSuccessCallback>& aOnSuccess,
    nsCOMPtr<nsIDOMGetUserMediaErrorCallback>& aOnFailure,
    nsIDOMFile* aFile, uint64_t aWindowID)
    : mFile(aFile)
    , mWindowID(aWindowID)
    , mManager(MediaManager::GetInstance())
  {
    mOnSuccess.swap(aOnSuccess);
    mOnFailure.swap(aOnFailure);
  }

  NS_IMETHOD
  Run()
  {
    
    NS_ASSERTION(NS_IsMainThread(), "Only call on main thread");

    nsCOMPtr<nsIDOMGetUserMediaSuccessCallback> onSuccess = mOnSuccess.forget();
    nsCOMPtr<nsIDOMGetUserMediaErrorCallback> onFailure = mOnFailure.forget();

    if (!(mManager->IsWindowStillActive(mWindowID))) {
      return NS_OK;
    }
    
    
    onSuccess->OnSuccess(mFile);
    return NS_OK;
  }

private:
  nsCOMPtr<nsIDOMGetUserMediaSuccessCallback> mOnSuccess;
  nsCOMPtr<nsIDOMGetUserMediaErrorCallback> mOnFailure;
  nsCOMPtr<nsIDOMFile> mFile;
  uint64_t mWindowID;
  nsRefPtr<MediaManager> mManager; 
};






class DeviceSuccessCallbackRunnable: public nsRunnable
{
public:
  DeviceSuccessCallbackRunnable(
    uint64_t aWindowID,
    nsCOMPtr<nsIGetUserMediaDevicesSuccessCallback>& aOnSuccess,
    nsCOMPtr<nsIDOMGetUserMediaErrorCallback>& aOnFailure,
    nsTArray<nsRefPtr<MediaDevice>>* aDevices)
    : mDevices(aDevices)
    , mWindowID(aWindowID)
    , mManager(MediaManager::GetInstance())
  {
    mOnSuccess.swap(aOnSuccess);
    mOnFailure.swap(aOnFailure);
  }

  ~DeviceSuccessCallbackRunnable()
  {
    if (!NS_IsMainThread()) {
      
      
      nsCOMPtr<nsIThread> mainThread = do_GetMainThread();

      NS_ProxyRelease(mainThread, mOnSuccess);
      NS_ProxyRelease(mainThread, mOnFailure);
    }
  }

  nsresult
  AnonymizeId(nsAString& aId, const nsACString& aOriginKey)
  {
    nsresult rv;
    nsCOMPtr<nsIKeyObjectFactory> factory =
      do_GetService("@mozilla.org/security/keyobjectfactory;1", &rv);
    if (NS_FAILED(rv)) {
      return rv;
    }
    nsCString rawKey;
    rv = Base64Decode(aOriginKey, rawKey);
    if (NS_FAILED(rv)) {
      return rv;
    }
    nsCOMPtr<nsIKeyObject> key;
    rv = factory->KeyFromString(nsIKeyObject::HMAC, rawKey, getter_AddRefs(key));
    if (NS_FAILED(rv)) {
      return rv;
    }

    nsCOMPtr<nsICryptoHMAC> hasher =
      do_CreateInstance(NS_CRYPTO_HMAC_CONTRACTID, &rv);
    if (NS_FAILED(rv)) {
      return rv;
    }
    rv = hasher->Init(nsICryptoHMAC::SHA256, key);
    if (NS_FAILED(rv)) {
      return rv;
    }
    NS_ConvertUTF16toUTF8 id(aId);
    rv = hasher->Update(reinterpret_cast<const uint8_t*> (id.get()), id.Length());
    if (NS_FAILED(rv)) {
      return rv;
    }
    nsCString mac;
    rv = hasher->Finish(true, mac);
    if (NS_FAILED(rv)) {
      return rv;
    }

    aId = NS_ConvertUTF8toUTF16(mac);
    return NS_OK;
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

    size_t len = mDevices->Length();
    if (len == 0) {
      
      
      
      
      nsGlobalWindow* window = nsGlobalWindow::GetInnerWindowWithId(mWindowID);
      if (window) {
        nsRefPtr<MediaStreamError> error = new MediaStreamError(window,
            NS_LITERAL_STRING("NotFoundError"));
        mOnFailure->OnError(error);
      }
      return NS_OK;
    }

    nsTArray<nsIMediaDevice*> tmp(len);
    for (auto& device : *mDevices) {
      if (!mOriginKey.IsEmpty()) {
        nsString id;
        device->GetId(id);
        AnonymizeId(id, mOriginKey);
        device->SetId(id);
      }
      tmp.AppendElement(device);
    }

    devices->SetAsArray(nsIDataType::VTYPE_INTERFACE,
                        &NS_GET_IID(nsIMediaDevice),
                        mDevices->Length(),
                        const_cast<void*>(
                          static_cast<const void*>(tmp.Elements())
                        ));

    mOnSuccess->OnSuccess(devices);
    return NS_OK;
  }

  nsCString mOriginKey;
private:
  nsCOMPtr<nsIGetUserMediaDevicesSuccessCallback> mOnSuccess;
  nsCOMPtr<nsIDOMGetUserMediaErrorCallback> mOnFailure;
  nsAutoPtr<nsTArray<nsRefPtr<MediaDevice>>> mDevices;
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

MediaDevice::MediaDevice(MediaEngineSource* aSource)
  : mHasFacingMode(false)
  , mSource(aSource) {
  mSource->GetName(mName);
  mSource->GetUUID(mID);
}

VideoDevice::VideoDevice(MediaEngineVideoSource* aSource)
  : MediaDevice(aSource) {
#if defined(MOZ_B2G_CAMERA) && defined(MOZ_WIDGET_GONK)
  if (mName.EqualsLiteral("back")) {
    mHasFacingMode = true;
    mFacingMode = dom::VideoFacingModeEnum::Environment;
  } else if (mName.EqualsLiteral("front")) {
    mHasFacingMode = true;
    mFacingMode = dom::VideoFacingModeEnum::User;
  }
#endif 
#if defined(ANDROID) && !defined(MOZ_WIDGET_GONK)
  
  
  
  

  if (mName.Find(NS_LITERAL_STRING("Facing back")) != kNotFound) {
    mHasFacingMode = true;
    mFacingMode = dom::VideoFacingModeEnum::Environment;
  } else if (mName.Find(NS_LITERAL_STRING("Facing front")) != kNotFound) {
    mHasFacingMode = true;
    mFacingMode = dom::VideoFacingModeEnum::User;
  }
#endif 
#ifdef XP_MACOSX
  
  if (mName.Find(NS_LITERAL_STRING("Face")) != -1) {
    mHasFacingMode = true;
    mFacingMode = dom::VideoFacingModeEnum::User;
  }
#endif
  mMediaSource = aSource->GetMediaSource();
}








uint32_t
VideoDevice::GetBestFitnessDistance(
    const nsTArray<const MediaTrackConstraintSet*>& aConstraintSets)
{
  
  for (size_t i = 0; i < aConstraintSets.Length(); i++) {
    auto& c = *aConstraintSets[i];
    if (!c.mFacingMode.IsConstrainDOMStringParameters() ||
        c.mFacingMode.GetAsConstrainDOMStringParameters().mExact.WasPassed()) {
      nsString deviceFacingMode;
      GetFacingMode(deviceFacingMode);
      if (c.mFacingMode.IsString()) {
        if (c.mFacingMode.GetAsString() != deviceFacingMode) {
          return UINT32_MAX;
        }
      } else if (c.mFacingMode.IsStringSequence()) {
        if (!c.mFacingMode.GetAsStringSequence().Contains(deviceFacingMode)) {
          return UINT32_MAX;
        }
      } else {
        auto& exact = c.mFacingMode.GetAsConstrainDOMStringParameters().mExact.Value();
        if (exact.IsString()) {
          if (exact.GetAsString() != deviceFacingMode) {
            return UINT32_MAX;
          }
        } else if (!exact.GetAsStringSequence().Contains(deviceFacingMode)) {
          return UINT32_MAX;
        }
      }
    }
    nsString s;
    GetMediaSource(s);
    if (s != c.mMediaSource) {
      return UINT32_MAX;
    }
  }
  
  return GetSource()->GetBestFitnessDistance(aConstraintSets);
}

AudioDevice::AudioDevice(MediaEngineAudioSource* aSource)
  : MediaDevice(aSource) {}

uint32_t
AudioDevice::GetBestFitnessDistance(
    const nsTArray<const MediaTrackConstraintSet*>& aConstraintSets)
{
  
  return 0;
}

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
  aType.AssignLiteral(MOZ_UTF16("video"));
  return NS_OK;
}

NS_IMETHODIMP
AudioDevice::GetType(nsAString& aType)
{
  aType.AssignLiteral(MOZ_UTF16("audio"));
  return NS_OK;
}

NS_IMETHODIMP
MediaDevice::GetId(nsAString& aID)
{
  aID.Assign(mID);
  return NS_OK;
}

void
MediaDevice::SetId(const nsAString& aID)
{
  mID.Assign(aID);
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

NS_IMETHODIMP
MediaDevice::GetMediaSource(nsAString& aMediaSource)
{
  if (mMediaSource == dom::MediaSourceEnum::Microphone) {
    aMediaSource.Assign(NS_LITERAL_STRING("microphone"));
  } else if (mMediaSource == dom::MediaSourceEnum::Window) { 
    aMediaSource.Assign(NS_LITERAL_STRING("window"));
  } else { 
    aMediaSource.Assign(NS_ConvertUTF8toUTF16(
      dom::MediaSourceEnumValues::strings[uint32_t(mMediaSource)].value));
  }
  return NS_OK;
}

VideoDevice::Source*
VideoDevice::GetSource()
{
  return static_cast<Source*>(&*mSource);
}

AudioDevice::Source*
AudioDevice::GetSource()
{
  return static_cast<Source*>(&*mSource);
}





class nsDOMUserMediaStream : public DOMLocalMediaStream
{
public:
  static already_AddRefed<nsDOMUserMediaStream>
  CreateTrackUnionStream(nsIDOMWindow* aWindow,
                         GetUserMediaCallbackMediaStreamListener* aListener,
                         MediaEngineSource* aAudioSource,
                         MediaEngineSource* aVideoSource)
  {
    nsRefPtr<nsDOMUserMediaStream> stream = new nsDOMUserMediaStream(aListener,
                                                                     aAudioSource,
                                                                     aVideoSource);
    stream->InitTrackUnionStream(aWindow);
    return stream.forget();
  }

  nsDOMUserMediaStream(GetUserMediaCallbackMediaStreamListener* aListener,
                       MediaEngineSource *aAudioSource,
                       MediaEngineSource *aVideoSource) :
    mListener(aListener),
    mAudioSource(aAudioSource),
    mVideoSource(aVideoSource),
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

  virtual void Stop() override
  {
    if (mSourceStream) {
      mSourceStream->EndAllTrackAndFinish();
    }
  }

  
  
  
  
  virtual void StopTrack(TrackID aTrackID) override
  {
    if (mSourceStream) {
      mSourceStream->EndTrack(aTrackID);
      
      
      
      if (GetDOMTrackFor(aTrackID)) {
        mListener->StopTrack(aTrackID, !!GetDOMTrackFor(aTrackID)->AsAudioStreamTrack());
      } else {
        LOG(("StopTrack(%d) on non-existant track", aTrackID));
      }
    }
  }

#if 0
  virtual void NotifyMediaStreamTrackEnded(dom::MediaStreamTrack* aTrack)
  {
    TrackID trackID = aTrack->GetTrackID();
    
    LOG(("track %d ending, type = %s",
         trackID, aTrack->AsAudioStreamTrack() ? "audio" : "video"));
    MOZ_ASSERT(aTrack->AsVideoStreamTrack() || aTrack->AsAudioStreamTrack());
    mListener->StopTrack(trackID, !!aTrack->AsAudioStreamTrack());

    
    DOMLocalMediaStream::NotifyMediaStreamTrackEnded(aTrack);
  }
#endif

  
  virtual bool AddDirectListener(MediaStreamDirectListener *aListener) override
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

  virtual void RemoveDirectListener(MediaStreamDirectListener *aListener) override
  {
    if (mSourceStream) {
      mSourceStream->RemoveDirectListener(aListener);
    }
  }

  
  virtual void SetTrackEnabled(TrackID aID, bool aEnabled) override
  {
    
    

    
    
    GetStream()->AsProcessedStream()->ForwardTrackEnabled(aID, aEnabled);
  }

  virtual DOMLocalMediaStream* AsDOMLocalMediaStream() override
  {
    return this;
  }

  virtual MediaEngineSource* GetMediaEngine(TrackID aTrackID) override
  {
    
    
    if (aTrackID == kVideoTrack) {
      return mVideoSource;
    }
    else if (aTrackID == kAudioTrack) {
      return mAudioSource;
    }

    return nullptr;
  }

  
  
  nsRefPtr<SourceMediaStream> mSourceStream;
  nsRefPtr<MediaInputPort> mPort;
  nsRefPtr<GetUserMediaCallbackMediaStreamListener> mListener;
  nsRefPtr<MediaEngineSource> mAudioSource; 
  nsRefPtr<MediaEngineSource> mVideoSource;
  bool mEchoOn;
  bool mAgcOn;
  bool mNoiseOn;
  uint32_t mEcho;
  uint32_t mAgc;
  uint32_t mNoise;
  uint32_t mPlayoutDelay;
};


void
MediaOperationTask::ReturnCallbackError(nsresult rv, const char* errorLog)
{
  MM_LOG(("%s , rv=%d", errorLog, rv));
  NS_DispatchToMainThread(new ReleaseMediaOperationResource(mStream.forget(),
      mOnTracksAvailableCallback.forget()));
  nsString log;

  log.AssignASCII(errorLog);
  nsCOMPtr<nsIDOMGetUserMediaSuccessCallback> onSuccess;
  nsRefPtr<MediaMgrError> error = new MediaMgrError(
    NS_LITERAL_STRING("InternalError"), log);
  NS_DispatchToMainThread(
    new ErrorCallbackRunnable<nsIDOMGetUserMediaSuccessCallback>(onSuccess,
                                                                 mOnFailure,
                                                                 *error,
                                                                 mWindowID));
}















class GetUserMediaStreamRunnable : public nsRunnable
{
public:
  GetUserMediaStreamRunnable(
    nsCOMPtr<nsIDOMGetUserMediaSuccessCallback>& aOnSuccess,
    nsCOMPtr<nsIDOMGetUserMediaErrorCallback>& aOnFailure,
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
    mOnSuccess.swap(aOnSuccess);
    mOnFailure.swap(aOnFailure);
  }

  ~GetUserMediaStreamRunnable() {}

  class TracksAvailableCallback : public DOMMediaStream::OnTracksAvailableCallback
  {
  public:
    TracksAvailableCallback(MediaManager* aManager,
                            nsIDOMGetUserMediaSuccessCallback* aSuccess,
                            uint64_t aWindowID,
                            DOMMediaStream* aStream)
      : mWindowID(aWindowID), mOnSuccess(aSuccess), mManager(aManager),
        mStream(aStream) {}
    virtual void NotifyTracksAvailable(DOMMediaStream* aStream) override
    {
      
      if (!(mManager->IsWindowStillActive(mWindowID))) {
        return;
      }

      
      
      aStream->SetLogicalStreamStartTime(aStream->GetStream()->GetCurrentTime());

      
      
      LOG(("Returning success for getUserMedia()"));
      mOnSuccess->OnSuccess(aStream);
    }
    uint64_t mWindowID;
    nsCOMPtr<nsIDOMGetUserMediaSuccessCallback> mOnSuccess;
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
      nsDOMUserMediaStream::CreateTrackUnionStream(window, mListener,
                                                   mAudioSource, mVideoSource);
    if (!trackunion) {
      nsCOMPtr<nsIDOMGetUserMediaErrorCallback> onFailure = mOnFailure.forget();
      LOG(("Returning error for getUserMedia() - no stream"));

      nsGlobalWindow* window = nsGlobalWindow::GetInnerWindowWithId(mWindowID);
      if (window) {
        nsRefPtr<MediaStreamError> error = new MediaStreamError(window,
            NS_LITERAL_STRING("InternalError"),
            NS_LITERAL_STRING("No stream."));
        onFailure->OnError(error);
      }
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
      principal = nsNullPrincipal::Create();
      trackunion->SetPeerIdentity(mPeerIdentity.forget());
    } else {
      principal = window->GetExtantDoc()->NodePrincipal();
    }
    trackunion->CombineWithPrincipal(principal);

    
    
    
    
    mListener->Activate(stream.forget(), mAudioSource, mVideoSource);

    
    TracksAvailableCallback* tracksAvailableCallback =
      new TracksAvailableCallback(mManager, mOnSuccess, mWindowID, trackunion);

    mListener->AudioConfig(aec_on, (uint32_t) aec,
                           agc_on, (uint32_t) agc,
                           noise_on, (uint32_t) noise,
                           playout_delay);

    
    
    
    
    MediaManager::GetMessageLoop()->PostTask(FROM_HERE,
      new MediaOperationTask(MEDIA_START, mListener, trackunion,
                             tracksAvailableCallback,
                             mAudioSource, mVideoSource, false, mWindowID,
                             mOnFailure.forget()));

    
    mOnFailure = nullptr;
    return NS_OK;
  }

private:
  nsCOMPtr<nsIDOMGetUserMediaSuccessCallback> mOnSuccess;
  nsCOMPtr<nsIDOMGetUserMediaErrorCallback> mOnFailure;
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



template<class DeviceType, class ConstraintsType>
static void
  GetSources(MediaEngine *engine,
             ConstraintsType &aConstraints,
             void (MediaEngine::* aEnumerate)(dom::MediaSourceEnum,
                 nsTArray<nsRefPtr<typename DeviceType::Source> >*),
             nsTArray<nsRefPtr<DeviceType>>& aResult,
             const char* media_device_name = nullptr)
{
  typedef nsTArray<nsRefPtr<DeviceType>> SourceSet;

  nsString deviceName;
  
  SourceSet candidateSet;
  {
    nsTArray<nsRefPtr<typename DeviceType::Source> > sources;

    MediaSourceEnum src = StringToEnum(dom::MediaSourceEnumValues::strings,
                                       aConstraints.mMediaSource,
                                       dom::MediaSourceEnum::Other);
    (engine->*aEnumerate)(src, &sources);
    





    for (uint32_t len = sources.Length(), i = 0; i < len; i++) {
      sources[i]->GetName(deviceName);
      if (media_device_name && strlen(media_device_name) > 0)  {
        if (deviceName.EqualsASCII(media_device_name)) {
          candidateSet.AppendElement(new DeviceType(sources[i]));
          break;
        }
      } else {
        candidateSet.AppendElement(new DeviceType(sources[i]));
      }
    }
  }

  

  auto& c = aConstraints;

  

  
  
  
  nsTArray<const MediaTrackConstraintSet*> aggregateConstraints;
  aggregateConstraints.AppendElement(&c);

  for (uint32_t i = 0; i < candidateSet.Length();) {
    if (candidateSet[i]->GetBestFitnessDistance(aggregateConstraints) == UINT32_MAX) {
      candidateSet.RemoveElementAt(i);
    } else {
      ++i;
    }
  }

  

  if (c.mAdvanced.WasPassed()) {
    auto &array = c.mAdvanced.Value();

    for (int i = 0; i < int(array.Length()); i++) {
      aggregateConstraints.AppendElement(&array[i]);
      SourceSet rejects;
      for (uint32_t j = 0; j < candidateSet.Length();) {
        if (candidateSet[j]->GetBestFitnessDistance(aggregateConstraints) == UINT32_MAX) {
          rejects.AppendElement(candidateSet[j]);
          candidateSet.RemoveElementAt(j);
        } else {
          ++j;
        }
      }
      if (!candidateSet.Length()) {
        candidateSet.MoveElementsFrom(rejects);
        aggregateConstraints.RemoveElementAt(aggregateConstraints.Length() - 1);
      }
    }
  }
  aResult.MoveElementsFrom(candidateSet);
}









class GetUserMediaTask : public Task
{
public:
  GetUserMediaTask(
    const MediaStreamConstraints& aConstraints,
    already_AddRefed<nsIDOMGetUserMediaSuccessCallback> aOnSuccess,
    already_AddRefed<nsIDOMGetUserMediaErrorCallback> aOnFailure,
    uint64_t aWindowID, GetUserMediaCallbackMediaStreamListener *aListener,
    MediaEnginePrefs &aPrefs)
    : mConstraints(aConstraints)
    , mOnSuccess(aOnSuccess)
    , mOnFailure(aOnFailure)
    , mWindowID(aWindowID)
    , mListener(aListener)
    , mPrefs(aPrefs)
    , mDeviceChosen(false)
    , mBackend(nullptr)
    , mManager(MediaManager::GetInstance())
  {}

  



  GetUserMediaTask(
    const MediaStreamConstraints& aConstraints,
    already_AddRefed<nsIDOMGetUserMediaSuccessCallback> aOnSuccess,
    already_AddRefed<nsIDOMGetUserMediaErrorCallback> aOnFailure,
    uint64_t aWindowID, GetUserMediaCallbackMediaStreamListener *aListener,
    MediaEnginePrefs &aPrefs,
    MediaEngine* aBackend)
    : mConstraints(aConstraints)
    , mOnSuccess(aOnSuccess)
    , mOnFailure(aOnFailure)
    , mWindowID(aWindowID)
    , mListener(aListener)
    , mPrefs(aPrefs)
    , mDeviceChosen(false)
    , mBackend(aBackend)
    , mManager(MediaManager::GetInstance())
  {}

  ~GetUserMediaTask() {
  }

  void
  Fail(const nsAString& aName,
       const nsAString& aMessage = EmptyString()) {
    nsRefPtr<MediaMgrError> error = new MediaMgrError(aName, aMessage);
    nsRefPtr<ErrorCallbackRunnable<nsIDOMGetUserMediaSuccessCallback>> runnable =
      new ErrorCallbackRunnable<nsIDOMGetUserMediaSuccessCallback>(mOnSuccess,
                                                                   mOnFailure,
                                                                   *error,
                                                                   mWindowID);
    
    MOZ_ASSERT(!mOnSuccess);
    MOZ_ASSERT(!mOnFailure);

    NS_DispatchToMainThread(runnable);
    
    NS_DispatchToMainThread(new GetUserMediaListenerRemove(mWindowID, mListener));
  }

  void
  Run()
  {
    MOZ_ASSERT(!NS_IsMainThread());
    MOZ_ASSERT(mOnSuccess);
    MOZ_ASSERT(mOnFailure);

    MediaEngine* backend = mBackend;
    
    if (!backend) {
      backend = mManager->GetBackend(mWindowID);
    }

    
    if (!mDeviceChosen) {
      nsresult rv = SelectDevice(backend);
      if (rv != NS_OK) {
        return;
      }
    }

    
    ProcessGetUserMedia(((IsOn(mConstraints.mAudio) && mAudioDevice) ?
                         mAudioDevice->GetSource() : nullptr),
                        ((IsOn(mConstraints.mVideo) && mVideoDevice) ?
                         mVideoDevice->GetSource() : nullptr));
  }

  nsresult
  Denied(const nsAString& aName,
         const nsAString& aMessage = EmptyString())
  {
    MOZ_ASSERT(mOnSuccess);
    MOZ_ASSERT(mOnFailure);

    
    
    if (NS_IsMainThread()) {
      
      
      nsCOMPtr<nsIDOMGetUserMediaSuccessCallback> onSuccess = mOnSuccess.forget();
      nsCOMPtr<nsIDOMGetUserMediaErrorCallback> onFailure = mOnFailure.forget();

      nsGlobalWindow* window = nsGlobalWindow::GetInnerWindowWithId(mWindowID);
      if (window) {
        nsRefPtr<MediaStreamError> error = new MediaStreamError(window,
                                                                aName, aMessage);
        onFailure->OnError(error);
      }
      
      nsRefPtr<MediaManager> manager(MediaManager::GetInstance());
      manager->RemoveFromWindowList(mWindowID, mListener);
    } else {
      
      
      Fail(aName, aMessage);
    }

    MOZ_ASSERT(!mOnSuccess);
    MOZ_ASSERT(!mOnFailure);

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
    MOZ_ASSERT(mOnSuccess);
    MOZ_ASSERT(mOnFailure);
    if (IsOn(mConstraints.mVideo)) {
      nsTArray<nsRefPtr<VideoDevice>> sources;
      GetSources(backend, GetInvariant(mConstraints.mVideo),
                 &MediaEngine::EnumerateVideoDevices, sources);
      if (!sources.Length()) {
        Fail(NS_LITERAL_STRING("NotFoundError"));
        return NS_ERROR_FAILURE;
      }
      
      mVideoDevice = sources[0];
      LOG(("Selected video device"));
    }
    if (IsOn(mConstraints.mAudio)) {
      nsTArray<nsRefPtr<AudioDevice>> sources;
      GetSources(backend, GetInvariant(mConstraints.mAudio),
                 &MediaEngine::EnumerateAudioDevices, sources);
      if (!sources.Length()) {
        Fail(NS_LITERAL_STRING("NotFoundError"));
        return NS_ERROR_FAILURE;
      }
      
      mAudioDevice = sources[0];
      LOG(("Selected audio device"));
    }

    return NS_OK;
  }

  



  void
  ProcessGetUserMedia(MediaEngineAudioSource* aAudioSource,
                      MediaEngineVideoSource* aVideoSource)
  {
    MOZ_ASSERT(mOnSuccess);
    MOZ_ASSERT(mOnFailure);
    nsresult rv;
    if (aAudioSource) {
      rv = aAudioSource->Allocate(GetInvariant(mConstraints.mAudio), mPrefs);
      if (NS_FAILED(rv)) {
        LOG(("Failed to allocate audiosource %d",rv));
        Fail(NS_LITERAL_STRING("SourceUnavailableError"),
             NS_LITERAL_STRING("Failed to allocate audiosource"));
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
        Fail(NS_LITERAL_STRING("SourceUnavailableError"),
             NS_LITERAL_STRING("Failed to allocate videosource"));
        return;
      }
    }
    PeerIdentity* peerIdentity = nullptr;
    if (!mConstraints.mPeerIdentity.IsEmpty()) {
      peerIdentity = new PeerIdentity(mConstraints.mPeerIdentity);
    }

    NS_DispatchToMainThread(new GetUserMediaStreamRunnable(
      mOnSuccess, mOnFailure, mWindowID, mListener, aAudioSource, aVideoSource,
      peerIdentity
    ));

    MOZ_ASSERT(!mOnSuccess);
    MOZ_ASSERT(!mOnFailure);

    return;
  }

private:
  MediaStreamConstraints mConstraints;

  nsCOMPtr<nsIDOMGetUserMediaSuccessCallback> mOnSuccess;
  nsCOMPtr<nsIDOMGetUserMediaErrorCallback> mOnFailure;
  uint64_t mWindowID;
  nsRefPtr<GetUserMediaCallbackMediaStreamListener> mListener;
  nsRefPtr<AudioDevice> mAudioDevice;
  nsRefPtr<VideoDevice> mVideoDevice;
  MediaEnginePrefs mPrefs;

  bool mDeviceChosen;

  RefPtr<MediaEngine> mBackend;
  nsRefPtr<MediaManager> mManager; 
};

#if defined(ANDROID) && !defined(MOZ_WIDGET_GONK)
class GetUserMediaRunnableWrapper : public nsRunnable
{
public:
  
  GetUserMediaRunnableWrapper(GetUserMediaTask* task) :
    mTask(task) {
  }

  ~GetUserMediaRunnableWrapper() {
  }

  NS_IMETHOD Run() {
    mTask->Run();
    return NS_OK;
  }

private:
  nsAutoPtr<GetUserMediaTask> mTask;
};
#endif

class SanitizeDeviceIdsTask : public Task
{
public:
  explicit SanitizeDeviceIdsTask(int64_t aSinceWhen)
  : mSinceWhen(aSinceWhen) {}

  void 
  Run()
  {
    MOZ_ASSERT(!NS_IsMainThread());
    nsRefPtr<media::ChildPledge<bool>> p =
        mozilla::media::SanitizeOriginKeys(mSinceWhen); 
  }
private:
  int64_t mSinceWhen;
};









class GetUserMediaDevicesTask : public Task
{
public:
  GetUserMediaDevicesTask(
    const MediaStreamConstraints& aConstraints,
    already_AddRefed<nsIGetUserMediaDevicesSuccessCallback> aOnSuccess,
    already_AddRefed<nsIDOMGetUserMediaErrorCallback> aOnFailure,
    uint64_t aWindowId, nsACString& aAudioLoopbackDev,
    nsACString& aVideoLoopbackDev, bool aPrivileged, const nsACString& aOrigin,
    bool aInPrivateBrowsing, bool aUseFakeDevices)
    : mConstraints(aConstraints)
    , mOnSuccess(aOnSuccess)
    , mOnFailure(aOnFailure)
    , mManager(MediaManager::GetInstance())
    , mWindowId(aWindowId)
    , mLoopbackAudioDevice(aAudioLoopbackDev)
    , mLoopbackVideoDevice(aVideoLoopbackDev)
    , mPrivileged(aPrivileged)
    , mOrigin(aOrigin)
    , mInPrivateBrowsing(aInPrivateBrowsing)
    , mUseFakeDevices(aUseFakeDevices) {}

  void 
  Run()
  {
    MOZ_ASSERT(!NS_IsMainThread());

    nsRefPtr<MediaEngine> backend;
    if (mConstraints.mFake || mUseFakeDevices)
      backend = new MediaEngineDefault(mConstraints.mFakeTracks);
    else
      backend = mManager->GetBackend(mWindowId);

    typedef nsTArray<nsRefPtr<MediaDevice>> SourceSet;

    ScopedDeletePtr<SourceSet> result(new SourceSet);
    if (IsOn(mConstraints.mVideo)) {
      nsTArray<nsRefPtr<VideoDevice>> sources;
      GetSources(backend, GetInvariant(mConstraints.mVideo),
                 &MediaEngine::EnumerateVideoDevices, sources,
                 mLoopbackVideoDevice.get());
      for (auto& source : sources) {
        result->AppendElement(source);
      }
    }
    if (IsOn(mConstraints.mAudio)) {
      nsTArray<nsRefPtr<AudioDevice>> sources;
      GetSources(backend, GetInvariant(mConstraints.mAudio),
                 &MediaEngine::EnumerateAudioDevices, sources,
                 mLoopbackAudioDevice.get());
      for (auto& source : sources) {
        result->AppendElement(source);
      }
    }
    nsRefPtr<DeviceSuccessCallbackRunnable> runnable =
        new DeviceSuccessCallbackRunnable(mWindowId, mOnSuccess, mOnFailure,
                                          result.forget());
    if (mPrivileged) {
      NS_DispatchToMainThread(runnable);
    } else {
      
      
      
      
      
      
      nsRefPtr<media::ChildPledge<nsCString>> p =
          media::GetOriginKey(mOrigin, mInPrivateBrowsing);
      p->Then([runnable](nsCString result) mutable {
        runnable->mOriginKey = result;
        NS_DispatchToMainThread(runnable);
      });
    }
    
    MOZ_ASSERT(!mOnSuccess && !mOnFailure);
  }

private:
  MediaStreamConstraints mConstraints;
  nsCOMPtr<nsIGetUserMediaDevicesSuccessCallback> mOnSuccess;
  nsCOMPtr<nsIDOMGetUserMediaErrorCallback> mOnFailure;
  nsRefPtr<MediaManager> mManager;
  uint64_t mWindowId;
  const nsString mCallId;
  
  
  
  nsCString mLoopbackAudioDevice;
  nsCString mLoopbackVideoDevice;
  bool mPrivileged;
  nsCString mOrigin;
  bool mInPrivateBrowsing;
  bool mUseFakeDevices;
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

#ifdef DEBUG
 bool
MediaManager::IsInMediaThread()
{
  return sSingleton?
      (sSingleton->mMediaThread->thread_id() == PlatformThread::CurrentId()) :
      false;
}
#endif




  MediaManager*
MediaManager::Get() {
  if (!sSingleton) {
    NS_ASSERTION(NS_IsMainThread(), "Only create MediaManager on main thread");
#ifdef DEBUG
    static int timesCreated = 0;
    timesCreated++;
    MOZ_ASSERT(timesCreated == 1);
#endif
    sSingleton = new MediaManager();

    sSingleton->mMediaThread = new base::Thread("MediaManager");
    base::Thread::Options options;
#if defined(_WIN32)
    options.message_loop_type = MessageLoop::TYPE_MOZILLA_NONMAINUITHREAD;
#else
    options.message_loop_type = MessageLoop::TYPE_MOZILLA_NONMAINTHREAD;
#endif
    if (!sSingleton->mMediaThread->StartWithOptions(options)) {
      MOZ_CRASH();
    }

    LOG(("New Media thread for gum"));

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

  MediaManager*
MediaManager::GetIfExists() {
  return sSingleton;
}

 already_AddRefed<MediaManager>
MediaManager::GetInstance()
{
  
  nsRefPtr<MediaManager> service = MediaManager::Get();
  return service.forget();
}


MessageLoop*
MediaManager::GetMessageLoop()
{
  NS_ASSERTION(Get(), "MediaManager singleton?");
  NS_ASSERTION(Get()->mMediaThread, "No thread yet");
  return Get()->mMediaThread->message_loop();
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
MediaManager::GetUserMedia(
  nsPIDOMWindow* aWindow, const MediaStreamConstraints& aConstraints,
  nsIDOMGetUserMediaSuccessCallback* aOnSuccess,
  nsIDOMGetUserMediaErrorCallback* aOnFailure)
{
  NS_ASSERTION(NS_IsMainThread(), "Only call on main thread");

  NS_ENSURE_TRUE(aWindow, NS_ERROR_NULL_POINTER);
  NS_ENSURE_TRUE(aOnFailure, NS_ERROR_NULL_POINTER);
  NS_ENSURE_TRUE(aOnSuccess, NS_ERROR_NULL_POINTER);

  bool privileged = nsContentUtils::IsCallerChrome();

  nsCOMPtr<nsIDOMGetUserMediaSuccessCallback> onSuccess(aOnSuccess);
  nsCOMPtr<nsIDOMGetUserMediaErrorCallback> onFailure(aOnFailure);

  MediaStreamConstraints c(aConstraints); 

  static bool created = false;
  if (!created) {
    
    
    (void) MediaManager::Get();
#ifdef MOZ_B2G
    
    (void) MediaPermissionManager::GetInstance();
#endif 
  }

  uint64_t windowID = aWindow->WindowID();
  StreamListeners* listeners = AddWindowID(windowID);

  
  GetUserMediaCallbackMediaStreamListener* listener =
    new GetUserMediaCallbackMediaStreamListener(mMediaThread, windowID);

  
  listeners->AppendElement(listener);

  
  if (Preferences::GetBool("media.navigator.permission.disabled", false)) {
    privileged = true;
  }
  if (!Preferences::GetBool("media.navigator.video.enabled", true)) {
    c.mVideo.SetAsBoolean() = false;
  }
  bool fake = true;
  if (!c.mFake &&
      !Preferences::GetBool("media.navigator.streams.fake", false)) {
    fake = false;
  }

  
  nsAutoPtr<GetUserMediaTask> task;
  if (fake) {
    
    task = new GetUserMediaTask(c, onSuccess.forget(),
      onFailure.forget(), windowID, listener, mPrefs, new MediaEngineDefault(c.mFakeTracks));
  } else {
    
    task = new GetUserMediaTask(c, onSuccess.forget(),
      onFailure.forget(), windowID, listener, mPrefs);
  }

  nsIURI* docURI = aWindow->GetDocumentURI();

  bool isLoop = false;
  nsCOMPtr<nsIURI> loopURI;
  nsresult rv = NS_NewURI(getter_AddRefs(loopURI), "about:loopconversation");
  NS_ENSURE_SUCCESS(rv, rv);
  rv = docURI->EqualsExceptRef(loopURI, &isLoop);
  NS_ENSURE_SUCCESS(rv, rv);

  if (isLoop) {
    privileged = true;
  }

  if (c.mVideo.IsMediaTrackConstraints()) {
    auto& vc = c.mVideo.GetAsMediaTrackConstraints();
    MediaSourceEnum src = StringToEnum(dom::MediaSourceEnumValues::strings,
                                       vc.mMediaSource,
                                       dom::MediaSourceEnum::Other);
    if (vc.mAdvanced.WasPassed()) {
      if (src != dom::MediaSourceEnum::Camera) {
        
        const char *camera = EnumToASCII(dom::MediaSourceEnumValues::strings,
                                         dom::MediaSourceEnum::Camera);
        for (MediaTrackConstraintSet& cs : vc.mAdvanced.Value()) {
          if (cs.mMediaSource.EqualsASCII(camera)) {
            cs.mMediaSource = vc.mMediaSource;
          }
        }
      }
    }
    if (!privileged) {
      
      if (vc.mBrowserWindow.WasPassed()) {
        vc.mBrowserWindow.Construct(-1);
      }
      if (vc.mAdvanced.WasPassed()) {
        for (MediaTrackConstraintSet& cs : vc.mAdvanced.Value()) {
          if (cs.mBrowserWindow.WasPassed()) {
            cs.mBrowserWindow.Construct(-1);
          }
        }
      }
    }

    switch (src) {
    case dom::MediaSourceEnum::Camera:
      break;

    case dom::MediaSourceEnum::Browser:
    case dom::MediaSourceEnum::Screen:
    case dom::MediaSourceEnum::Application:
    case dom::MediaSourceEnum::Window:
      
      
      
      if (!Preferences::GetBool(((src == dom::MediaSourceEnum::Browser)?
                                "media.getusermedia.browser.enabled" :
                                "media.getusermedia.screensharing.enabled"),
                                false) ||
#if defined(XP_MACOSX) || defined(XP_WIN)
          (
            !Preferences::GetBool("media.getusermedia.screensharing.allow_on_old_platforms",
                                  false) &&
#if defined(XP_MACOSX)
            !nsCocoaFeatures::OnLionOrLater()
#endif
#if defined (XP_WIN)
            !IsVistaOrLater()
#endif
            ) ||
#endif
          (!privileged && !HostHasPermission(*docURI))) {
        return task->Denied(NS_LITERAL_STRING("PermissionDeniedError"));
      }
      break;

    case dom::MediaSourceEnum::Microphone:
    case dom::MediaSourceEnum::Other:
    default:
      return task->Denied(NS_LITERAL_STRING("NotFoundError"));
    }

    
    
    
    
    if (isLoop &&
        (src == dom::MediaSourceEnum::Window ||
         src == dom::MediaSourceEnum::Application ||
         src == dom::MediaSourceEnum::Screen)) {
       privileged = false;
    }
  }

#if defined(MOZ_B2G_CAMERA) && defined(MOZ_WIDGET_GONK)
  if (mCameraManager == nullptr) {
    mCameraManager = nsDOMCameraManager::CreateInstance(aWindow);
  }
#endif

  
  if (privileged ||
      (fake && !Preferences::GetBool("media.navigator.permission.fake"))) {
    MediaManager::GetMessageLoop()->PostTask(FROM_HERE, task.forget());
  } else {
    bool isHTTPS = false;
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
    }

    uint32_t videoPerm = nsIPermissionManager::UNKNOWN_ACTION;
    if (IsOn(c.mVideo)) {
      rv = permManager->TestExactPermissionFromPrincipal(
        aWindow->GetExtantDoc()->NodePrincipal(), "camera", &videoPerm);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    if ((!IsOn(c.mAudio) || audioPerm == nsIPermissionManager::DENY_ACTION) &&
        (!IsOn(c.mVideo) || videoPerm == nsIPermissionManager::DENY_ACTION)) {
      return task->Denied(NS_LITERAL_STRING("PermissionDeniedError"));
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

    
    mActiveCallbacks.Put(callID, task.forget());

    
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

#ifdef MOZ_WEBRTC
  EnableWebRtcLog();
#endif

  return NS_OK;
}

nsresult
MediaManager::GetUserMediaDevices(nsPIDOMWindow* aWindow,
  const MediaStreamConstraints& aConstraints,
  nsIGetUserMediaDevicesSuccessCallback* aOnSuccess,
  nsIDOMGetUserMediaErrorCallback* aOnFailure,
  uint64_t aInnerWindowID,
  bool aPrivileged)
{
  NS_ASSERTION(NS_IsMainThread(), "Only call on main thread");

  NS_ENSURE_TRUE(aOnFailure, NS_ERROR_NULL_POINTER);
  NS_ENSURE_TRUE(aOnSuccess, NS_ERROR_NULL_POINTER);

  nsCOMPtr<nsIGetUserMediaDevicesSuccessCallback> onSuccess(aOnSuccess);
  nsCOMPtr<nsIDOMGetUserMediaErrorCallback> onFailure(aOnFailure);

  
  nsAdoptingCString loopbackAudioDevice =
    Preferences::GetCString("media.audio_loopback_dev");
  nsAdoptingCString loopbackVideoDevice =
    Preferences::GetCString("media.video_loopback_dev");
  bool useFakeStreams =
    Preferences::GetBool("media.navigator.streams.fake", false);

  nsCString origin;
  nsPrincipal::GetOriginForURI(aWindow->GetDocumentURI(),
                               getter_Copies(origin));
  bool inPrivateBrowsing;
  {
    nsCOMPtr<nsIDocument> doc = aWindow->GetDoc();
    nsCOMPtr<nsILoadContext> loadContext = doc->GetLoadContext();
    inPrivateBrowsing = loadContext && loadContext->UsePrivateBrowsing();
  }
  MediaManager::GetMessageLoop()->PostTask(FROM_HERE,
    new GetUserMediaDevicesTask(
      aConstraints, onSuccess.forget(), onFailure.forget(),
      (aInnerWindowID ? aInnerWindowID : aWindow->WindowID()),
      loopbackAudioDevice, loopbackVideoDevice, aPrivileged, origin,
      inPrivateBrowsing, useFakeStreams));

  return NS_OK;
}

nsresult
MediaManager::EnumerateDevices(nsPIDOMWindow* aWindow,
                               nsIGetUserMediaDevicesSuccessCallback* aOnSuccess,
                               nsIDOMGetUserMediaErrorCallback* aOnFailure)
{
  NS_ASSERTION(NS_IsMainThread(), "Only call on main thread");

  MediaStreamConstraints c;
  c.mVideo.SetAsBoolean() = true;
  c.mAudio.SetAsBoolean() = true;

  AddWindowID(aWindow->WindowID());
  return GetUserMediaDevices(aWindow, c, aOnSuccess, aOnFailure, 0, false);
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

static void
StopSharingCallback(MediaManager *aThis,
                    uint64_t aWindowID,
                    StreamListeners *aListeners,
                    void *aData)
{
  if (aListeners) {
    auto length = aListeners->Length();
    for (size_t i = 0; i < length; ++i) {
      GetUserMediaCallbackMediaStreamListener *listener = aListeners->ElementAt(i);

      if (listener->Stream()) { 
        listener->Invalidate();
      }
      listener->Remove();
      listener->StopScreenWindowSharing();
    }
    aListeners->Clear();
    aThis->RemoveWindowID(aWindowID);
  }
}


void
MediaManager::OnNavigation(uint64_t aWindowID)
{
  NS_ASSERTION(NS_IsMainThread(), "OnNavigation called off main thread");
  LOG(("OnNavigation for %llu", aWindowID));

  
  

  nsTArray<nsString>* callIds;
  if (mCallIds.Get(aWindowID, &callIds)) {
    for (int i = 0, len = callIds->Length(); i < len; ++i) {
      mActiveCallbacks.Remove((*callIds)[i]);
    }
    mCallIds.Remove(aWindowID);
  }

  
  
  nsPIDOMWindow *window = static_cast<nsPIDOMWindow*>
      (nsGlobalWindow::GetInnerWindowWithId(aWindowID));
  if (window) {
    IterateWindowListeners(window, StopSharingCallback, nullptr);
  } else {
    RemoveWindowID(aWindowID);
  }
}

StreamListeners*
MediaManager::AddWindowID(uint64_t aWindowId)
{
  NS_ASSERTION(NS_IsMainThread(), "Only call on main thread");
  
  
  
  
  StreamListeners* listeners = GetActiveWindows()->Get(aWindowId);
  if (!listeners) {
    listeners = new StreamListeners;
    GetActiveWindows()->Put(aWindowId, listeners);
  }
  return listeners;
}

void
MediaManager::RemoveWindowID(uint64_t aWindowId)
{
  mActiveWindows.Remove(aWindowId);

  
  nsPIDOMWindow *window = static_cast<nsPIDOMWindow*>
    (nsGlobalWindow::GetInnerWindowWithId(aWindowId));
  if (!window) {
    LOG(("No inner window for %llu", aWindowId));
    return;
  }

  nsPIDOMWindow *outer = window->GetOuterWindow();
  if (!outer) {
    LOG(("No outer window for inner %llu", aWindowId));
    return;
  }

  uint64_t outerID = outer->WindowID();

  
  char windowBuffer[32];
  PR_snprintf(windowBuffer, sizeof(windowBuffer), "%llu", outerID);
  nsString data = NS_ConvertUTF8toUTF16(windowBuffer);

  nsCOMPtr<nsIObserverService> obs = services::GetObserverService();
  obs->NotifyObservers(nullptr, "recording-window-ended", data.get());
  LOG(("Sent recording-window-ended for window %llu (outer %llu)",
       aWindowId, outerID));
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

    
    

    class ShutdownTask : public Task
    {
    public:
      explicit ShutdownTask(nsRunnable* aReply) : mReply(aReply) {}
    private:
      virtual void
      Run()
      {
        MOZ_ASSERT(MediaManager::IsInMediaThread());
        mozilla::ipc::BackgroundChild::CloseForCurrentThread();
        NS_DispatchToMainThread(mReply);
      }
      nsRefPtr<nsRunnable> mReply;
    };

    
    
    
    
    
    

    MediaManager::GetMessageLoop()->PostTask(FROM_HERE, new ShutdownTask(
        media::CallbackRunnable::New([this]() mutable {
      
      MutexAutoLock lock(mMutex);
      GetActiveWindows()->Clear();
      mActiveCallbacks.Clear();
      mCallIds.Clear();
      LOG(("Releasing MediaManager singleton and thread"));
      
      sSingleton = nullptr;
      if (mMediaThread) {
        mMediaThread->Stop();
      }
      mBackend = nullptr;
      return NS_OK;
    })));
    return NS_OK;

  } else if (!strcmp(aTopic, "getUserMedia:response:allow")) {
    nsString key(aData);
    nsAutoPtr<GetUserMediaTask> task;
    mActiveCallbacks.RemoveAndForget(key, task);
    if (!task) {
      return NS_OK;
    }

    if (aSubject) {
      
      
      nsCOMPtr<nsISupportsArray> array(do_QueryInterface(aSubject));
      MOZ_ASSERT(array);
      uint32_t len = 0;
      array->Count(&len);
      MOZ_ASSERT(len);
      if (!len) {
        
        task->Denied(NS_LITERAL_STRING("PermissionDeniedError"));
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
            task->SetVideoDevice(static_cast<VideoDevice*>(device.get()));
          } else if (type.EqualsLiteral("audio")) {
            task->SetAudioDevice(static_cast<AudioDevice*>(device.get()));
          } else {
            NS_WARNING("Unknown device type in getUserMedia");
          }
        }
      }
    }

    
    MediaManager::GetMessageLoop()->PostTask(FROM_HERE, task.forget());
    return NS_OK;

  } else if (!strcmp(aTopic, "getUserMedia:response:deny")) {
    nsString errorMessage(NS_LITERAL_STRING("PermissionDeniedError"));

    if (aSubject) {
      nsCOMPtr<nsISupportsString> msg(do_QueryInterface(aSubject));
      MOZ_ASSERT(msg);
      msg->GetData(errorMessage);
      if (errorMessage.IsEmpty())
        errorMessage.AssignLiteral(MOZ_UTF16("InternalError"));
    }

    nsString key(aData);
    nsAutoPtr<GetUserMediaTask> task;
    mActiveCallbacks.RemoveAndForget(key, task);
    if (task) {
      task->Denied(errorMessage);
    }
    return NS_OK;

  } else if (!strcmp(aTopic, "getUserMedia:revoke")) {
    nsresult rv;
    
    nsDependentString data(aData);
    if (Substring(data, 0, strlen("screen:")).EqualsLiteral("screen:")) {
      uint64_t windowID = PromiseFlatString(Substring(data, strlen("screen:"))).ToInteger64(&rv);
      MOZ_ASSERT(NS_SUCCEEDED(rv));
      if (NS_SUCCEEDED(rv)) {
        LOG(("Revoking Screeen/windowCapture access for window %llu", windowID));
        StopScreensharing(windowID);
      }
    } else {
      uint64_t windowID = nsString(aData).ToInteger64(&rv);
      MOZ_ASSERT(NS_SUCCEEDED(rv));
      if (NS_SUCCEEDED(rv)) {
        LOG(("Revoking MediaCapture access for window %llu", windowID));
        OnNavigation(windowID);
      }
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
        if (listener->CapturingVideo() || listener->CapturingAudio() ||
            listener->CapturingScreen() || listener->CapturingWindow() ||
            listener->CapturingApplication()) {
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


struct CaptureWindowStateData {
  bool *mVideo;
  bool *mAudio;
  bool *mScreenShare;
  bool *mWindowShare;
  bool *mAppShare;
  bool *mBrowserShare;
};

static void
CaptureWindowStateCallback(MediaManager *aThis,
                           uint64_t aWindowID,
                           StreamListeners *aListeners,
                           void *aData)
{
  struct CaptureWindowStateData *data = (struct CaptureWindowStateData *) aData;

  if (aListeners) {
    auto length = aListeners->Length();
    for (size_t i = 0; i < length; ++i) {
      GetUserMediaCallbackMediaStreamListener *listener = aListeners->ElementAt(i);

      if (listener->CapturingVideo()) {
        *data->mVideo = true;
      }
      if (listener->CapturingAudio()) {
        *data->mAudio = true;
      }
      if (listener->CapturingScreen()) {
        *data->mScreenShare = true;
      }
      if (listener->CapturingWindow()) {
        *data->mWindowShare = true;
      }
      if (listener->CapturingApplication()) {
        *data->mAppShare = true;
      }
      if (listener->CapturingBrowser()) {
        *data->mBrowserShare = true;
      }
    }
  }
}


NS_IMETHODIMP
MediaManager::MediaCaptureWindowState(nsIDOMWindow* aWindow, bool* aVideo,
                                      bool* aAudio, bool *aScreenShare,
                                      bool* aWindowShare, bool *aAppShare,
                                      bool *aBrowserShare)
{
  NS_ASSERTION(NS_IsMainThread(), "Only call on main thread");
  struct CaptureWindowStateData data;
  data.mVideo = aVideo;
  data.mAudio = aAudio;
  data.mScreenShare = aScreenShare;
  data.mWindowShare = aWindowShare;
  data.mAppShare = aAppShare;
  data.mBrowserShare = aBrowserShare;

  *aVideo = false;
  *aAudio = false;
  *aScreenShare = false;
  *aWindowShare = false;
  *aAppShare = false;
  *aBrowserShare = false;

  nsCOMPtr<nsPIDOMWindow> piWin = do_QueryInterface(aWindow);
  if (piWin) {
    IterateWindowListeners(piWin, CaptureWindowStateCallback, &data);
  }
#ifdef DEBUG
  LOG(("%s: window %lld capturing %s %s %s %s %s %s", __FUNCTION__, piWin ? piWin->WindowID() : -1,
       *aVideo ? "video" : "", *aAudio ? "audio" : "",
       *aScreenShare ? "screenshare" : "",  *aWindowShare ? "windowshare" : "",
       *aAppShare ? "appshare" : "", *aBrowserShare ? "browsershare" : ""));
#endif
  return NS_OK;
}

NS_IMETHODIMP
MediaManager::SanitizeDeviceIds(int64_t aSinceWhen)
{
  NS_ASSERTION(NS_IsMainThread(), "Only call on main thread");
  LOG(("%s: sinceWhen = %llu", __FUNCTION__, aSinceWhen));

  MediaManager::GetMessageLoop()->PostTask(FROM_HERE,
    new SanitizeDeviceIdsTask(aSinceWhen));
  return NS_OK;
}

static void
StopScreensharingCallback(MediaManager *aThis,
                          uint64_t aWindowID,
                          StreamListeners *aListeners,
                          void *aData)
{
  if (aListeners) {
    auto length = aListeners->Length();
    for (size_t i = 0; i < length; ++i) {
      aListeners->ElementAt(i)->StopScreenWindowSharing();
    }
  }
}

void
MediaManager::StopScreensharing(uint64_t aWindowID)
{
  
  

  nsPIDOMWindow *window = static_cast<nsPIDOMWindow*>
      (nsGlobalWindow::GetInnerWindowWithId(aWindowID));
  if (!window) {
    return;
  }
  IterateWindowListeners(window, &StopScreensharingCallback, nullptr);
}


void
MediaManager::IterateWindowListeners(nsPIDOMWindow *aWindow,
                                     WindowListenerCallback aCallback,
                                     void *aData)
{
  
  
  nsCOMPtr<nsPIDOMWindow> piWin = do_QueryInterface(aWindow);
  if (piWin) {
    if (piWin->IsInnerWindow() || piWin->GetCurrentInnerWindow()) {
      uint64_t windowID;
      if (piWin->IsInnerWindow()) {
        windowID = piWin->WindowID();
      } else {
        windowID = piWin->GetCurrentInnerWindow()->WindowID();
      }
      StreamListeners* listeners = GetActiveWindows()->Get(windowID);
      
      (*aCallback)(this, windowID, listeners, aData);
    }

    
    nsCOMPtr<nsIDocShell> docShell = piWin->GetDocShell();
    if (docShell) {
      int32_t i, count;
      docShell->GetChildCount(&count);
      for (i = 0; i < count; ++i) {
        nsCOMPtr<nsIDocShellTreeItem> item;
        docShell->GetChildAt(i, getter_AddRefs(item));
        nsCOMPtr<nsPIDOMWindow> win = item ? item->GetWindow() : nullptr;

        if (win) {
          IterateWindowListeners(win, aCallback, aData);
        }
      }
    }
  }
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

bool
MediaManager::IsWindowActivelyCapturing(uint64_t aWindowId)
{
  nsCOMPtr<nsISupportsArray> array;
  GetActiveMediaCaptureWindows(getter_AddRefs(array));
  uint32_t len;
  array->Count(&len);
  for (uint32_t i = 0; i < len; i++) {
    nsCOMPtr<nsISupports> window;
    array->GetElementAt(i, getter_AddRefs(window));
    nsCOMPtr<nsPIDOMWindow> win(do_QueryInterface(window));
    if (win && win->WindowID() == aWindowId) {
      return true;
    }
  }
  return false;
}

void
GetUserMediaCallbackMediaStreamListener::AudioConfig(bool aEchoOn,
              uint32_t aEcho,
              bool aAgcOn, uint32_t aAGC,
              bool aNoiseOn, uint32_t aNoise,
              int32_t aPlayoutDelay)
{
  if (mAudioSource) {
#ifdef MOZ_WEBRTC
    mMediaThread->message_loop()->PostTask(FROM_HERE,
      NewRunnableMethod(mAudioSource.get(), &MediaEngineSource::Config,
                        aEchoOn, aEcho, aAgcOn, aAGC, aNoiseOn,
                        aNoise, aPlayoutDelay));
#else
    unused << mMediaThread;
#endif
  }
}


void
GetUserMediaCallbackMediaStreamListener::Invalidate()
{
  
  
  
  
  MediaManager::GetMessageLoop()->PostTask(FROM_HERE,
    new MediaOperationTask(MEDIA_STOP,
                           this, nullptr, nullptr,
                           mAudioSource, mVideoSource,
                           mFinished, mWindowID, nullptr));
}



void
GetUserMediaCallbackMediaStreamListener::StopScreenWindowSharing()
{
  NS_ASSERTION(NS_IsMainThread(), "Only call on main thread");
  if (mVideoSource && !mStopped &&
      (mVideoSource->GetMediaSource() == dom::MediaSourceEnum::Screen ||
       mVideoSource->GetMediaSource() == dom::MediaSourceEnum::Application ||
       mVideoSource->GetMediaSource() == dom::MediaSourceEnum::Window)) {
    
    MediaManager::GetMessageLoop()->PostTask(FROM_HERE,
      new MediaOperationTask(mAudioSource ? MEDIA_STOP_TRACK : MEDIA_STOP,
                             this, nullptr, nullptr,
                             nullptr, mVideoSource,
                             mFinished, mWindowID, nullptr));
  }
}



void
GetUserMediaCallbackMediaStreamListener::StopTrack(TrackID aID, bool aIsAudio)
{
  if (((aIsAudio && mAudioSource) ||
       (!aIsAudio && mVideoSource)) && !mStopped)
  {
    
    
    MediaManager::GetMessageLoop()->PostTask(FROM_HERE,
      new MediaOperationTask(MEDIA_STOP_TRACK,
                             this, nullptr, nullptr,
                             aIsAudio  ? mAudioSource : nullptr,
                             !aIsAudio ? mVideoSource : nullptr,
                             mFinished, mWindowID, nullptr));
  } else {
    LOG(("gUM track %d ended, but we don't have type %s",
         aID, aIsAudio ? "audio" : "video"));
  }
}


void
GetUserMediaCallbackMediaStreamListener::NotifyFinished(MediaStreamGraph* aGraph)
{
  mFinished = true;
  Invalidate(); 
  NS_DispatchToMainThread(new GetUserMediaListenerRemove(mWindowID, this));
}


void
GetUserMediaCallbackMediaStreamListener::NotifyDirectListeners(MediaStreamGraph* aGraph,
                                                               bool aHasListeners)
{
  MediaManager::GetMessageLoop()->PostTask(FROM_HERE,
    new MediaOperationTask(MEDIA_DIRECT_LISTENERS,
                           this, nullptr, nullptr,
                           mAudioSource, mVideoSource,
                           aHasListeners, mWindowID, nullptr));
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
  case STOPPED_TRACK:
    msg = NS_LITERAL_STRING("shutdown");
    break;
  }

  nsCOMPtr<nsPIDOMWindow> window = nsGlobalWindow::GetInnerWindowWithId(mWindowID);
  NS_ENSURE_TRUE(window, NS_ERROR_FAILURE);

  return MediaManager::NotifyRecordingStatusChange(window, msg, mIsAudio, mIsVideo);
}

} 
