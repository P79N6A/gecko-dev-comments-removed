



































#include "nsGeolocation.h"
#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsIDOMWindow.h"
#include "nsDOMClassInfo.h"
#include "nsComponentManagerUtils.h"
#include "nsServiceManagerUtils.h"
#include "nsIJSContextStack.h"
#include "nsIScriptGlobalObject.h"
#include "nsIScriptSecurityManager.h"
#include "nsContentUtils.h"
#include "nsIPrincipal.h"
#include "nsIURI.h"
#include "nsIPermissionManager.h"
#include "nsIObserverService.h"
#include "nsIPrefService.h"
#include "nsIPrefBranch2.h"

#ifdef NS_OSSO
#include "MaemoLocationProvider.h"
#endif





nsGeolocationRequest::nsGeolocationRequest(nsGeolocator* locator, nsIDOMGeolocationCallback* callback)
  : mAllowed(PR_FALSE), mCleared(PR_FALSE), mFuzzLocation(PR_FALSE), mCallback(callback), mLocator(locator)
{
  SetOwner();
  SetURI();
}

nsGeolocationRequest::~nsGeolocationRequest()
{
}

NS_INTERFACE_MAP_BEGIN(nsGeolocationRequest)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIGeolocationRequest)
  NS_INTERFACE_MAP_ENTRY(nsIGeolocationRequest)
NS_INTERFACE_MAP_END

NS_IMPL_ADDREF(nsGeolocationRequest)
NS_IMPL_RELEASE(nsGeolocationRequest)

NS_IMETHODIMP
nsGeolocationRequest::GetRequestingURI(nsIURI * *aRequestingURI)
{
  NS_ENSURE_ARG_POINTER(aRequestingURI);
  *aRequestingURI = mURI;
  NS_ADDREF(*aRequestingURI);
  return NS_OK;
}

NS_IMETHODIMP
nsGeolocationRequest::GetRequestingWindow(nsIDOMWindow * *aRequestingWindow)
{
  NS_ENSURE_ARG_POINTER(aRequestingWindow);
  *aRequestingWindow = mOwner;
  NS_ADDREF(*aRequestingWindow);
  return NS_OK;
}

NS_IMETHODIMP
nsGeolocationRequest::Cancel()
{
  
  mCallback->OnRequest(nsnull);

  
  mLocator->RemoveRequest(this);
  return NS_OK;
}

NS_IMETHODIMP
nsGeolocationRequest::Allow()
{
  
  nsRefPtr<nsGeolocatorService> geoService = nsGeolocatorService::GetInstance();
  geoService->StartDevice();

  mAllowed = PR_TRUE;
  return NS_OK;
}

NS_IMETHODIMP
nsGeolocationRequest::AllowButFuzz()
{
  mFuzzLocation = PR_TRUE;
  return Allow();
}

void
nsGeolocationRequest::MarkCleared()
{
  mCleared = PR_TRUE;
}

void
nsGeolocationRequest::SendLocation(nsIDOMGeolocation* location)
{
  if (!OwnerStillExists() || mCleared)
    return;

  
  if (mFuzzLocation)
  {
    
    

    double lat, lon, alt, herror, verror;
    DOMTimeStamp time;
    location->GetLatitude(&lat);
    location->GetLongitude(&lon);
    location->GetAltitude(&alt);
    location->GetHorizontalAccuracy(&herror);
    location->GetVerticalAccuracy(&verror);
    location->GetTimestamp(&time); 

    
    
    
    lat = 0; lon = 0; alt = 0; herror = 100000; verror = 100000;

    nsRefPtr<nsGeolocation> somewhere = new nsGeolocation(lat,
                                                          lon,
                                                          alt,
                                                          herror,
                                                          verror,
                                                          time);
    mCallback->OnRequest(somewhere);
    return;
  }
  
  mCallback->OnRequest(location);
}

void
nsGeolocationRequest::SetURI()
{

  nsIScriptSecurityManager* secman = nsContentUtils::GetSecurityManager();
  if (!secman)
    return;
  
  nsCOMPtr<nsIPrincipal> principal;
  secman->GetSubjectPrincipal(getter_AddRefs(principal));
  if (!principal)
    return;

  principal->GetURI(getter_AddRefs(mURI));
}

void
nsGeolocationRequest::SetOwner()
{
  nsCOMPtr<nsIJSContextStack> stack = do_GetService("@mozilla.org/js/xpc/ContextStack;1");
  if (!stack)
  {
    return;
  }

  JSContext *cx;
  if (NS_FAILED(stack->Peek(&cx)) || !cx) 
  {
    return;
  }

  nsIScriptContext* context = GetScriptContextFromJSContext(cx);
  if (context)
  {
    nsCOMPtr<nsPIDOMWindow> window = do_QueryInterface(context->GetGlobalObject());
    if (window)
    {
      mOwner = window->GetCurrentInnerWindow();
    }
  }
}

