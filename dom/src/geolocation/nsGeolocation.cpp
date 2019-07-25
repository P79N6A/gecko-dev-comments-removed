



#include "nsContentPermissionHelper.h"
#include "nsXULAppAPI.h"

#include "mozilla/dom/PBrowserChild.h"
#include "mozilla/dom/PBrowserParent.h"
#include "mozilla/dom/ContentChild.h"
#include "nsNetUtil.h"

#include "nsFrameManager.h"
#include "nsFrameLoader.h"
#include "nsIFrameLoader.h"

#include "nsIDocShellTreeOwner.h"
#include "nsIDocShellTreeItem.h"
#include "nsIWebProgressListener2.h"

#include "nsDOMEventTargetHelper.h"
#include "TabChild.h"

#include "nsGeolocation.h"
#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsIDOMWindow.h"
#include "nsDOMClassInfoID.h"
#include "nsComponentManagerUtils.h"
#include "nsICategoryManager.h"
#include "nsISupportsPrimitives.h"
#include "nsServiceManagerUtils.h"
#include "nsContentUtils.h"
#include "nsIURI.h"
#include "nsIPermissionManager.h"
#include "nsIObserverService.h"
#include "nsIJSContextStack.h"
#include "nsThreadUtils.h"
#include "mozilla/Services.h"
#include "mozilla/unused.h"
#include "mozilla/Preferences.h"
#include "mozilla/Attributes.h"

#include <math.h>

#ifdef MOZ_MAEMO_LIBLOCATION
#include "MaemoLocationProvider.h"
#endif

#ifdef MOZ_ENABLE_QTMOBILITY
#include "QTMLocationProvider.h"
#endif

#ifdef MOZ_WIDGET_ANDROID
#include "AndroidLocationProvider.h"
#endif

#ifdef MOZ_WIDGET_GONK
#include "GonkGPSGeolocationProvider.h"
#endif

#include "nsIDOMDocument.h"
#include "nsIDocument.h"



#define MAX_GEO_REQUESTS_PER_WINDOW  1500

using mozilla::unused;          
using namespace mozilla;
using namespace mozilla::dom;

class RequestPromptEvent : public nsRunnable
{
public:
  RequestPromptEvent(nsGeolocationRequest* request)
    : mRequest(request)
  {
  }

  NS_IMETHOD Run() {
    nsCOMPtr<nsIContentPermissionPrompt> prompt = do_GetService(NS_CONTENT_PERMISSION_PROMPT_CONTRACTID);
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
    if (mAllow)
      mRequest->Allow();
    else
      mRequest->Cancel();
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
                           nsGeolocationRequest* aRequest,
                           nsGeolocation* aLocator)
    : mPosition(aPosition),
      mRequest(aRequest),
      mLocator(aLocator)
  {
  }

  NS_IMETHOD Run() {
    mRequest->SendLocation(mPosition);
    if (mLocator)
      mLocator->RemoveRequest(mRequest);
    return NS_OK;
  }

private:
  nsCOMPtr<nsIDOMGeoPosition>    mPosition;
  nsRefPtr<nsGeolocationRequest> mRequest;

  nsRefPtr<nsGeolocation>        mLocator;
};





class nsDOMGeoPositionError MOZ_FINAL : public nsIDOMGeoPositionError
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMGEOPOSITIONERROR

  nsDOMGeoPositionError(PRInt16 aCode);
  void NotifyCallback(nsIDOMGeoPositionErrorCallback* callback);

private:
  ~nsDOMGeoPositionError();
  PRInt16 mCode;
};

DOMCI_DATA(GeoPositionError, nsDOMGeoPositionError)

NS_INTERFACE_MAP_BEGIN(nsDOMGeoPositionError)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDOMGeoPositionError)
  NS_INTERFACE_MAP_ENTRY(nsIDOMGeoPositionError)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(GeoPositionError)
NS_INTERFACE_MAP_END

NS_IMPL_THREADSAFE_ADDREF(nsDOMGeoPositionError)
NS_IMPL_THREADSAFE_RELEASE(nsDOMGeoPositionError)

