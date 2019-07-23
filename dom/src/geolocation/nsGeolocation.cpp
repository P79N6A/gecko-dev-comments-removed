



































#include "nsGeolocation.h"
#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsIDOMWindow.h"
#include "nsDOMClassInfo.h"
#include "nsComponentManagerUtils.h"
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
    NotifyError(NS_GEO_ERROR_CODE_LOCATION_PROVIDER_ERROR);
    return NS_ERROR_FAILURE;;
  }

  return NS_OK;
}


NS_INTERFACE_MAP_BEGIN(nsGeolocationRequest)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIGeolocationRequest)
  NS_INTERFACE_MAP_ENTRY(nsIGeolocationRequest)
  NS_INTERFACE_MAP_ENTRY(nsITimerCallback)
NS_INTERFACE_MAP_END

NS_IMPL_ADDREF(nsGeolocationRequest)
NS_IMPL_RELEASE(nsGeolocationRequest)


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
    NotifyError(NS_GEO_ERROR_CODE_TIMEOUT);
    
    mLocator->RemoveRequest(this);
  }

  mTimeoutTimer = nsnull;
  return NS_OK;
}
 
NS_IMETHODIMP
nsGeolocationRequest::GetRequestingURI(nsIURI * *aRequestingURI)
{
  NS_ENSURE_ARG_POINTER(aRequestingURI);
  *aRequestingURI = mLocator->GetURI();
  NS_IF_ADDREF(*aRequestingURI);
  return NS_OK;
}

NS_IMETHODIMP
nsGeolocationRequest::GetRequestingWindow(nsIDOMWindow * *aRequestingWindow)
{
  NS_ENSURE_ARG_POINTER(aRequestingWindow);
  *aRequestingWindow = mLocator->GetOwner();
  NS_IF_ADDREF(*aRequestingWindow);
  return NS_OK;
}

NS_IMETHODIMP
nsGeolocationRequest::Cancel()
{
  NotifyError(NS_GEO_ERROR_CODE_PERMISSION_ERROR);

  
  mLocator->RemoveRequest(this);
  return NS_OK;
}

NS_IMETHODIMP
nsGeolocationRequest::Allow()
{
  
  nsRefPtr<nsGeolocationService> geoService = nsGeolocationService::GetInstance();
  nsresult rv = geoService->StartDevice();
  
  if (NS_FAILED(rv)) {
    
    NotifyError(NS_GEO_ERROR_CODE_LOCATION_PROVIDER_ERROR);
    return NS_OK;
  }

  PRUint32 timeout;
  if (mOptions && NS_SUCCEEDED(mOptions->GetTimeout(&timeout)) && timeout > 0) {
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

nsGeolocationService::nsGeolocationService()
 : mProviderStarted(PR_FALSE)
{
  nsCOMPtr<nsIObserverService> obs = do_GetService("@mozilla.org/observer-service;1");
  if (obs) {
    obs->AddObserver(this, "quit-application", false);
  }

  mTimeout = nsContentUtils::GetIntPref("geo.timeout", 6000);

  mProvider = do_GetService(NS_GEOLOCATION_PROVIDER_CONTRACTID);
  
  
#ifdef NS_MAEMO_LOCATION
  if (!mProvider)
    mProvider = new MaemoLocationProvider();
#endif

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
  for (PRUint32 i = 0; i< mGeolocators.Length(); i++)
    mGeolocators[i]->Update(aSomewhere);
  return NS_OK;
}

already_AddRefed<nsIDOMGeoPosition>
nsGeolocationService::GetLastKnownPosition()
{
  nsIDOMGeoPosition* p = nsnull;
  if (mProvider)
    mProvider->GetCurrentPosition(&p);

  return p;
}

PRBool
nsGeolocationService::IsDeviceReady()
{
  PRBool ready = PR_FALSE;
  if (mProvider)
    mProvider->IsReady(&ready);

  return ready;
}

PRBool
nsGeolocationService::HasGeolocationProvider()
{
  return (mProvider != nsnull);
}

nsresult
nsGeolocationService::StartDevice()
{
  if (!mProvider)
    return NS_ERROR_NOT_AVAILABLE;
  
  if (!mProviderStarted) {

    
    nsresult rv = mProvider->Startup();
    if (NS_FAILED(rv)) 
      return NS_ERROR_NOT_AVAILABLE;
  
    
    mProvider->Watch(this);

    
    mProviderStarted = PR_TRUE;

    
    
    
    SetDisconnectTimer();

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
                         mTimeout,
                         nsITimer::TYPE_ONE_SHOT);
}

void 
nsGeolocationService::StopDevice()
{
  if (mProvider) {
    mProvider->Shutdown();
    mProviderStarted = PR_FALSE;
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
  }
  return nsGeolocationService::gService;
}

nsGeolocationService*
nsGeolocationService::GetGeolocationService()
{
  nsGeolocationService* inst = nsGeolocationService::GetInstance();
  NS_ADDREF(inst);
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





NS_INTERFACE_MAP_BEGIN(nsGeolocation)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDOMGeoGeolocation)
  NS_INTERFACE_MAP_ENTRY(nsIDOMGeoGeolocation)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(GeoGeolocation)
NS_INTERFACE_MAP_END

NS_IMPL_ADDREF(nsGeolocation)
NS_IMPL_RELEASE(nsGeolocation)

nsGeolocation::nsGeolocation(nsIDOMWindow* aContentDom) 
: mUpdateInProgress(PR_FALSE)
{
  
  nsCOMPtr<nsPIDOMWindow> window = do_QueryInterface(aContentDom);
  if (window)
    mOwner = window->GetCurrentInnerWindow();

  
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
  mOwner = nsnull;
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
nsGeolocation::GetLastPosition(nsIDOMGeoPosition * *aLastPosition)
{
  
  NS_ENSURE_ARG_POINTER(aLastPosition);
  *aLastPosition = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
nsGeolocation::GetCurrentPosition(nsIDOMGeoPositionCallback *callback,
                                  nsIDOMGeoPositionErrorCallback *errorCallback,
                                  nsIDOMGeoPositionOptions *options)
{
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

  if (aWatchId < 0 || count == 0 || aWatchId > count + 1)
    return NS_ERROR_FAILURE;

  mWatchingCallbacks[aWatchId]->MarkCleared();
  return NS_OK;
}

PRBool
nsGeolocation::OwnerStillExists()
{
  if (!mOwner)
    return PR_FALSE;

  nsCOMPtr<nsIDOMWindowInternal> domWindow(mOwner);
  if (domWindow)
  {
    PRBool closed = PR_FALSE;
    domWindow->GetClosed(&closed);
    if (closed)
      return PR_FALSE;
  }

  nsPIDOMWindow* outer = mOwner->GetOuterWindow();
  if (!outer || outer->GetCurrentInnerWindow() != mOwner)
    return PR_FALSE;

  return PR_TRUE;
}