void
nsGeolocationRequest::Shutdown()
{
  mCleared = PR_TRUE;

  mCallback = nsnull;
  mOwner = nsnull;
  mURI = nsnull;
}

PRBool
nsGeolocationRequest::OwnerStillExists()
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
  {
    return PR_FALSE;
  }

  return PR_TRUE;
}





NS_INTERFACE_MAP_BEGIN(nsGeolocation)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDOMGeolocation)
  NS_INTERFACE_MAP_ENTRY(nsIDOMGeolocation)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(Geolocation)
NS_INTERFACE_MAP_END

NS_IMPL_THREADSAFE_ADDREF(nsGeolocation)
NS_IMPL_THREADSAFE_RELEASE(nsGeolocation)

NS_IMETHODIMP
nsGeolocation::GetLatitude(double *aLatitude)
{
  *aLatitude = mLat;
  return NS_OK;
}

NS_IMETHODIMP
nsGeolocation::GetLongitude(double *aLongitude)
{
  *aLongitude = mLong;
  return NS_OK;
}

NS_IMETHODIMP
nsGeolocation::GetAltitude(double *aAltitude)
{
  *aAltitude = mAlt;
  return NS_OK;
}

NS_IMETHODIMP
nsGeolocation::GetHorizontalAccuracy(double *aHorizontalAccuracy)
{
  *aHorizontalAccuracy = mHError;
  return NS_OK;
}

NS_IMETHODIMP
nsGeolocation::GetVerticalAccuracy(double *aVerticalAccuracy)
{
  *aVerticalAccuracy = mVError;
  return NS_OK;
}

NS_IMETHODIMP
nsGeolocation::GetTimestamp(DOMTimeStamp* aTimestamp)
{
  *aTimestamp = mTimestamp;
  return NS_OK;
}




NS_INTERFACE_MAP_BEGIN(nsGeolocatorService)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIGeolocationUpdate)
  NS_INTERFACE_MAP_ENTRY(nsIGeolocationUpdate)
  NS_INTERFACE_MAP_ENTRY(nsIGeolocationService)
  NS_INTERFACE_MAP_ENTRY(nsIObserver)
NS_INTERFACE_MAP_END

NS_IMPL_THREADSAFE_ADDREF(nsGeolocatorService)
NS_IMPL_THREADSAFE_RELEASE(nsGeolocatorService)

nsGeolocatorService::nsGeolocatorService()
{
  nsCOMPtr<nsIObserverService> obs = do_GetService("@mozilla.org/observer-service;1");
  if (obs) {
    obs->AddObserver(this, "quit-application", false);
  }

  mTimeout = nsContentUtils::GetIntPref("geo.timeout", 6000);
}

nsGeolocatorService::~nsGeolocatorService()
{
}

NS_IMETHODIMP
nsGeolocatorService::Observe(nsISupports* aSubject, const char* aTopic,
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

    
    mPrompt = nsnull;
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
nsGeolocatorService::GetPrompt(nsIGeolocationPrompt * *aPrompt)
{
  NS_ENSURE_ARG_POINTER(aPrompt);
  *aPrompt = mPrompt;
  return NS_OK;
}

NS_IMETHODIMP
nsGeolocatorService::SetPrompt(nsIGeolocationPrompt * aPrompt)
{
  mPrompt = aPrompt;
  return NS_OK;
}

NS_IMETHODIMP
nsGeolocatorService::Update(nsIDOMGeolocation *somewhere)
{
  for (PRUint32 i = 0; i< mGeolocators.Length(); i++)
    mGeolocators[i]->Update(somewhere);
  return NS_OK;
}

already_AddRefed<nsIDOMGeolocation>
nsGeolocatorService::GetLastKnownPosition()
{
  nsIDOMGeolocation* location = nsnull;
  if (mProvider)
    mProvider->GetCurrentLocation(&location);

  return location;
}

PRBool
nsGeolocatorService::IsDeviceReady()
{
  PRBool ready = PR_FALSE;
  if (mProvider)
    mProvider->IsReady(&ready);

  return ready;
}

nsresult
nsGeolocatorService::StartDevice()
{
  if (!mProvider)
  {
    
    mProvider = do_GetService(NS_GEOLOCATION_PROVIDER_CONTRACTID);

    
#ifdef NS_OSSO
    if (!mProvider)
    {
      
      mProvider = new MaemoLocationProvider();
    }
#endif

    if (!mProvider)
      return NS_ERROR_NOT_AVAILABLE;
    
    
    nsresult rv = mProvider->Startup();
    if (NS_FAILED(rv)) 
      return NS_ERROR_NOT_AVAILABLE;;
    
    
    mProvider->Watch(this);
    
    
    
    
    SetDisconnectTimer();
  }

  return NS_OK;
}

void
nsGeolocatorService::SetDisconnectTimer()
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
nsGeolocatorService::StopDevice()
{
  if (mProvider) {
    mProvider->Shutdown();
    mProvider = nsnull;
  }

  if(mDisconnectTimer) {
    mDisconnectTimer->Cancel();
    mDisconnectTimer = nsnull;
  }
}

nsGeolocatorService* nsGeolocatorService::gService = nsnull;

nsGeolocatorService*
nsGeolocatorService::GetInstance()
{
  if (!nsGeolocatorService::gService) {
    nsGeolocatorService::gService = new nsGeolocatorService();
  }
  return nsGeolocatorService::gService;
}

nsGeolocatorService*
nsGeolocatorService::GetGeolocationService()
{
  nsGeolocatorService* inst = nsGeolocatorService::GetInstance();
  NS_ADDREF(inst);
  return inst;
}

void
nsGeolocatorService::AddLocator(nsGeolocator* locator)
{
  mGeolocators.AppendElement(locator);
}

void
nsGeolocatorService::RemoveLocator(nsGeolocator* locator)
{
  mGeolocators.RemoveElement(locator);
}





NS_INTERFACE_MAP_BEGIN(nsGeolocator)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDOMGeolocator)
  NS_INTERFACE_MAP_ENTRY(nsIDOMGeolocator)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(Geolocator)