nsDOMGeoPositionError::nsDOMGeoPositionError(PRInt16 aCode)
  : mCode(aCode)
{
}

nsDOMGeoPositionError::~nsDOMGeoPositionError(){}


NS_IMETHODIMP
nsDOMGeoPositionError::GetCode(PRInt16 *aCode)
{
  NS_ENSURE_ARG_POINTER(aCode);
  *aCode = mCode;
  return NS_OK;
}

void
nsDOMGeoPositionError::NotifyCallback(nsIDOMGeoPositionErrorCallback* aCallback)
{
  if (!aCallback)
    return;
  
  
  nsCOMPtr<nsIJSContextStack> stack(do_GetService("@mozilla.org/js/xpc/ContextStack;1"));
  if (!stack || NS_FAILED(stack->Push(nsnull)))
    return;

  nsAutoMicroTask mt;
  aCallback->HandleEvent(this);
  
  
  JSContext* cx;
  stack->Pop(&cx);
}




nsGeolocationRequest::nsGeolocationRequest(nsGeolocation* aLocator,
                                           nsIDOMGeoPositionCallback* aCallback,
                                           nsIDOMGeoPositionErrorCallback* aErrorCallback,
                                           bool aWatchPositionRequest)
  : mAllowed(false),
    mCleared(false),
    mIsWatchPositionRequest(aWatchPositionRequest),
    mCallback(aCallback),
    mErrorCallback(aErrorCallback),
    mLocator(aLocator)
{
}

nsGeolocationRequest::~nsGeolocationRequest()
{
}

nsresult
nsGeolocationRequest::Init(JSContext* aCx, const jsval& aOptions)
{
  if (aCx && !JSVAL_IS_VOID(aOptions) && !JSVAL_IS_NULL(aOptions)) {
    mOptions = new mozilla::dom::GeoPositionOptions();
    nsresult rv = mOptions->Init(aCx, &aOptions);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  return NS_OK;
}

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsGeolocationRequest)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIContentPermissionRequest)
  NS_INTERFACE_MAP_ENTRY(nsIContentPermissionRequest)
  NS_INTERFACE_MAP_ENTRY(nsITimerCallback)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsGeolocationRequest)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsGeolocationRequest)

NS_IMPL_CYCLE_COLLECTION_3(nsGeolocationRequest, mCallback, mErrorCallback, mLocator)


void
nsGeolocationRequest::NotifyError(PRInt16 errorCode)
{
  nsRefPtr<nsDOMGeoPositionError> positionError = new nsDOMGeoPositionError(errorCode);
  if (!positionError)
    return;
  
  positionError->NotifyCallback(mErrorCallback);
}


NS_IMETHODIMP
nsGeolocationRequest::Notify(nsITimer* aTimer)
{
  
  
  
  
  
  mLocator->RemoveRequest(this);
  NotifyError(nsIDOMGeoPositionError::TIMEOUT);

  mTimeoutTimer = nsnull;
  return NS_OK;
}
 
NS_IMETHODIMP
nsGeolocationRequest::GetUri(nsIURI * *aRequestingURI)
{
  NS_ENSURE_ARG_POINTER(aRequestingURI);

  nsCOMPtr<nsIURI> uri = mLocator->GetURI();
  uri.forget(aRequestingURI);

  return NS_OK;
}

