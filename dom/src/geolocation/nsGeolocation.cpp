



#include "nsXULAppAPI.h"

#include "mozilla/dom/ContentChild.h"
#include "mozilla/dom/TabChild.h"

#include "nsISettingsService.h"

#include "nsGeolocation.h"
#include "nsDOMClassInfoID.h"
#include "nsComponentManagerUtils.h"
#include "nsServiceManagerUtils.h"
#include "nsContentUtils.h"
#include "nsCxPusher.h"
#include "nsIDocument.h"
#include "nsIObserverService.h"
#include "nsPIDOMWindow.h"
#include "nsThreadUtils.h"
#include "mozilla/Services.h"
#include "mozilla/unused.h"
#include "mozilla/Preferences.h"
#include "mozilla/ClearOnShutdown.h"
#include "PCOMContentPermissionRequestChild.h"

class nsIPrincipal;

#ifdef MOZ_ENABLE_QTMOBILITY
#include "QTMLocationProvider.h"
#endif

#ifdef MOZ_WIDGET_ANDROID
#include "AndroidLocationProvider.h"
#endif

#ifdef MOZ_WIDGET_GONK
#include "GonkGPSGeolocationProvider.h"
#endif

#ifdef MOZ_WIDGET_COCOA
#include "CoreLocationLocationProvider.h"
#endif



#define MAX_GEO_REQUESTS_PER_WINDOW  1500


#define GEO_SETINGS_ENABLED          "geolocation.enabled"

using mozilla::unused;          
using namespace mozilla;
using namespace mozilla::dom;

class nsGeolocationRequest
 : public nsIContentPermissionRequest
 , public nsITimerCallback
 , public nsIGeolocationUpdate
 , public PCOMContentPermissionRequestChild
{
 public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_NSICONTENTPERMISSIONREQUEST
  NS_DECL_NSITIMERCALLBACK
  NS_DECL_NSIGEOLOCATIONUPDATE

  NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(nsGeolocationRequest, nsIContentPermissionRequest)

  nsGeolocationRequest(Geolocation* aLocator,
                       const GeoPositionCallback& aCallback,
                       const GeoPositionErrorCallback& aErrorCallback,
                       PositionOptions* aOptions,
                       bool aWatchPositionRequest = false,
                       int32_t aWatchId = 0);
  void Shutdown();

  void SendLocation(nsIDOMGeoPosition* location);
  bool WantsHighAccuracy() {return mOptions && mOptions->mEnableHighAccuracy;}
  void SetTimeoutTimer();
  nsIPrincipal* GetPrincipal();

  ~nsGeolocationRequest();

  virtual bool Recv__delete__(const bool& allow) MOZ_OVERRIDE;
  virtual void IPDLRelease() MOZ_OVERRIDE { Release(); }

  bool IsWatch() { return mIsWatchPositionRequest; }
  int32_t WatchId() { return mWatchId; }
 private:
  bool mIsWatchPositionRequest;

  nsCOMPtr<nsITimer> mTimeoutTimer;
  GeoPositionCallback mCallback;
  GeoPositionErrorCallback mErrorCallback;
  nsAutoPtr<PositionOptions> mOptions;

  nsRefPtr<Geolocation> mLocator;

  int32_t mWatchId;
  bool mShutdown;
};

static PositionOptions*
CreatePositionOptionsCopy(const PositionOptions& aOptions)
{
  nsAutoPtr<PositionOptions> geoOptions(new PositionOptions());

  geoOptions->mEnableHighAccuracy = aOptions.mEnableHighAccuracy;
  geoOptions->mMaximumAge = aOptions.mMaximumAge;
  geoOptions->mTimeout = aOptions.mTimeout;

  return geoOptions.forget();
}

class GeolocationSettingsCallback : public nsISettingsServiceCallback
{
public:
  NS_DECL_ISUPPORTS

  GeolocationSettingsCallback() {
    MOZ_COUNT_CTOR(GeolocationSettingsCallback);
  }

  virtual ~GeolocationSettingsCallback() {
    MOZ_COUNT_DTOR(GeolocationSettingsCallback);
  }

  NS_IMETHOD Handle(const nsAString& aName, const JS::Value& aResult)
  {
    MOZ_ASSERT(NS_IsMainThread());

    
    bool value = true;
    if (aResult.isBoolean()) {
      value = aResult.toBoolean();
    }

    MozSettingValue(value);
    return NS_OK;
  }

  NS_IMETHOD HandleError(const nsAString& aName)
  {
    NS_WARNING("Unable to get value for '" GEO_SETINGS_ENABLED "'");

    
    MozSettingValue(true);
    return NS_OK;
  }

  void MozSettingValue(const bool aValue)
  {
    nsRefPtr<nsGeolocationService> gs = nsGeolocationService::GetGeolocationService();
    if (gs) {
      gs->HandleMozsettingValue(aValue);
    }
  }
};

NS_IMPL_ISUPPORTS1(GeolocationSettingsCallback, nsISettingsServiceCallback)

class RequestPromptEvent : public nsRunnable
{
public:
  RequestPromptEvent(nsGeolocationRequest* request)
    : mRequest(request)
  {
  }

