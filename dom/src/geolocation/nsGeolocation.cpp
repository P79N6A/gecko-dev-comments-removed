



































#include "nsGeolocation.h"
#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsIDOMWindow.h"
#include "nsDOMClassInfo.h"
#include "nsComponentManagerUtils.h"
#include "nsICategoryManager.h"
#include "nsISupportsPrimitives.h"
#include "nsServiceManagerUtils.h"
#include "nsContentUtils.h"
#include "nsIURI.h"
#include "nsIPermissionManager.h"
#include "nsIObserverService.h"
#include "nsIPrefService.h"
#include "nsIPrefBranch2.h"
#include "nsIJSContextStack.h"

#include <math.h>

#ifdef NS_MAEMO_LOCATION
#include "MaemoLocationProvider.h"
#endif

#ifdef WINCE_WINDOWS_MOBILE
#include "WinMobileLocationProvider.h"
#endif

#include "nsIDOMDocument.h"
#include "nsIDocument.h"



#define MAX_GEO_REQUESTS_PER_WINDOW  1500





class nsDOMGeoPositionError : public nsIDOMGeoPositionError
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

NS_IMETHODIMP
nsDOMGeoPositionError::GetMessage(nsAString & aMessage)
{
  aMessage.Truncate();
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
  
  aCallback->HandleEvent(this);
  
  
  JSContext* cx;
  stack->Pop(&cx);
}




nsGeolocationRequest::nsGeolocationRequest(nsGeolocation* aLocator,
                                           nsIDOMGeoPositionCallback* aCallback,
                                           nsIDOMGeoPositionErrorCallback* aErrorCallback,
                                           nsIDOMGeoPositionOptions* aOptions)
  : mAllowed(PR_FALSE),
    mCleared(PR_FALSE),
    mHasSentData(PR_FALSE),
    mCallback(aCallback),
    mErrorCallback(aErrorCallback),
    mOptions(aOptions),
    mLocator(aLocator)
{
}

nsGeolocationRequest::~nsGeolocationRequest()
{
}

nsresult
nsGeolocationRequest::Init()
{
  

  
  nsRefPtr<nsGeolocationService> geoService = nsGeolocationService::GetInstance();
  if (!geoService->HasGeolocationProvider()) {
    NotifyError(nsIDOMGeoPositionError::POSITION_UNAVAILABLE);
    return NS_ERROR_FAILURE;
  }

  return NS_OK;
}

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsGeolocationRequest)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIGeolocationRequest)
  NS_INTERFACE_MAP_ENTRY(nsIGeolocationRequest)
  NS_INTERFACE_MAP_ENTRY(nsITimerCallback)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsGeolocationRequest)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsGeolocationRequest)

NS_IMPL_CYCLE_COLLECTION_4(nsGeolocationRequest, mCallback, mErrorCallback, mOptions, mLocator)


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
  
  
  
  
  if (!mHasSentData) {
    NotifyError(nsIDOMGeoPositionError::TIMEOUT);
    
    mLocator->RemoveRequest(this);
  }

  mTimeoutTimer = nsnull;
  return NS_OK;
}
 
NS_IMETHODIMP
nsGeolocationRequest::GetRequestingURI(nsIURI * *aRequestingURI)
{
  NS_ENSURE_ARG_POINTER(aRequestingURI);

  nsCOMPtr<nsIURI> uri = mLocator->GetURI();
  uri.forget(aRequestingURI);

  return NS_OK;
}

NS_IMETHODIMP
nsGeolocationRequest::GetRequestingWindow(nsIDOMWindow * *aRequestingWindow)
{
  NS_ENSURE_ARG_POINTER(aRequestingWindow);

  nsCOMPtr<nsIDOMWindow> window = do_QueryReferent(mLocator->GetOwner());
  window.forget(aRequestingWindow);

  return NS_OK;
}

NS_IMETHODIMP
nsGeolocationRequest::Cancel()
{
  NotifyError(nsIDOMGeoPositionError::PERMISSION_DENIED);

  
  mLocator->RemoveRequest(this);
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
    PRInt32 tempAge;
    nsresult rv = mOptions->GetMaximumAge(&tempAge);
    if (NS_SUCCEEDED(rv)) {
      if (tempAge >= 0)
        maximumAge = tempAge;
    }
  }

  if (lastPosition && maximumAge > 0 && ( (PR_Now() / PR_USEC_PER_MSEC ) - maximumAge <= cachedPositionTime) ) {
    
    mAllowed = PR_TRUE;
    
    
    SendLocation(lastPosition);
  }

  PRInt32 timeout;
  if (mOptions && NS_SUCCEEDED(mOptions->GetTimeout(&timeout)) && timeout > 0) {
    
    if (timeout < 10)
      timeout = 10;

    mTimeoutTimer = do_CreateInstance("@mozilla.org/timer;1");
    mTimeoutTimer->InitWithCallback(this, timeout, nsITimer::TYPE_ONE_SHOT);
  }

  mAllowed = PR_TRUE;
  return NS_OK;
}

