



































#ifndef nsGeoLocation_h
#define nsGeoLocation_h

#include "mozilla/dom/PContentPermissionRequestChild.h"

#undef CreateEvent

#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "nsCOMArray.h"
#include "nsTArray.h"
#include "nsITimer.h"
#include "nsIObserver.h"
#include "nsIURI.h"

#include "nsWeakPtr.h"
#include "nsCycleCollectionParticipant.h"

#include "nsIDOMGeoGeolocation.h"
#include "nsIDOMGeoPosition.h"
#include "nsIDOMGeoPositionError.h"
#include "nsIDOMGeoPositionCallback.h"
#include "nsIDOMGeoPositionErrorCallback.h"
#include "nsIDOMGeoPositionOptions.h"
#include "nsIDOMNavigatorGeolocation.h"

#include "nsPIDOMWindow.h"

#include "nsIGeolocationProvider.h"
#include "nsIContentPermissionPrompt.h"

#include "PCOMContentPermissionRequestChild.h"

class nsGeolocationService;
class nsGeolocation;

class nsGeolocationRequest
 : public nsIContentPermissionRequest
 , public nsITimerCallback
 , public PCOMContentPermissionRequestChild
{
 public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_NSICONTENTPERMISSIONREQUEST
  NS_DECL_NSITIMERCALLBACK

  NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(nsGeolocationRequest, nsIContentPermissionRequest)

  nsGeolocationRequest(nsGeolocation* locator,
                       nsIDOMGeoPositionCallback* callback,
                       nsIDOMGeoPositionErrorCallback* errorCallback,
                       nsIDOMGeoPositionOptions* options,
                       PRBool watchPositionRequest = PR_FALSE);
  nsresult Init();
  void Shutdown();

  
  void Update(nsIDOMGeoPosition* aPosition);

  void SendLocation(nsIDOMGeoPosition* location);
  void MarkCleared();
  PRBool IsActive() {return !mCleared;}
  PRBool Allowed() {return mAllowed;}
  void SetTimeoutTimer();

  ~nsGeolocationRequest();

  bool Recv__delete__(const bool& allow);
  void IPDLRelease() { Release(); }

 private:

  void NotifyError(PRInt16 errorCode);
  PRPackedBool mAllowed;
  PRPackedBool mCleared;
  PRPackedBool mIsWatchPositionRequest;

  nsCOMPtr<nsITimer> mTimeoutTimer;
  nsCOMPtr<nsIDOMGeoPositionCallback> mCallback;
  nsCOMPtr<nsIDOMGeoPositionErrorCallback> mErrorCallback;
  nsCOMPtr<nsIDOMGeoPositionOptions> mOptions;

  nsRefPtr<nsGeolocation> mLocator;
};




class nsGeolocationService : public nsIGeolocationUpdate, public nsIObserver
{
public:

  static nsGeolocationService* GetGeolocationService();
  static nsGeolocationService* GetInstance();  
  static nsGeolocationService* gService;

  NS_DECL_ISUPPORTS
  NS_DECL_NSIGEOLOCATIONUPDATE
  NS_DECL_NSIOBSERVER

  nsGeolocationService() {mTimeout = 6000;};

  nsresult Init();

  
  void AddLocator(nsGeolocation* locator);
  void RemoveLocator(nsGeolocation* locator);

  void SetCachedPosition(nsIDOMGeoPosition* aPosition);
  nsIDOMGeoPosition* GetCachedPosition();

  
  nsresult StartDevice();

  
  void     StopDevice();
  
  
  void     SetDisconnectTimer();

private:

  ~nsGeolocationService();

  
  
  
  nsCOMPtr<nsITimer> mDisconnectTimer;

  
  PRInt32 mTimeout;

  
  nsCOMArray<nsIGeolocationProvider> mProviders;

  
  
  
  nsTArray<nsGeolocation*> mGeolocators;

  
  nsCOMPtr<nsIDOMGeoPosition> mLastPosition;
};




 
class nsGeolocation : public nsIDOMGeoGeolocation
{
public:

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_NSIDOMGEOGEOLOCATION

  NS_DECL_CYCLE_COLLECTION_CLASS(nsGeolocation)

  nsGeolocation();

  nsresult Init(nsIDOMWindow* contentDom=nsnull);

  
  void Update(nsIDOMGeoPosition* aPosition);

  
  PRBool HasActiveCallbacks();

  
  void RemoveRequest(nsGeolocationRequest* request);

  
  void Shutdown();

  
  nsIURI* GetURI() { return mURI; }

  
  nsIWeakReference* GetOwner() { return mOwner; }

  
  PRBool WindowOwnerStillExists();

private:

  ~nsGeolocation();

  bool RegisterRequestWithPrompt(nsGeolocationRequest* request);

  
  
  
  

  nsTArray<nsRefPtr<nsGeolocationRequest> > mPendingCallbacks;
  nsTArray<nsRefPtr<nsGeolocationRequest> > mWatchingCallbacks;

  
  nsWeakPtr mOwner;

  
  nsCOMPtr<nsIURI> mURI;

  
  nsRefPtr<nsGeolocationService> mService;
};

#endif 