NS_INTERFACE_MAP_END

NS_IMPL_ADDREF(nsGeolocator)
NS_IMPL_RELEASE(nsGeolocator)

nsGeolocator::nsGeolocator() 
 : mUpdateInProgress(PR_FALSE)
{
  mService = nsGeolocatorService::GetInstance();
  if (mService)
    mService->AddLocator(this);
}

nsGeolocator::~nsGeolocator()
{
}

void
nsGeolocator::Shutdown()
{
  
  for (PRInt32 i = 0; i< mPendingCallbacks.Count(); i++)
    mPendingCallbacks[i]->Shutdown();
  mPendingCallbacks.Clear();

  for (PRInt32 i = 0; i< mWatchingCallbacks.Count(); i++)
    mWatchingCallbacks[i]->Shutdown();
  mWatchingCallbacks.Clear();

  if (mService)
    mService->RemoveLocator(this);

  mService = nsnull;
}

PRBool
nsGeolocator::HasActiveCallbacks()
{
  return (PRBool) mWatchingCallbacks.Count();
}

void
nsGeolocator::RemoveRequest(nsGeolocationRequest* request)
{
  mPendingCallbacks.RemoveObject(request);

  
  
  
  
  

  request->MarkCleared();
}

void
nsGeolocator::Update(nsIDOMGeolocation *somewhere)
{
  
  
  
  
  

  if (mUpdateInProgress)
    return;

  mUpdateInProgress = PR_TRUE;

  
  for (PRInt32 i = 0; i< mPendingCallbacks.Count(); i++)
    mPendingCallbacks[i]->SendLocation(somewhere);
  mPendingCallbacks.Clear();

  
  for (PRInt32 i = 0; i< mWatchingCallbacks.Count(); i++)
      mWatchingCallbacks[i]->SendLocation(somewhere);

  mUpdateInProgress = PR_FALSE;
}

NS_IMETHODIMP
nsGeolocator::GetLastPosition(nsIDOMGeolocation * *aLastPosition)
{
  
  NS_ENSURE_ARG_POINTER(aLastPosition);
  *aLastPosition = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
nsGeolocator::GetCurrentPosition(nsIDOMGeolocationCallback *callback)
{
  nsIGeolocationPrompt* prompt = mService->GetPrompt();
  if (prompt == nsnull)
    return NS_ERROR_NOT_AVAILABLE;

  nsRefPtr<nsGeolocationRequest> request = new nsGeolocationRequest(this, callback);
  prompt->Prompt(request);

  
  mPendingCallbacks.AppendObject(request);
  return NS_OK;
}

NS_IMETHODIMP
nsGeolocator::WatchPosition(nsIDOMGeolocationCallback *callback, PRUint16 *_retval NS_OUTPARAM)
{
  nsIGeolocationPrompt* prompt = mService->GetPrompt();
  if (prompt == nsnull)
    return NS_ERROR_NOT_AVAILABLE;

  nsRefPtr<nsGeolocationRequest> request = new nsGeolocationRequest(this, callback);
  prompt->Prompt(request);

  
  mWatchingCallbacks.AppendObject(request);
  *_retval = mWatchingCallbacks.Count() - 1;
  return NS_OK;
}

NS_IMETHODIMP
nsGeolocator::ClearWatch(PRUint16 watchId)
{
  mWatchingCallbacks[watchId]->MarkCleared();
  return NS_OK;
}
