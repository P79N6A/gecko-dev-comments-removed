




































#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "nsCOMArray.h"
#include "nsTArray.h"
#include "nsITimer.h"
#include "nsIObserver.h"
#include "nsIURI.h"

#include "nsIDOMGeoGeolocation.h"
#include "nsIDOMGeoPosition.h"
#include "nsIDOMGeoPositionError.h"
#include "nsIDOMGeoPositionCallback.h"
#include "nsIDOMGeoPositionErrorCallback.h"
#include "nsIDOMGeoPositionOptions.h"
#include "nsIDOMNavigatorGeolocation.h"

#include "nsPIDOMWindow.h"

#include "nsIGeolocationProvider.h"

#define NS_GEO_ERROR_CODE_PERMISSION_ERROR        1
#define NS_GEO_ERROR_CODE_LOCATION_PROVIDER_ERROR 2
#define NS_GEO_ERROR_CODE_POSITION_NOT_FOUND      3
#define NS_GEO_ERROR_CODE_TIMEOUT                 4

class nsGeolocationService;
class nsGeolocation;

class nsGeolocationRequest : public nsIGeolocationRequest, public nsITimerCallback
{
 public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIGEOLOCATIONREQUEST
  NS_DECL_NSITIMERCALLBACK
 
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
  PRPackedBool mFuzzLocation;
  PRPackedBool mHasSentData;

  nsCOMPtr<nsITimer> mTimeoutTimer;
  nsCOMPtr<nsIDOMGeoPositionCallback> mCallback;
  nsCOMPtr<nsIDOMGeoPositionErrorCallback> mErrorCallback;
  nsCOMPtr<nsIDOMGeoPositionOptions> mOptions;

  nsGeolocation* mLocator; 

};



 
class nsGeoPosition : public nsIDOMGeoPosition
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMGEOPOSITION

    nsGeoPosition(double aLat, double aLong, double aAlt, double aHError, double aVError, double aHeading, double aSpeed, long long aTimestamp)
    : mLat(aLat), mLong(aLong), mAlt(aAlt), mHError(aHError), mVError(aVError), mHeading(aHeading), mSpeed(aSpeed), mTimestamp(aTimestamp){};

private:
  ~nsGeoPosition(){}
  double mLat, mLong, mAlt, mHError, mVError, mHeading, mSpeed;
  long long mTimestamp;
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

  nsGeolocationService();

  
  void AddLocator(nsGeolocation* locator);
  void RemoveLocator(nsGeolocation* locator);

  
  already_AddRefed<nsIDOMGeoPosition> GetLastKnownPosition();
  
  
  
  PRBool   IsDeviceReady();

  
  PRBool   HasGeolocationProvider();

  
  nsresult StartDevice();

  
  void     StopDevice();
  
  
  void     SetDisconnectTimer();

private:

  ~nsGeolocationService();

  
  
  
  nsCOMPtr<nsITimer> mDisconnectTimer;

  
  PRInt32 mTimeout;

  
  nsCOMPtr<nsIGeolocationProvider> mProvider;

  
  PRBool mProviderStarted;

  
  
  
  nsTArray<nsGeolocation*> mGeolocators;
};




 
class nsGeolocation : public nsIDOMGeoGeolocation
{
public:

  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMGEOGEOLOCATION

  nsGeolocation(nsIDOMWindow* contentDom);

  
  void Update(nsIDOMGeoPosition* aPosition);

  
  PRBool HasActiveCallbacks();

  
  void RemoveRequest(nsGeolocationRequest* request);

  
  void Shutdown();

  
  nsIURI* GetURI() { return mURI; }

  
  nsIDOMWindow* GetOwner() { return mOwner; }

  
  PRBool OwnerStillExists();

private:

  ~nsGeolocation();

  
  
  
  

  nsTArray<nsRefPtr<nsGeolocationRequest> > mPendingCallbacks;
  nsTArray<nsRefPtr<nsGeolocationRequest> > mWatchingCallbacks;

  PRBool mUpdateInProgress;

  
  nsPIDOMWindow* mOwner;

  
  nsCOMPtr<nsIURI> mURI;

  
  nsRefPtr<nsGeolocationService> mService;
};