  NS_IMETHOD Run() {
    nsCOMPtr<nsIContentPermissionPrompt> prompt = do_CreateInstance(NS_CONTENT_PERMISSION_PROMPT_CONTRACTID);
    if (prompt) {
      prompt->Prompt(mRequest);
    }
    return NS_OK;
  }

private:
  nsRefPtr<nsGeolocationRequest> mRequest;
};

class RequestAllowEvent : public nsRunnable
{
public:
  RequestAllowEvent(int allow, nsGeolocationRequest* request)
    : mAllow(allow),
      mRequest(request)
  {
  }

  NS_IMETHOD Run() {
    if (mAllow) {
      mRequest->Allow();
    } else {
      mRequest->Cancel();
    }
    return NS_OK;
  }

private:
  bool mAllow;
  nsRefPtr<nsGeolocationRequest> mRequest;
};

class RequestSendLocationEvent : public nsRunnable
{
public:
  RequestSendLocationEvent(nsIDOMGeoPosition* aPosition,
                           nsGeolocationRequest* aRequest)
    : mPosition(aPosition),
      mRequest(aRequest)
  {
  }

  NS_IMETHOD Run() {
    mRequest->SendLocation(mPosition);
    return NS_OK;
  }

private:
  nsCOMPtr<nsIDOMGeoPosition> mPosition;
  nsRefPtr<nsGeolocationRequest> mRequest;
  nsRefPtr<Geolocation> mLocator;
};





NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(PositionError)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDOMGeoPositionError)
  NS_INTERFACE_MAP_ENTRY(nsIDOMGeoPositionError)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_1(PositionError, mParent)
NS_IMPL_CYCLE_COLLECTING_ADDREF(PositionError)
NS_IMPL_CYCLE_COLLECTING_RELEASE(PositionError)

PositionError::PositionError(Geolocation* aParent, int16_t aCode)
  : mCode(aCode)
  , mParent(aParent)
{
  SetIsDOMBinding();
}

PositionError::~PositionError(){}


NS_IMETHODIMP
PositionError::GetCode(int16_t *aCode)
{
  NS_ENSURE_ARG_POINTER(aCode);
  *aCode = Code();
  return NS_OK;
}

NS_IMETHODIMP
PositionError::GetMessage(nsAString& aMessage)
{
  switch (mCode)
  {
    case nsIDOMGeoPositionError::PERMISSION_DENIED:
      aMessage = NS_LITERAL_STRING("User denied geolocation prompt");
      break;
    case nsIDOMGeoPositionError::POSITION_UNAVAILABLE:
      aMessage = NS_LITERAL_STRING("Unknown error acquiring position");
      break;
    case nsIDOMGeoPositionError::TIMEOUT:
      aMessage = NS_LITERAL_STRING("Position acquisition timed out");
      break;
    default:
      break;
  }
  return NS_OK;
}

Geolocation*
PositionError::GetParentObject() const
{
  return mParent;
}

JSObject*
PositionError::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aScope)
{
  return PositionErrorBinding::Wrap(aCx, aScope, this);
}

void
PositionError::NotifyCallback(const GeoPositionErrorCallback& aCallback)
{
  
  nsCxPusher pusher;
  pusher.PushNull();

  nsAutoMicroTask mt;
  if (aCallback.HasWebIDLCallback()) {
    PositionErrorCallback* callback = aCallback.GetWebIDLCallback();

    if (callback) {
      ErrorResult err;
      callback->Call(*this, err);
    }
  } else {
    nsIDOMGeoPositionErrorCallback* callback = aCallback.GetXPCOMCallback();
    if (callback) {
      callback->HandleEvent(this);
    }
  }
}




nsGeolocationRequest::nsGeolocationRequest(Geolocation* aLocator,
                                           const GeoPositionCallback& aCallback,
                                           const GeoPositionErrorCallback& aErrorCallback,
                                           PositionOptions* aOptions,
                                           bool aWatchPositionRequest,
                                           int32_t aWatchId)
  : mIsWatchPositionRequest(aWatchPositionRequest),
    mCallback(aCallback),
    mErrorCallback(aErrorCallback),
    mOptions(aOptions),
    mLocator(aLocator),
    mWatchId(aWatchId),
    mShutdown(false)
{
}

nsGeolocationRequest::~nsGeolocationRequest()
{
}

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsGeolocationRequest)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIContentPermissionRequest)
  NS_INTERFACE_MAP_ENTRY(nsIContentPermissionRequest)
  NS_INTERFACE_MAP_ENTRY(nsITimerCallback)
  NS_INTERFACE_MAP_ENTRY(nsIGeolocationUpdate)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsGeolocationRequest)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsGeolocationRequest)

NS_IMPL_CYCLE_COLLECTION_3(nsGeolocationRequest, mCallback, mErrorCallback, mLocator)

