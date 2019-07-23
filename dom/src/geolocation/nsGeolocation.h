




































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

class nsGeolocationService;
class nsGeolocation;

class nsGeolocationRequest : public nsIGeolocationRequest, public nsITimerCallback
{
 public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_NSIGEOLOCATIONREQUEST
  NS_DECL_NSITIMERCALLBACK

  NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(nsGeolocationRequest, nsIGeolocationRequest)

  nsGeolocationRequest(nsGeolocation* locator,
                       nsIDOMGeoPositionCallback* callback,
                       nsIDOMGeoPositionErrorCallback* errorCallback,
                       nsIDOMGeoPositionOptions* options);
  nsresult Init();
  void Shutdown();

  void SendLocation(nsIDOMGeoPosition* location);
  void MarkCleared();
  PRBool Allowed() {return mAllowed;}

  ~nsGeolocationRequest();

 private:

  void NotifyError(PRInt16 errorCode);
  PRPackedBool mAllowed;
  PRPackedBool mCleared;
  PRPackedBool mHasSentData;

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

  PRBool IsBetterPosition(nsIDOMGeoPosition* aPosition);

  void SetCachedPosition(nsIDOMGeoPosition* aPosition);
  nsIDOMGeoPosition* GetCachedPosition();

  
  PRBool   HasGeolocationProvider();

  
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

  nsGeolocation(nsIDOMWindow* contentDom);

  
  void Update(nsIDOMGeoPosition* aPosition);

  
  PRBool HasActiveCallbacks();

  
  void RemoveRequest(nsGeolocationRequest* request);

  
  void Shutdown();

  
  nsIURI* GetURI() { return mURI; }

  
  nsIWeakReference* GetOwner() { return mOwner; }

  
  PRBool OwnerStillExists();

private:

  ~nsGeolocation();

  
  
  
  

  nsTArray<nsRefPtr<nsGeolocationRequest> > mPendingCallbacks;
  nsTArray<nsRefPtr<nsGeolocationRequest> > mWatchingCallbacks;

  PRBool mUpdateInProgress;

  
  nsWeakPtr mOwner;

  
  nsCOMPtr<nsIURI> mURI;

  
  nsRefPtr<nsGeolocationService> mService;
};