NS_IMETHODIMP
nsGeolocationRequest::GetType(nsACString & aType)
{
  aType = "geolocation";
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
  *aRequestingElement = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
nsGeolocationRequest::Cancel()
{
  
  mLocator->RemoveRequest(this);

  NotifyError(nsIDOMGeoPositionError::PERMISSION_DENIED);
  return NS_OK;
}

NS_IMETHODIMP
nsGeolocationRequest::Allow()
{
  nsRefPtr<nsGeolocationService> geoService = nsGeolocationService::GetInstance();

  
  nsresult rv = geoService->StartDevice();
  
  if (NS_FAILED(rv)) {
    
    NotifyError(nsIDOMGeoPositionError::POSITION_UNAVAILABLE);
    return NS_OK;
  }
  
  nsCOMPtr<nsIDOMGeoPosition> lastPosition = geoService->GetCachedPosition();
  DOMTimeStamp cachedPositionTime;
  if (lastPosition)
    lastPosition->GetTimestamp(&cachedPositionTime);

  
  
  
  
  
  
  
  PRUint32 maximumAge = 30 * PR_MSEC_PER_SEC;
  if (mOptions) {
    if (mOptions->maximumAge >= 0) {
      maximumAge = mOptions->maximumAge;
    }
    if (mOptions->enableHighAccuracy) {
      geoService->SetHigherAccuracy(true);
    }
  }

  if (lastPosition && maximumAge > 0 &&
      ( PRTime(PR_Now() / PR_USEC_PER_MSEC) - maximumAge <=
        PRTime(cachedPositionTime) )) {
    
    mAllowed = true;
    
     nsCOMPtr<nsIRunnable> ev =
         new RequestSendLocationEvent(lastPosition, this,
                                      mIsWatchPositionRequest ? nsnull : mLocator);
    NS_DispatchToMainThread(ev);
  }

  SetTimeoutTimer();

  mAllowed = true;
  return NS_OK;
}

void
nsGeolocationRequest::SetTimeoutTimer()
{
  if (mTimeoutTimer) {
    mTimeoutTimer->Cancel();
    mTimeoutTimer = nsnull;
  }
  PRInt32 timeout;
  if (mOptions && (timeout = mOptions->timeout) != 0) {

    if (timeout < 0)
      timeout = 0;
    else if (timeout < 10)
      timeout = 10;

    mTimeoutTimer = do_CreateInstance("@mozilla.org/timer;1");
    mTimeoutTimer->InitWithCallback(this, timeout, nsITimer::TYPE_ONE_SHOT);
  }
}

void
nsGeolocationRequest::MarkCleared()
{
  if (mTimeoutTimer) {
    mTimeoutTimer->Cancel();
    mTimeoutTimer = nsnull;
  }
  mCleared = true;
}

void
nsGeolocationRequest::SendLocation(nsIDOMGeoPosition* aPosition)
{
  if (mCleared || !mAllowed)
    return;

  if (mTimeoutTimer) {
    mTimeoutTimer->Cancel();
    mTimeoutTimer = nsnull;
  }

  
  if (!aPosition) {
    NotifyError(nsIDOMGeoPositionError::POSITION_UNAVAILABLE);
    return;
  }

  
  nsCOMPtr<nsIJSContextStack> stack(do_GetService("@mozilla.org/js/xpc/ContextStack;1"));
  if (!stack || NS_FAILED(stack->Push(nsnull)))
    return; 

  nsAutoMicroTask mt;
  mCallback->HandleEvent(aPosition);

  
  JSContext* cx;
  stack->Pop(&cx);

  if (mIsWatchPositionRequest)
    SetTimeoutTimer();
}

bool
nsGeolocationRequest::Update(nsIDOMGeoPosition* aPosition)
{
  if (!mAllowed)
    return false;
  nsCOMPtr<nsIRunnable> ev  =
      new RequestSendLocationEvent(aPosition, this,
                                   mIsWatchPositionRequest ? nsnull : mLocator);
  NS_DispatchToMainThread(ev);
  return true;
}

void
nsGeolocationRequest::Shutdown()
{
  if (mOptions && mOptions->enableHighAccuracy) {
    nsRefPtr<nsGeolocationService> geoService = nsGeolocationService::GetInstance();
    if (geoService) {
      geoService->SetHigherAccuracy(false);
    }
  }

  if (mTimeoutTimer) {
    mTimeoutTimer->Cancel();
    mTimeoutTimer = nsnull;
  }
  mCleared = true;
  mCallback = nsnull;
  mErrorCallback = nsnull;
}

bool nsGeolocationRequest::Recv__delete__(const bool& allow)
{
  if (allow)
    (void) Allow();
  else
    (void) Cancel();
  return true;
}



NS_INTERFACE_MAP_BEGIN(nsGeolocationService)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIGeolocationUpdate)
  NS_INTERFACE_MAP_ENTRY(nsIGeolocationUpdate)
  NS_INTERFACE_MAP_ENTRY(nsIObserver)
NS_INTERFACE_MAP_END