NS_IMETHODIMP
nsGeolocationRequest::Notify(nsITimer* aTimer)
{
  MOZ_ASSERT(!mShutdown, "timeout after shutdown");

  if (!mIsWatchPositionRequest) {
    Shutdown();
    mLocator->RemoveRequest(this);
  }

  NotifyError(nsIDOMGeoPositionError::TIMEOUT);

  if (!mShutdown) {
    SetTimeoutTimer();
  }

  return NS_OK;
}

NS_IMETHODIMP
nsGeolocationRequest::GetPrincipal(nsIPrincipal * *aRequestingPrincipal)
{
  NS_ENSURE_ARG_POINTER(aRequestingPrincipal);

  nsCOMPtr<nsIPrincipal> principal = mLocator->GetPrincipal();
  principal.forget(aRequestingPrincipal);

  return NS_OK;
}

NS_IMETHODIMP
nsGeolocationRequest::GetType(nsACString & aType)
{
  aType = "geolocation";
  return NS_OK;
}

NS_IMETHODIMP
nsGeolocationRequest::GetAccess(nsACString & aAccess)
{
  aAccess = "unused";
  return NS_OK;
}

NS_IMETHODIMP
nsGeolocationRequest::GetWindow(nsIDOMWindow * *aRequestingWindow)
{
  NS_ENSURE_ARG_POINTER(aRequestingWindow);

  nsCOMPtr<nsIDOMWindow> window = do_QueryReferent(mLocator->GetOwner());
  window.forget(aRequestingWindow);

  return NS_OK;
}

NS_IMETHODIMP
nsGeolocationRequest::GetElement(nsIDOMElement * *aRequestingElement)
{
  NS_ENSURE_ARG_POINTER(aRequestingElement);
  *aRequestingElement = nullptr;
  return NS_OK;
}

NS_IMETHODIMP
nsGeolocationRequest::Cancel()
{
  NotifyError(nsIDOMGeoPositionError::PERMISSION_DENIED);
  return NS_OK;
}

NS_IMETHODIMP
nsGeolocationRequest::Allow()
{
  
  nsRefPtr<nsGeolocationService> gs = nsGeolocationService::GetGeolocationService();
  nsresult rv = gs->StartDevice(GetPrincipal());

  if (NS_FAILED(rv)) {
    
    NotifyError(nsIDOMGeoPositionError::POSITION_UNAVAILABLE);
    return NS_OK;
  }

  nsCOMPtr<nsIDOMGeoPosition> lastPosition = gs->GetCachedPosition();
  DOMTimeStamp cachedPositionTime;
  if (lastPosition) {
    lastPosition->GetTimestamp(&cachedPositionTime);
  }

  
  

  uint32_t maximumAge = 0;
  if (mOptions) {
    if (mOptions->mMaximumAge > 0) {
      maximumAge = mOptions->mMaximumAge;
    }
  }
  gs->SetHigherAccuracy(mOptions && mOptions->mEnableHighAccuracy);

  bool canUseCache = lastPosition && maximumAge > 0 &&
    (PRTime(PR_Now() / PR_USEC_PER_MSEC) - maximumAge <=
    PRTime(cachedPositionTime));

  if (canUseCache) {
    
    
    
    Update(lastPosition);
  }

  if (mIsWatchPositionRequest || !canUseCache) {
    
    
    mLocator->NotifyAllowedRequest(this);
  }

  SetTimeoutTimer();

  return NS_OK;
}

void
nsGeolocationRequest::SetTimeoutTimer()
{
  if (mTimeoutTimer) {
    mTimeoutTimer->Cancel();
    mTimeoutTimer = nullptr;
  }

  int32_t timeout;
  if (mOptions && (timeout = mOptions->mTimeout) != 0) {

    if (timeout < 0) {
      timeout = 0;
    } else if (timeout < 10) {
      timeout = 10;
    }

    mTimeoutTimer = do_CreateInstance("@mozilla.org/timer;1");
    mTimeoutTimer->InitWithCallback(this, timeout, nsITimer::TYPE_ONE_SHOT);
  }
}

void
nsGeolocationRequest::SendLocation(nsIDOMGeoPosition* aPosition)
{
  if (mShutdown) {
    
    return;
  }

  nsRefPtr<Position> wrapped, cachedWrapper = mLocator->GetCachedPosition();
  if (cachedWrapper && aPosition == cachedWrapper->GetWrappedGeoPosition()) {
    wrapped = cachedWrapper;
  } else if (aPosition) {
    nsCOMPtr<nsIDOMGeoPositionCoords> coords;
    aPosition->GetCoords(getter_AddRefs(coords));
    if (coords) {
      wrapped = new Position(ToSupports(mLocator), aPosition);
    }
  }

  if (!wrapped) {
    NotifyError(nsIDOMGeoPositionError::POSITION_UNAVAILABLE);
    return;
  }

  mLocator->SetCachedPosition(wrapped);
  if (!mIsWatchPositionRequest) {
    
    
    Shutdown();
  }

  
  nsCxPusher pusher;
  pusher.PushNull();
  nsAutoMicroTask mt;
  if (mCallback.HasWebIDLCallback()) {
    ErrorResult err;
    PositionCallback* callback = mCallback.GetWebIDLCallback();

    MOZ_ASSERT(callback);
    callback->Call(*wrapped, err);
  } else {
    nsIDOMGeoPositionCallback* callback = mCallback.GetXPCOMCallback();

    MOZ_ASSERT(callback);
    callback->HandleEvent(aPosition);
  }

  if (!mShutdown) {
    
    MOZ_ASSERT(mIsWatchPositionRequest,
               "non-shutdown getCurrentPosition request after callback!");
    SetTimeoutTimer();
  }
}