void
nsGeolocationRequest::MarkCleared()
{
  mCleared = PR_TRUE;
}

void
nsGeolocationRequest::SendLocation(nsIDOMGeoPosition* aPosition)
{
  if (mCleared || !mAllowed)
    return;

  
  if (!aPosition) {
    NotifyError(nsIDOMGeoPositionError::POSITION_UNAVAILABLE);
    return;
  }

  
  nsCOMPtr<nsIJSContextStack> stack(do_GetService("@mozilla.org/js/xpc/ContextStack;1"));
  if (!stack || NS_FAILED(stack->Push(nsnull)))
    return; 
  
  mCallback->HandleEvent(aPosition);

  
  JSContext* cx;
  stack->Pop(&cx);

  mHasSentData = PR_TRUE;
}

void
nsGeolocationRequest::Shutdown()
{
  mCleared = PR_TRUE;
  mCallback = nsnull;
  mErrorCallback = nsnull;
}




NS_INTERFACE_MAP_BEGIN(nsGeolocationService)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIGeolocationUpdate)
  NS_INTERFACE_MAP_ENTRY(nsIGeolocationUpdate)
  NS_INTERFACE_MAP_ENTRY(nsIObserver)
NS_INTERFACE_MAP_END

NS_IMPL_THREADSAFE_ADDREF(nsGeolocationService)
NS_IMPL_THREADSAFE_RELEASE(nsGeolocationService)


static PRBool sGeoEnabled = PR_TRUE;
static int
GeoEnabledChangedCallback(const char *aPrefName, void *aClosure)
{
  sGeoEnabled = nsContentUtils::GetBoolPref("geo.enabled", PR_TRUE);
  return 0;
}