NS_IMPL_THREADSAFE_ADDREF(nsGeolocationService)
NS_IMPL_THREADSAFE_RELEASE(nsGeolocationService)


static bool sGeoEnabled = true;
static bool sGeoIgnoreLocationFilter = false;
static PRInt32 sProviderTimeout = 6000; 

nsresult nsGeolocationService::Init()
{
  Preferences::AddIntVarCache(&sProviderTimeout, "geo.timeout", sProviderTimeout);
  Preferences::AddBoolVarCache(&sGeoEnabled, "geo.enabled", sGeoEnabled);
  Preferences::AddBoolVarCache(&sGeoIgnoreLocationFilter, "geo.ignore.location_filter", sGeoIgnoreLocationFilter);

  if (!sGeoEnabled)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIGeolocationProvider> provider = do_GetService(NS_GEOLOCATION_PROVIDER_CONTRACTID);
  if (provider)
    mProviders.AppendObject(provider);

  
  nsCOMPtr<nsICategoryManager> catMan(do_GetService("@mozilla.org/categorymanager;1"));
  if (!catMan)
    return NS_ERROR_FAILURE;

  
  nsCOMPtr<nsIObserverService> obs = mozilla::services::GetObserverService();
  if (!obs)
    return NS_ERROR_FAILURE;

  obs->AddObserver(this, "quit-application", false);

  nsCOMPtr<nsISimpleEnumerator> geoproviders;
  catMan->EnumerateCategory("geolocation-provider", getter_AddRefs(geoproviders));
  if (geoproviders) {

    bool hasMore;
    while (NS_SUCCEEDED(geoproviders->HasMoreElements(&hasMore)) && hasMore) {
      nsCOMPtr<nsISupports> elem;
      geoproviders->GetNext(getter_AddRefs(elem));

      nsCOMPtr<nsISupportsCString> elemString = do_QueryInterface(elem);
      
      nsCAutoString name;
      elemString->GetData(name);

      nsXPIDLCString spec;
      catMan->GetCategoryEntry("geolocation-provider", name.get(), getter_Copies(spec));

      provider = do_GetService(spec);
      if (provider)
        mProviders.AppendObject(provider);
    }
  }

  

#ifdef MOZ_MAEMO_LIBLOCATION
  provider = new MaemoLocationProvider();
  if (provider)
    mProviders.AppendObject(provider);
#endif

#ifdef MOZ_ENABLE_QTMOBILITY
  provider = new QTMLocationProvider();
  if (provider)
    mProviders.AppendObject(provider);
#endif

#ifdef MOZ_WIDGET_ANDROID
  provider = new AndroidLocationProvider();
  if (provider)
    mProviders.AppendObject(provider);
#endif

#ifdef MOZ_WIDGET_GONK
  provider = GonkGPSGeolocationProvider::GetSingleton();
  if (provider)
    mProviders.AppendObject(provider);
#endif

  return NS_OK;
}

nsGeolocationService::~nsGeolocationService()
{
}