nsIPrincipal*
nsGeolocationRequest::GetPrincipal()
{
  if (!mLocator) {
    return nullptr;
  }
  return mLocator->GetPrincipal();
}

NS_IMETHODIMP
nsGeolocationRequest::Update(nsIDOMGeoPosition* aPosition)
{
  nsCOMPtr<nsIRunnable> ev = new RequestSendLocationEvent(aPosition, this);
  NS_DispatchToMainThread(ev);
  return NS_OK;
}

NS_IMETHODIMP
nsGeolocationRequest::NotifyError(uint16_t aErrorCode)
{
  MOZ_ASSERT(NS_IsMainThread());

  nsRefPtr<PositionError> positionError = new PositionError(mLocator, aErrorCode);
  positionError->NotifyCallback(mErrorCallback);
  return NS_OK;
}

void
nsGeolocationRequest::Shutdown()
{
  MOZ_ASSERT(!mShutdown, "request shutdown twice");
  mShutdown = true;

  if (mTimeoutTimer) {
    mTimeoutTimer->Cancel();
    mTimeoutTimer = nullptr;
  }

  
  
  if (mOptions && mOptions->mEnableHighAccuracy) {
    nsRefPtr<nsGeolocationService> gs = nsGeolocationService::GetGeolocationService();
    if (gs) {
      gs->SetHigherAccuracy(false);
    }
  }
}

bool nsGeolocationRequest::Recv__delete__(const bool& allow)
{
  if (allow) {
    (void) Allow();
  } else {
    (void) Cancel();
  }
  return true;
}



NS_INTERFACE_MAP_BEGIN(nsGeolocationService)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIGeolocationUpdate)
  NS_INTERFACE_MAP_ENTRY(nsIGeolocationUpdate)
  NS_INTERFACE_MAP_ENTRY(nsIObserver)
NS_INTERFACE_MAP_END

NS_IMPL_ADDREF(nsGeolocationService)
NS_IMPL_RELEASE(nsGeolocationService)


static bool sGeoEnabled = true;
static bool sGeoInitPending = true;
static int32_t sProviderTimeout = 6000; 

nsresult nsGeolocationService::Init()
{
  Preferences::AddIntVarCache(&sProviderTimeout, "geo.timeout", sProviderTimeout);
  Preferences::AddBoolVarCache(&sGeoEnabled, "geo.enabled", sGeoEnabled);

  if (!sGeoEnabled) {
    return NS_ERROR_FAILURE;
  }

  if (XRE_GetProcessType() == GeckoProcessType_Content) {
    sGeoInitPending = false;
    return NS_OK;
  }

  
  nsCOMPtr<nsISettingsService> settings =
    do_GetService("@mozilla.org/settingsService;1");

  if (settings) {
    nsCOMPtr<nsISettingsServiceLock> settingsLock;
    nsresult rv = settings->CreateLock(getter_AddRefs(settingsLock));
    NS_ENSURE_SUCCESS(rv, rv);

    nsRefPtr<GeolocationSettingsCallback> callback = new GeolocationSettingsCallback();
    rv = settingsLock->Get(GEO_SETINGS_ENABLED, callback);
    NS_ENSURE_SUCCESS(rv, rv);
  } else {
    
    
    sGeoInitPending = false;
  }

  
  nsCOMPtr<nsIObserverService> obs = services::GetObserverService();
  if (!obs) {
    return NS_ERROR_FAILURE;
  }

  obs->AddObserver(this, "quit-application", false);
  obs->AddObserver(this, "mozsettings-changed", false);

#ifdef MOZ_ENABLE_QTMOBILITY
  mProvider = new QTMLocationProvider();
#endif

#ifdef MOZ_WIDGET_ANDROID
  mProvider = new AndroidLocationProvider();
#endif

#ifdef MOZ_WIDGET_GONK
  mProvider = do_GetService(GONK_GPS_GEOLOCATION_PROVIDER_CONTRACTID);
#endif

#ifdef MOZ_WIDGET_COCOA
  if (Preferences::GetBool("geo.provider.use_corelocation", false)) {
    mProvider = new CoreLocationLocationProvider();
  }
#endif

  
  
  
  
  
  if (!mProvider || Preferences::GetBool("geo.provider.testing", false)) {
    nsCOMPtr<nsIGeolocationProvider> override =
      do_GetService(NS_GEOLOCATION_PROVIDER_CONTRACTID);

    if (override) {
      mProvider = override;
    }
  }

  return NS_OK;
}

nsGeolocationService::~nsGeolocationService()
{
}