nsresult nsGeolocationService::Init()
{
  nsCOMPtr<nsIObserverService> obs = do_GetService("@mozilla.org/observer-service;1");
  if (!obs)
    return NS_ERROR_FAILURE;

  obs->AddObserver(this, "quit-application", false);

  mTimeout = nsContentUtils::GetIntPref("geo.timeout", 6000);

  nsContentUtils::RegisterPrefCallback("geo.enabled",
                                       GeoEnabledChangedCallback,
                                       nsnull);

  GeoEnabledChangedCallback("geo.enabled", nsnull);

  if (sGeoEnabled == PR_FALSE)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIGeolocationProvider> provider = do_GetService(NS_GEOLOCATION_PROVIDER_CONTRACTID);
  if (provider)
    mProviders.AppendObject(provider);


  
  nsCOMPtr<nsICategoryManager> catMan(do_GetService("@mozilla.org/categorymanager;1"));
  if (!catMan)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsISimpleEnumerator> geoproviders;
  catMan->EnumerateCategory("geolocation-provider", getter_AddRefs(geoproviders));
  if (geoproviders) {

    PRBool hasMore;
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

  

  
#ifdef NS_MAEMO_LOCATION
  provider = new MaemoLocationProvider();
  if (provider)
    mProviders.AppendObject(provider);
#endif

  
#ifdef WINCE_WINDOWS_MOBILE
  provider = new WinMobileLocationProvider();
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
    nsCOMPtr<nsIObserverService> obs = do_GetService("@mozilla.org/observer-service;1");
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
  
  

  if (!IsBetterPosition(aSomewhere))
    return NS_OK;

  SetCachedPosition(aSomewhere);

  for (PRUint32 i = 0; i< mGeolocators.Length(); i++)
    mGeolocators[i]->Update(aSomewhere);
  return NS_OK;
}

PRBool
nsGeolocationService::IsBetterPosition(nsIDOMGeoPosition *aSomewhere)
{
  if (!aSomewhere)
    return PR_FALSE;

  nsRefPtr<nsGeolocationService> geoService = nsGeolocationService::GetInstance();
  if (!geoService)
    return PR_FALSE;

  nsCOMPtr<nsIDOMGeoPosition> lastPosition = geoService->GetCachedPosition();
  if (!lastPosition)
    return PR_TRUE;
  
  nsresult rv;
  DOMTimeStamp oldTime;
  rv = lastPosition->GetTimestamp(&oldTime);
  NS_ENSURE_SUCCESS(rv, PR_FALSE);

  nsCOMPtr<nsIDOMGeoPositionCoords> coords;
  lastPosition->GetCoords(getter_AddRefs(coords));
  if (!coords)
    return PR_FALSE;

  double oldAccuracy;
  rv = coords->GetAccuracy(&oldAccuracy);
  NS_ENSURE_SUCCESS(rv, PR_FALSE);

  double oldLat, oldLon;
  rv = coords->GetLongitude(&oldLon);
  NS_ENSURE_SUCCESS(rv, PR_FALSE);

  rv = coords->GetLatitude(&oldLat);
  NS_ENSURE_SUCCESS(rv, PR_FALSE);


  DOMTimeStamp newTime;
  rv = aSomewhere->GetTimestamp(&newTime);
  NS_ENSURE_SUCCESS(rv, PR_FALSE);

  aSomewhere->GetCoords(getter_AddRefs(coords));
  if (!coords)
    return PR_FALSE;

  double newAccuracy;
  rv = coords->GetAccuracy(&newAccuracy);
  NS_ENSURE_SUCCESS(rv, PR_FALSE);

  double newLat, newLon;
  rv = coords->GetLongitude(&newLon);
  NS_ENSURE_SUCCESS(rv, PR_FALSE);

  rv = coords->GetLatitude(&newLat);
  NS_ENSURE_SUCCESS(rv, PR_FALSE);

  
  double delta = fabs(newLat - oldLat) + fabs(newLon + oldLon);

  
  
  delta *= 60 * 1852;

  
  
  double max_accuracy = PR_MAX(oldAccuracy, newAccuracy);
  if (delta > max_accuracy)
    return PR_TRUE;

  
  if (oldAccuracy >= newAccuracy)
    return PR_TRUE;

  return PR_FALSE;
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

PRBool
nsGeolocationService::HasGeolocationProvider()
{
  return mProviders.Count() > 0;
}

nsresult
nsGeolocationService::StartDevice()
{
  if (sGeoEnabled == PR_FALSE)
    return NS_ERROR_NOT_AVAILABLE;

  if (!HasGeolocationProvider())
    return NS_ERROR_NOT_AVAILABLE;
  
  

  
  nsresult rv = NS_ERROR_NOT_AVAILABLE;
  for (PRUint32 i = mProviders.Count() - 1; i != PRUint32(-1); --i) {
    
    
    nsresult temp = mProviders[i]->Startup();
    if (NS_SUCCEEDED(temp)) {
      rv = NS_OK;

      mProviders[i]->Watch(this);
    }
  }
  
  if (NS_FAILED(rv)) 
    return NS_ERROR_NOT_AVAILABLE;
  
  
  
  
  SetDisconnectTimer();

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
                         mTimeout,
                         nsITimer::TYPE_ONE_SHOT);
}

void 
nsGeolocationService::StopDevice()
{
  for (PRUint32 i = mProviders.Count() - 1; i != PRUint32(-1); --i) {
    mProviders[i]->Shutdown();
  }

  if(mDisconnectTimer) {
    mDisconnectTimer->Cancel();
    mDisconnectTimer = nsnull;
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
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR_AMBIGUOUS(mPendingCallbacks[i], nsIGeolocationRequest)

  for (i = 0; i < tmp->mWatchingCallbacks.Length(); ++i)
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR_AMBIGUOUS(mWatchingCallbacks[i], nsIGeolocationRequest)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

nsGeolocation::nsGeolocation(nsIDOMWindow* aContentDom) 
: mUpdateInProgress(PR_FALSE)
{
  
  nsCOMPtr<nsPIDOMWindow> window = do_QueryInterface(aContentDom);
  if (window)
    mOwner = do_GetWeakReference(window->GetCurrentInnerWindow());

  
  nsCOMPtr<nsIDOMDocument> domdoc;
  aContentDom->GetDocument(getter_AddRefs(domdoc));
  nsCOMPtr<nsIDocument> doc = do_QueryInterface(domdoc);
  if (doc)
    doc->NodePrincipal()->GetURI(getter_AddRefs(mURI));

  mService = nsGeolocationService::GetInstance();
  if (mService)
    mService->AddLocator(this);
}

nsGeolocation::~nsGeolocation()
{
  if (mService)
    Shutdown();
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

PRBool
nsGeolocation::HasActiveCallbacks()
{
  return mWatchingCallbacks.Length() != 0;
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
  
  
  
  
  

  if (mUpdateInProgress)
    return;

  mUpdateInProgress = PR_TRUE;

  if (!OwnerStillExists())
  {
    Shutdown();
    return;
  }

  
  for (PRUint32 i = 0; i< mPendingCallbacks.Length(); i++)
    mPendingCallbacks[i]->SendLocation(aSomewhere);
  mPendingCallbacks.Clear();

  
  for (PRUint32 i = 0; i< mWatchingCallbacks.Length(); i++)
    mWatchingCallbacks[i]->SendLocation(aSomewhere);

  mUpdateInProgress = PR_FALSE;
}

NS_IMETHODIMP
nsGeolocation::GetCurrentPosition(nsIDOMGeoPositionCallback *callback,
                                  nsIDOMGeoPositionErrorCallback *errorCallback,
                                  nsIDOMGeoPositionOptions *options)
{
  NS_ENSURE_ARG_POINTER(callback);

  if (sGeoEnabled == PR_FALSE)
    return NS_ERROR_NOT_AVAILABLE;

  nsCOMPtr<nsIGeolocationPrompt> prompt = do_GetService(NS_GEOLOCATION_PROMPT_CONTRACTID);
  if (prompt == nsnull)
    return NS_ERROR_NOT_AVAILABLE;

  if (mPendingCallbacks.Length() > MAX_GEO_REQUESTS_PER_WINDOW)
    return NS_ERROR_NOT_AVAILABLE;

  nsRefPtr<nsGeolocationRequest> request = new nsGeolocationRequest(this, callback, errorCallback, options);
  if (!request)
    return NS_ERROR_OUT_OF_MEMORY;

  if (NS_FAILED(request->Init()))
    return NS_OK;

  prompt->Prompt(request);

  
  mPendingCallbacks.AppendElement(request);
  return NS_OK;
}

NS_IMETHODIMP
nsGeolocation::WatchPosition(nsIDOMGeoPositionCallback *aCallback,
                             nsIDOMGeoPositionErrorCallback *aErrorCallback,
                             nsIDOMGeoPositionOptions *aOptions, 
                             PRInt32 *_retval NS_OUTPARAM)
{
  NS_ENSURE_ARG_POINTER(aCallback);

  if (sGeoEnabled == PR_FALSE)
    return NS_ERROR_NOT_AVAILABLE;

  nsCOMPtr<nsIGeolocationPrompt> prompt = do_GetService(NS_GEOLOCATION_PROMPT_CONTRACTID);
  if (prompt == nsnull)
    return NS_ERROR_NOT_AVAILABLE;

  if (mWatchingCallbacks.Length() > MAX_GEO_REQUESTS_PER_WINDOW)
    return NS_ERROR_NOT_AVAILABLE;

  nsRefPtr<nsGeolocationRequest> request = new nsGeolocationRequest(this, aCallback, aErrorCallback, aOptions);
  if (!request)
    return NS_ERROR_OUT_OF_MEMORY;

  if (NS_FAILED(request->Init()))
    return NS_OK;

  prompt->Prompt(request);

  
  mWatchingCallbacks.AppendElement(request);
  *_retval = mWatchingCallbacks.Length() - 1;
  return NS_OK;
}

NS_IMETHODIMP
nsGeolocation::ClearWatch(PRInt32 aWatchId)
{
  PRUint32 count = mWatchingCallbacks.Length();
  if (aWatchId < 0 || count == 0 || aWatchId > count)
    return NS_ERROR_FAILURE;

  mWatchingCallbacks[aWatchId]->MarkCleared();
  return NS_OK;
}

PRBool
nsGeolocation::OwnerStillExists()
{
  nsCOMPtr<nsPIDOMWindow> window = do_QueryReferent(mOwner);

  if (!window)
    return PR_FALSE;

  if (window)
  {
    PRBool closed = PR_FALSE;
    window->GetClosed(&closed);
    if (closed)
      return PR_FALSE;
  }

  nsPIDOMWindow* outer = window->GetOuterWindow();
  if (!outer || outer->GetCurrentInnerWindow() != window)
    return PR_FALSE;

  return PR_TRUE;
}