NS_IMETHODIMP
nsGeolocationService::Observe(nsISupports* aSubject,
                              const char* aTopic,
                              const PRUnichar* aData)
{
  if (!strcmp("quit-application", aTopic))
  {
    nsCOMPtr<nsIObserverService> obs = mozilla::services::GetObserverService();
    if (obs) {
      obs->RemoveObserver(this, "quit-application");
    }

    for (PRUint32 i = 0; i< mGeolocators.Length(); i++)
      mGeolocators[i]->Shutdown();

    StopDevice();

    return NS_OK;
  }
  
  if (!strcmp("timer-callback", aTopic))
  {
    
    for (PRUint32 i = 0; i< mGeolocators.Length(); i++)
      if (mGeolocators[i]->HasActiveCallbacks())
      {
        SetDisconnectTimer();
        return NS_OK;
      }
    
    
    StopDevice();
    Update(nsnull);
    return NS_OK;
  }

  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsGeolocationService::Update(nsIDOMGeoPosition *aSomewhere)
{
  SetCachedPosition(aSomewhere);

  for (PRUint32 i = 0; i< mGeolocators.Length(); i++)
    mGeolocators[i]->Update(aSomewhere);
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
nsGeolocationService::StartDevice()
{
  if (!sGeoEnabled)
    return NS_ERROR_NOT_AVAILABLE;

  
  
  
  SetDisconnectTimer();

  if (XRE_GetProcessType() == GeckoProcessType_Content) {
    ContentChild* cpc = ContentChild::GetSingleton();
    cpc->SendAddGeolocationListener();
    return NS_OK;
  }

  
  nsCOMPtr<nsIObserverService> obs = mozilla::services::GetObserverService();
  if (!obs)
    return NS_ERROR_FAILURE;

  for (PRInt32 i = 0; i < mProviders.Count(); i++) {
    mProviders[i]->Startup();
    mProviders[i]->Watch(this);
    obs->NotifyObservers(mProviders[i],
                         "geolocation-device-events",
                         NS_LITERAL_STRING("starting").get());
  }

  return NS_OK;
}

void
nsGeolocationService::SetDisconnectTimer()
{
  if (!mDisconnectTimer)
    mDisconnectTimer = do_CreateInstance("@mozilla.org/timer;1");
  else
    mDisconnectTimer->Cancel();

  mDisconnectTimer->Init(this,
                         sProviderTimeout,
                         nsITimer::TYPE_ONE_SHOT);
}

void
nsGeolocationService::SetHigherAccuracy(bool aEnable)
{
    if (!mHigherAccuracy && aEnable) {
	  for (PRInt32 i = 0; i < mProviders.Count(); i++) {
	    mProviders[i]->SetHighAccuracy(true);
	  }
    }
	
    if (mHigherAccuracy && !aEnable) {
	  for (PRInt32 i = 0; i < mProviders.Count(); i++) {
	    mProviders[i]->SetHighAccuracy(false);
	  }
    }

    mHigherAccuracy = aEnable;
}

void 
nsGeolocationService::StopDevice()
{
  if(mDisconnectTimer) {
    mDisconnectTimer->Cancel();
    mDisconnectTimer = nsnull;
  }

  if (XRE_GetProcessType() == GeckoProcessType_Content) {
    ContentChild* cpc = ContentChild::GetSingleton();
    cpc->SendRemoveGeolocationListener();
    return; 
  }

  nsCOMPtr<nsIObserverService> obs = mozilla::services::GetObserverService();
  if (!obs)
    return;

  for (PRInt32 i = 0; i < mProviders.Count(); i++) {
    mProviders[i]->Shutdown();
    obs->NotifyObservers(mProviders[i],
                         "geolocation-device-events",
                         NS_LITERAL_STRING("shutdown").get());
  }
}

nsGeolocationService* nsGeolocationService::gService = nsnull;

nsGeolocationService*
nsGeolocationService::GetInstance()
{
  if (!nsGeolocationService::gService) {
    nsGeolocationService::gService = new nsGeolocationService();
    NS_ASSERTION(nsGeolocationService::gService, "null nsGeolocationService.");

    if (nsGeolocationService::gService) {
      if (NS_FAILED(nsGeolocationService::gService->Init())) {
        delete nsGeolocationService::gService;
        nsGeolocationService::gService = nsnull;
      }        
    }
  }
  return nsGeolocationService::gService;
}

nsGeolocationService*
nsGeolocationService::GetGeolocationService()
{
  nsGeolocationService* inst = nsGeolocationService::GetInstance();
  NS_IF_ADDREF(inst);
  return inst;
}

void
nsGeolocationService::AddLocator(nsGeolocation* aLocator)
{
  mGeolocators.AppendElement(aLocator);
}

void
nsGeolocationService::RemoveLocator(nsGeolocation* aLocator)
{
  mGeolocators.RemoveElement(aLocator);
}





DOMCI_DATA(GeoGeolocation, nsGeolocation)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsGeolocation)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDOMGeoGeolocation)
  NS_INTERFACE_MAP_ENTRY(nsIDOMGeoGeolocation)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(GeoGeolocation)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsGeolocation)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsGeolocation)