void
nsGeolocationService::HandleMozsettingChanged(const PRUnichar* aData)
{
    
    

    AutoSafeJSContext cx;

    nsDependentString dataStr(aData);
    JS::Rooted<JS::Value> val(cx);
    if (!JS_ParseJSON(cx, dataStr.get(), dataStr.Length(), &val) || !val.isObject()) {
      return;
    }

    JS::Rooted<JSObject*> obj(cx, &val.toObject());
    JS::Rooted<JS::Value> key(cx);
    if (!JS_GetProperty(cx, obj, "key", &key) || !key.isString()) {
      return;
    }

    bool match;
    if (!JS_StringEqualsAscii(cx, key.toString(), GEO_SETINGS_ENABLED, &match) || !match) {
      return;
    }

    JS::Rooted<JS::Value> value(cx);
    if (!JS_GetProperty(cx, obj, "value", &value) || !value.isBoolean()) {
      return;
    }

    HandleMozsettingValue(value.toBoolean());
}

void
nsGeolocationService::HandleMozsettingValue(const bool aValue)
{
    if (!aValue) {
      
      StopDevice();
      Update(nullptr);
      mLastPosition = nullptr;
      sGeoEnabled = false;
    } else {
      sGeoEnabled = true;
    }

    if (sGeoInitPending) {
      sGeoInitPending = false;
      for (uint32_t i = 0, length = mGeolocators.Length(); i < length; ++i) {
        mGeolocators[i]->ServiceReady();
      }
    }
}

NS_IMETHODIMP
nsGeolocationService::Observe(nsISupports* aSubject,
                              const char* aTopic,
                              const PRUnichar* aData)
{
  if (!strcmp("quit-application", aTopic)) {
    nsCOMPtr<nsIObserverService> obs = services::GetObserverService();
    if (obs) {
      obs->RemoveObserver(this, "quit-application");
      obs->RemoveObserver(this, "mozsettings-changed");
    }

    for (uint32_t i = 0; i< mGeolocators.Length(); i++) {
      mGeolocators[i]->Shutdown();
    }
    StopDevice();

    return NS_OK;
  }

  if (!strcmp("mozsettings-changed", aTopic)) {
    HandleMozsettingChanged(aData);
    return NS_OK;
  }

  if (!strcmp("timer-callback", aTopic)) {
    
    for (uint32_t i = 0; i< mGeolocators.Length(); i++)
      if (mGeolocators[i]->HasActiveCallbacks()) {
        SetDisconnectTimer();
        return NS_OK;
      }

    
    StopDevice();
    Update(nullptr);
    return NS_OK;
  }

  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsGeolocationService::Update(nsIDOMGeoPosition *aSomewhere)
{
  SetCachedPosition(aSomewhere);

  for (uint32_t i = 0; i< mGeolocators.Length(); i++) {
    mGeolocators[i]->Update(aSomewhere);
  }
  return NS_OK;
}

NS_IMETHODIMP
nsGeolocationService::NotifyError(uint16_t aErrorCode)
{
  for (uint32_t i = 0; i < mGeolocators.Length(); i++) {
    mGeolocators[i]->NotifyError(aErrorCode);
  }

  return NS_OK;
}

void
nsGeolocationService::SetCachedPosition(nsIDOMGeoPosition* aPosition)
{
  mLastPosition = aPosition;
}

nsIDOMGeoPosition*
nsGeolocationService::GetCachedPosition()
{
  return mLastPosition;
}

nsresult
nsGeolocationService::StartDevice(nsIPrincipal *aPrincipal)
{
  if (!sGeoEnabled || sGeoInitPending) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  
  
  
  SetDisconnectTimer();

  if (XRE_GetProcessType() == GeckoProcessType_Content) {
    ContentChild* cpc = ContentChild::GetSingleton();
    cpc->SendAddGeolocationListener(IPC::Principal(aPrincipal),
                                    HighAccuracyRequested());
    return NS_OK;
  }

  
  nsCOMPtr<nsIObserverService> obs = services::GetObserverService();
  if (!obs) {
    return NS_ERROR_FAILURE;
  }

  if (!mProvider) {
    return NS_ERROR_FAILURE;
  }

  nsresult rv;

  if (NS_FAILED(rv = mProvider->Startup()) ||
      NS_FAILED(rv = mProvider->Watch(this))) {

    NotifyError(nsIDOMGeoPositionError::POSITION_UNAVAILABLE);
    return rv;
  }

  obs->NotifyObservers(mProvider,
                       "geolocation-device-events",
                       NS_LITERAL_STRING("starting").get());

  return NS_OK;
}

void
nsGeolocationService::SetDisconnectTimer()
{
  if (!mDisconnectTimer) {
    mDisconnectTimer = do_CreateInstance("@mozilla.org/timer;1");
  } else {
    mDisconnectTimer->Cancel();
  }

  mDisconnectTimer->Init(this,
                         sProviderTimeout,
                         nsITimer::TYPE_ONE_SHOT);
}

bool
nsGeolocationService::HighAccuracyRequested()
{
  for (uint32_t i = 0; i < mGeolocators.Length(); i++) {
    if (mGeolocators[i]->HighAccuracyRequested()) {
      return true;
    }
  }
  return false;
}

void
nsGeolocationService::SetHigherAccuracy(bool aEnable)
{
  bool highRequired = aEnable || HighAccuracyRequested();

  if (XRE_GetProcessType() == GeckoProcessType_Content) {
    ContentChild* cpc = ContentChild::GetSingleton();
    cpc->SendSetGeolocationHigherAccuracy(highRequired);
    return;
  }

  if (!mHigherAccuracy && highRequired) {
      mProvider->SetHighAccuracy(true);
  }

  if (mHigherAccuracy && !highRequired) {
      mProvider->SetHighAccuracy(false);
  }

  mHigherAccuracy = highRequired;
}

void
nsGeolocationService::StopDevice()
{
  if(mDisconnectTimer) {
    mDisconnectTimer->Cancel();
    mDisconnectTimer = nullptr;
  }

  if (XRE_GetProcessType() == GeckoProcessType_Content) {
    ContentChild* cpc = ContentChild::GetSingleton();
    cpc->SendRemoveGeolocationListener();
    return; 
  }

  nsCOMPtr<nsIObserverService> obs = services::GetObserverService();
  if (!obs) {
    return;
  }

  if (!mProvider) {
    return;
  }

  mHigherAccuracy = false;

  mProvider->Shutdown();
  obs->NotifyObservers(mProvider,
                       "geolocation-device-events",
                       NS_LITERAL_STRING("shutdown").get());
}

StaticRefPtr<nsGeolocationService> nsGeolocationService::sService;

already_AddRefed<nsGeolocationService>
nsGeolocationService::GetGeolocationService()
{
  nsRefPtr<nsGeolocationService> result;
  if (nsGeolocationService::sService) {
    result = nsGeolocationService::sService;
    return result.forget();
  }

  result = new nsGeolocationService();
  if (NS_FAILED(result->Init())) {
    return nullptr;
  }
  ClearOnShutdown(&nsGeolocationService::sService);
  nsGeolocationService::sService = result;
  return result.forget();
}

void
nsGeolocationService::AddLocator(Geolocation* aLocator)
{
  mGeolocators.AppendElement(aLocator);
}

void
nsGeolocationService::RemoveLocator(Geolocation* aLocator)
{
  mGeolocators.RemoveElement(aLocator);
}





NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(Geolocation)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDOMGeoGeolocation)
  NS_INTERFACE_MAP_ENTRY(nsIDOMGeoGeolocation)
  NS_INTERFACE_MAP_ENTRY(nsIGeolocationUpdate)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(Geolocation)