NS_IMPL_CYCLE_COLLECTION_CLASS(nsGeolocation)

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(nsGeolocation)
  tmp->mPendingCallbacks.Clear();
  tmp->mWatchingCallbacks.Clear();
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(nsGeolocation)
  PRUint32 i; 
  for (i = 0; i < tmp->mPendingCallbacks.Length(); ++i)
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR_AMBIGUOUS(mPendingCallbacks[i], nsIContentPermissionRequest)

  for (i = 0; i < tmp->mWatchingCallbacks.Length(); ++i)
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR_AMBIGUOUS(mWatchingCallbacks[i], nsIContentPermissionRequest)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

nsGeolocation::nsGeolocation() 
{
}

nsGeolocation::~nsGeolocation()
{
  if (mService)
    Shutdown();
}

nsresult
nsGeolocation::Init(nsIDOMWindow* aContentDom)
{
  
  if (aContentDom) {
    nsCOMPtr<nsPIDOMWindow> window = do_QueryInterface(aContentDom);
    if (!window)
      return NS_ERROR_FAILURE;

    mOwner = do_GetWeakReference(window->GetCurrentInnerWindow());
    if (!mOwner)
      return NS_ERROR_FAILURE;

    
    nsCOMPtr<nsIDOMDocument> domdoc;
    aContentDom->GetDocument(getter_AddRefs(domdoc));
    nsCOMPtr<nsIDocument> doc = do_QueryInterface(domdoc);
    if (!doc)
      return NS_ERROR_FAILURE;

    doc->NodePrincipal()->GetURI(getter_AddRefs(mURI));
    
    if (!mURI)
      return NS_ERROR_FAILURE;
  }

  
  
  
  mService = nsGeolocationService::GetInstance();
  if (mService)
    mService->AddLocator(this);

  return NS_OK;
}

void
nsGeolocation::Shutdown()
{
  
  for (PRUint32 i = 0; i< mPendingCallbacks.Length(); i++)
    mPendingCallbacks[i]->Shutdown();
  mPendingCallbacks.Clear();

  for (PRUint32 i = 0; i< mWatchingCallbacks.Length(); i++)
    mWatchingCallbacks[i]->Shutdown();
  mWatchingCallbacks.Clear();

  if (mService)
    mService->RemoveLocator(this);

  mService = nsnull;
  mURI = nsnull;
}

bool
nsGeolocation::HasActiveCallbacks()
{
  for (PRUint32 i = 0; i < mWatchingCallbacks.Length(); i++)
    if (mWatchingCallbacks[i]->IsActive())
      return true;

  return mPendingCallbacks.Length() != 0;
}

void
nsGeolocation::RemoveRequest(nsGeolocationRequest* aRequest)
{
  mPendingCallbacks.RemoveElement(aRequest);

  
  
  
  
  

  aRequest->MarkCleared();
}

void
nsGeolocation::Update(nsIDOMGeoPosition *aSomewhere)
{
  if (!WindowOwnerStillExists())
    return Shutdown();

  for (PRUint32 i = mPendingCallbacks.Length(); i> 0; i--) {
    if (mPendingCallbacks[i-1]->Update(aSomewhere))
      mPendingCallbacks.RemoveElementAt(i-1);
  }

  
  for (PRUint32 i = 0; i< mWatchingCallbacks.Length(); i++) {
    mWatchingCallbacks[i]->Update(aSomewhere);
  }
}

NS_IMETHODIMP
nsGeolocation::GetCurrentPosition(nsIDOMGeoPositionCallback *callback,
                                  nsIDOMGeoPositionErrorCallback *errorCallback,
                                  const jsval& options,
                                  JSContext* cx)
{
  NS_ENSURE_ARG_POINTER(callback);

  if (!sGeoEnabled)
    return NS_ERROR_NOT_AVAILABLE;

  if (mPendingCallbacks.Length() > MAX_GEO_REQUESTS_PER_WINDOW)
    return NS_ERROR_NOT_AVAILABLE;

  nsRefPtr<nsGeolocationRequest> request = new nsGeolocationRequest(this,
								    callback,
								    errorCallback,
								    false);
  if (!request)
    return NS_ERROR_OUT_OF_MEMORY;

  nsresult rv = request->Init(cx, options);
  NS_ENSURE_SUCCESS(rv, rv);

  if (mOwner) {
    if (!RegisterRequestWithPrompt(request))
      return NS_ERROR_NOT_AVAILABLE;
    
    mPendingCallbacks.AppendElement(request);
    return NS_OK;
  }

  if (!nsContentUtils::IsCallerChrome())
    return NS_ERROR_FAILURE;

  mPendingCallbacks.AppendElement(request);

  nsCOMPtr<nsIRunnable> ev = new RequestAllowEvent(true, request);
  NS_DispatchToMainThread(ev);

  return NS_OK;
}