NS_IMPL_CYCLE_COLLECTING_RELEASE(Geolocation)

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_4(Geolocation,
                                        mCachedPosition,
                                        mPendingCallbacks,
                                        mWatchingCallbacks,
                                        mPendingRequests)

Geolocation::Geolocation()
: mLastWatchId(0)
{
  SetIsDOMBinding();
}

Geolocation::~Geolocation()
{
  if (mService) {
    Shutdown();
  }
}

nsresult
Geolocation::Init(nsIDOMWindow* aContentDom)
{
  
  if (aContentDom) {
    nsCOMPtr<nsPIDOMWindow> window = do_QueryInterface(aContentDom);
    if (!window) {
      return NS_ERROR_FAILURE;
    }

    mOwner = do_GetWeakReference(window->GetCurrentInnerWindow());
    if (!mOwner) {
      return NS_ERROR_FAILURE;
    }

    
    nsCOMPtr<nsIDocument> doc = window->GetDoc();
    if (!doc) {
      return NS_ERROR_FAILURE;
    }

    mPrincipal = doc->NodePrincipal();
  }

  
  
  
  mService = nsGeolocationService::GetGeolocationService();
  if (mService) {
    mService->AddLocator(this);
  }
  return NS_OK;
}

void
Geolocation::Shutdown()
{
  
  mPendingCallbacks.Clear();
  mWatchingCallbacks.Clear();

  if (mService) {
    mService->RemoveLocator(this);
  }

  mService = nullptr;
  mPrincipal = nullptr;
}

nsIDOMWindow*
Geolocation::GetParentObject() const {
  nsCOMPtr<nsPIDOMWindow> window = do_QueryReferent(mOwner);
  return window.get();
}

bool
Geolocation::HasActiveCallbacks()
{
  return mPendingCallbacks.Length() || mWatchingCallbacks.Length();
}

bool
Geolocation::HighAccuracyRequested()
{
  for (uint32_t i = 0; i < mWatchingCallbacks.Length(); i++) {
    if (mWatchingCallbacks[i]->WantsHighAccuracy()) {
      return true;
    }
  }

  for (uint32_t i = 0; i < mPendingCallbacks.Length(); i++) {
    if (mPendingCallbacks[i]->WantsHighAccuracy()) {
      return true;
    }
  }

  return false;
}

void
Geolocation::RemoveRequest(nsGeolocationRequest* aRequest)
{
  bool requestWasKnown =
    (mPendingCallbacks.RemoveElement(aRequest) !=
     mWatchingCallbacks.RemoveElement(aRequest));

  unused << requestWasKnown;
}

NS_IMETHODIMP
Geolocation::Update(nsIDOMGeoPosition *aSomewhere)
{
  if (!WindowOwnerStillExists()) {
    Shutdown();
    return NS_OK;
  }

  for (uint32_t i = mPendingCallbacks.Length(); i > 0; i--) {
    mPendingCallbacks[i-1]->Update(aSomewhere);
    RemoveRequest(mPendingCallbacks[i-1]);
  }

  
  for (uint32_t i = 0; i < mWatchingCallbacks.Length(); i++) {
    mWatchingCallbacks[i]->Update(aSomewhere);
  }

  return NS_OK;
}

NS_IMETHODIMP
Geolocation::NotifyError(uint16_t aErrorCode)
{
  if (!WindowOwnerStillExists()) {
    Shutdown();
    return NS_OK;
  }

  for (uint32_t i = mPendingCallbacks.Length(); i > 0; i--) {
    mPendingCallbacks[i-1]->NotifyError(aErrorCode);
    RemoveRequest(mPendingCallbacks[i-1]);
  }

  
  for (uint32_t i = 0; i < mWatchingCallbacks.Length(); i++) {
    mWatchingCallbacks[i]->NotifyError(aErrorCode);
  }

  return NS_OK;
}

void
Geolocation::SetCachedPosition(Position* aPosition)
{
  mCachedPosition = aPosition;
}

Position*
Geolocation::GetCachedPosition()
{
  return mCachedPosition;
}

void
Geolocation::GetCurrentPosition(PositionCallback& aCallback,
                                PositionErrorCallback* aErrorCallback,
                                const PositionOptions& aOptions,
                                ErrorResult& aRv)
{
  GeoPositionCallback successCallback(&aCallback);
  GeoPositionErrorCallback errorCallback(aErrorCallback);

  nsresult rv = GetCurrentPosition(successCallback, errorCallback,
                                   CreatePositionOptionsCopy(aOptions));

  if (NS_FAILED(rv)) {
    aRv.Throw(rv);
  }

  return;
}

NS_IMETHODIMP
Geolocation::GetCurrentPosition(nsIDOMGeoPositionCallback* aCallback,
                                nsIDOMGeoPositionErrorCallback* aErrorCallback,
                                PositionOptions* aOptions)
{
  NS_ENSURE_ARG_POINTER(aCallback);

  GeoPositionCallback successCallback(aCallback);
  GeoPositionErrorCallback errorCallback(aErrorCallback);

  return GetCurrentPosition(successCallback, errorCallback, aOptions);
}

nsresult
Geolocation::GetCurrentPosition(GeoPositionCallback& callback,
                                GeoPositionErrorCallback& errorCallback,
                                PositionOptions *options)
{
  if (mPendingCallbacks.Length() > MAX_GEO_REQUESTS_PER_WINDOW) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  nsRefPtr<nsGeolocationRequest> request = new nsGeolocationRequest(this,
                                                                    callback,
                                                                    errorCallback,
                                                                    options,
                                                                    false);

  if (!sGeoEnabled) {
    nsCOMPtr<nsIRunnable> ev = new RequestAllowEvent(false, request);
    NS_DispatchToMainThread(ev);
    return NS_OK;
  }

  if (!mOwner && !nsContentUtils::IsCallerChrome()) {
    return NS_ERROR_FAILURE;
  }

  if (sGeoInitPending) {
    mPendingRequests.AppendElement(request);
    return NS_OK;
  }

  return GetCurrentPositionReady(request);
}

nsresult
Geolocation::GetCurrentPositionReady(nsGeolocationRequest* aRequest)
{
  if (mOwner) {
    if (!RegisterRequestWithPrompt(aRequest)) {
      return NS_ERROR_NOT_AVAILABLE;
    }

    return NS_OK;
  }

  if (!nsContentUtils::IsCallerChrome()) {
    return NS_ERROR_FAILURE;
  }

  nsCOMPtr<nsIRunnable> ev = new RequestAllowEvent(true, aRequest);
  NS_DispatchToMainThread(ev);

  return NS_OK;
}

int32_t
Geolocation::WatchPosition(PositionCallback& aCallback,
                           PositionErrorCallback* aErrorCallback,
                           const PositionOptions& aOptions,
                           ErrorResult& aRv)
{
  int32_t ret;
  GeoPositionCallback successCallback(&aCallback);
  GeoPositionErrorCallback errorCallback(aErrorCallback);

  nsresult rv = WatchPosition(successCallback, errorCallback,
                              CreatePositionOptionsCopy(aOptions), &ret);

  if (NS_FAILED(rv)) {
    aRv.Throw(rv);
  }

  return ret;
}

NS_IMETHODIMP
Geolocation::WatchPosition(nsIDOMGeoPositionCallback *aCallback,
                           nsIDOMGeoPositionErrorCallback *aErrorCallback,
                           PositionOptions *aOptions,
                           int32_t* aRv)
{
  NS_ENSURE_ARG_POINTER(aCallback);

  GeoPositionCallback successCallback(aCallback);
  GeoPositionErrorCallback errorCallback(aErrorCallback);

  return WatchPosition(successCallback, errorCallback, aOptions, aRv);
}