NS_IMETHODIMP
nsGeolocation::WatchPosition(nsIDOMGeoPositionCallback *callback,
                             nsIDOMGeoPositionErrorCallback *errorCallback,
                             const jsval& options,
                             JSContext* cx,
                             PRInt32 *_retval NS_OUTPARAM)
{

  NS_ENSURE_ARG_POINTER(callback);

  if (!sGeoEnabled)
    return NS_ERROR_NOT_AVAILABLE;

  if (mPendingCallbacks.Length() > MAX_GEO_REQUESTS_PER_WINDOW)
    return NS_ERROR_NOT_AVAILABLE;

  nsRefPtr<nsGeolocationRequest> request = new nsGeolocationRequest(this,
								    callback,
								    errorCallback,
								    true);
  if (!request)
    return NS_ERROR_OUT_OF_MEMORY;

  nsresult rv = request->Init(cx, options);
  NS_ENSURE_SUCCESS(rv, rv);

  if (mOwner) {
    if (!RegisterRequestWithPrompt(request))
      return NS_ERROR_NOT_AVAILABLE;

    
    mWatchingCallbacks.AppendElement(request);
    *_retval = mWatchingCallbacks.Length() - 1;

    return NS_OK;
  }

  if (!nsContentUtils::IsCallerChrome())
    return NS_ERROR_FAILURE;

  request->Allow();

  
  mWatchingCallbacks.AppendElement(request);
  *_retval = mWatchingCallbacks.Length() - 1;

  return NS_OK;
}

NS_IMETHODIMP
nsGeolocation::ClearWatch(PRInt32 aWatchId)
{
  PRUint32 count = mWatchingCallbacks.Length();
  if (aWatchId < 0 || count == 0 || PRUint32(aWatchId) >= count)
    return NS_OK;

  mWatchingCallbacks[aWatchId]->MarkCleared();
  return NS_OK;
}

bool
nsGeolocation::WindowOwnerStillExists()
{
  
  
  
  if (mOwner == nsnull)
    return true;

  nsCOMPtr<nsPIDOMWindow> window = do_QueryReferent(mOwner);

  if (window)
  {
    bool closed = false;
    window->GetClosed(&closed);
    if (closed)
      return false;

    nsPIDOMWindow* outer = window->GetOuterWindow();
    if (!outer || outer->GetCurrentInnerWindow() != window)
      return false;
  }

  return true;
}

bool
nsGeolocation::RegisterRequestWithPrompt(nsGeolocationRequest* request)
{
  if (Preferences::GetBool("geo.prompt.testing", false)) {
    nsCOMPtr<nsIRunnable> ev =
        new RequestAllowEvent(Preferences::GetBool("geo.prompt.testing.allow",
                                                   false), request);
    NS_DispatchToMainThread(ev);
    return true;
  }

  if (XRE_GetProcessType() == GeckoProcessType_Content) {
    nsCOMPtr<nsPIDOMWindow> window = do_QueryReferent(mOwner);
    if (!window)
      return true;

    
    
    TabChild* child = GetTabChildFrom(window->GetDocShell());
    if (!child)
      return false;
    
    
    
    request->AddRef();

    nsCString type = NS_LITERAL_CSTRING("geolocation");
    child->SendPContentPermissionRequestConstructor(request, type, IPC::URI(mURI));
    
    request->Sendprompt();
    return true;
  }

  nsCOMPtr<nsIRunnable> ev  = new RequestPromptEvent(request);
  NS_DispatchToMainThread(ev);
  return true;
}