nsresult
Geolocation::WatchPosition(GeoPositionCallback& aCallback,
                           GeoPositionErrorCallback& aErrorCallback,
                           PositionOptions* aOptions,
                           int32_t* aRv)
{
  if (mWatchingCallbacks.Length() > MAX_GEO_REQUESTS_PER_WINDOW) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  
  *aRv = mLastWatchId++;

  nsRefPtr<nsGeolocationRequest> request = new nsGeolocationRequest(this,
                                                                    aCallback,
                                                                    aErrorCallback,
                                                                    aOptions,
                                                                    true,
                                                                    *aRv);

  if (!sGeoEnabled) {
    nsCOMPtr<nsIRunnable> ev = new RequestAllowEvent(false, request);
    NS_DispatchToMainThread(ev);
    return NS_OK;
  }

  if (!mOwner && !nsContentUtils::IsCallerChrome()) {
    return NS_ERROR_FAILURE;
  }

  if (sGeoInitPending) {
    mPendingRequests.AppendElement(request);
    return NS_OK;
  }

  return WatchPositionReady(request);
}

nsresult
Geolocation::WatchPositionReady(nsGeolocationRequest* aRequest)
{
  if (mOwner) {
    if (!RegisterRequestWithPrompt(aRequest))
      return NS_ERROR_NOT_AVAILABLE;

    return NS_OK;
  }

  if (!nsContentUtils::IsCallerChrome()) {
    return NS_ERROR_FAILURE;
  }

  aRequest->Allow();

  return NS_OK;
}

NS_IMETHODIMP
Geolocation::ClearWatch(int32_t aWatchId)
{
  if (aWatchId < 0) {
    return NS_OK;
  }

  for (uint32_t i = 0, length = mWatchingCallbacks.Length(); i < length; ++i) {
    if (mWatchingCallbacks[i]->WatchId() == aWatchId) {
      mWatchingCallbacks[i]->Shutdown();
      RemoveRequest(mWatchingCallbacks[i]);
      break;
    }
  }

  
  
  for (uint32_t i = 0, length = mPendingRequests.Length(); i < length; ++i) {
    if (mPendingRequests[i]->IsWatch() &&
        (mPendingRequests[i]->WatchId() == aWatchId)) {
      mPendingRequests[i]->Shutdown();
      mPendingRequests.RemoveElementAt(i);
      break;
    }
  }

  return NS_OK;
}

void
Geolocation::ServiceReady()
{
  for (uint32_t length = mPendingRequests.Length(); length > 0; --length) {
    if (mPendingRequests[0]->IsWatch()) {
      WatchPositionReady(mPendingRequests[0]);
    } else {
      GetCurrentPositionReady(mPendingRequests[0]);
    }

    mPendingRequests.RemoveElementAt(0);
  }
}

bool
Geolocation::WindowOwnerStillExists()
{
  
  
  
  if (mOwner == nullptr) {
    return true;
  }

  nsCOMPtr<nsPIDOMWindow> window = do_QueryReferent(mOwner);

  if (window) {
    bool closed = false;
    window->GetClosed(&closed);
    if (closed) {
      return false;
    }

    nsPIDOMWindow* outer = window->GetOuterWindow();
    if (!outer || outer->GetCurrentInnerWindow() != window) {
      return false;
    }
  }

  return true;
}

void
Geolocation::NotifyAllowedRequest(nsGeolocationRequest* aRequest)
{
  if (aRequest->IsWatch()) {
    mWatchingCallbacks.AppendElement(aRequest);
  } else {
    mPendingCallbacks.AppendElement(aRequest);
  }
}

bool
Geolocation::RegisterRequestWithPrompt(nsGeolocationRequest* request)
{
  if (Preferences::GetBool("geo.prompt.testing", false)) {
    bool allow = Preferences::GetBool("geo.prompt.testing.allow", false);
    nsCOMPtr<nsIRunnable> ev = new RequestAllowEvent(allow,
						     request);
    NS_DispatchToMainThread(ev);
    return true;
  }

  if (XRE_GetProcessType() == GeckoProcessType_Content) {
    nsCOMPtr<nsPIDOMWindow> window = do_QueryReferent(mOwner);
    if (!window) {
      return true;
    }

    
    
    TabChild* child = TabChild::GetFrom(window->GetDocShell());
    if (!child) {
      return false;
    }

    
    
    request->AddRef();
    child->SendPContentPermissionRequestConstructor(request,
                                                    NS_LITERAL_CSTRING("geolocation"),
                                                    NS_LITERAL_CSTRING("unused"),
                                                    IPC::Principal(mPrincipal));

    request->Sendprompt();
    return true;
  }

  nsCOMPtr<nsIRunnable> ev  = new RequestPromptEvent(request);
  NS_DispatchToMainThread(ev);
  return true;
}

JSObject*
Geolocation::WrapObject(JSContext *aCtx, JS::Handle<JSObject*> aScope)
{
  return GeolocationBinding::Wrap(aCtx, aScope, this);
}
