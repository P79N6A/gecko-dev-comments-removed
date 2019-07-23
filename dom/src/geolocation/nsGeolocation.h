




































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

class nsGeolocationService;
class nsGeolocation;

class nsGeolocationRequest : public nsIGeolocationRequest
{
 public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIGEOLOCATIONREQUEST

  nsGeolocationRequest(nsGeolocation* locator,
                       nsIDOMGeoPositionCallback* callback,
                       nsIDOMGeoPositionErrorCallback* errorCallback);
  void Shutdown();

  void SendLocation(nsIDOMGeoPosition* location);
  void MarkCleared();
  PRBool Allowed() {return mAllowed;}

  ~nsGeolocationRequest();

 private:
  PRBool mAllowed;
  PRBool mCleared;
  PRBool mFuzzLocation;

  nsCOMPtr<nsIDOMGeoPositionCallback> mCallback;
  nsCOMPtr<nsIDOMGeoPositionErrorCallback> mErrorCallback;

  nsGeolocation* mLocator; 
};



 
class nsGeoPosition : public nsIDOMGeoPosition
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMGEOPOSITION

    nsGeoPosition(double aLat, double aLong, double aAlt, double aHError, double aVError, double aHeading, double aVelocity, long long aTimestamp)
    : mLat(aLat), mLong(aLong), mAlt(aAlt), mHError(aHError), mVError(aVError), mHeading(aHeading), mVelocity(aVelocity), mTimestamp(aTimestamp){};

private:
  ~nsGeoPosition(){}
  double mLat, mLong, mAlt, mHError, mVError, mHeading, mVelocity;
  long long mTimestamp;
};




class nsGeolocationService : public nsIGeolocationService, public nsIGeolocationUpdate, public nsIObserver
{
public:

  static nsGeolocationService* GetGeolocationService();
  static nsGeolocationService* GetInstance();  
  static nsGeolocationService* gService;

  NS_DECL_ISUPPORTS
  NS_DECL_NSIGEOLOCATIONUPDATE
  NS_DECL_NSIOBSERVER
  NS_DECL_NSIGEOLOCATIONSERVICE

  nsGeolocationService();

  
  void AddLocator(nsGeolocation* locator);
  void RemoveLocator(nsGeolocation* locator);

  
  already_AddRefed<nsIDOMGeoPosition> GetLastKnownPosition();
  
  
  nsIGeolocationPrompt* GetPrompt() { return mPrompt; } 

  
  
  PRBool   IsDeviceReady();

  
  nsresult StartDevice();

  
  void     StopDevice();
  
  
  void     SetDisconnectTimer();

private:

  ~nsGeolocationService();

  
  
  
  nsCOMPtr<nsITimer> mDisconnectTimer;

  
  PRInt32 mTimeout;

  
  nsCOMPtr<nsIGeolocationProvider> mProvider;

  
  
  
  nsTArray<nsGeolocation*> mGeolocators;

  
  nsCOMPtr<nsIGeolocationPrompt> mPrompt;
};




 
class nsGeolocation : public nsIDOMGeoGeolocation
{
public:

  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMGEOGEOLOCATION

  nsGeolocation(nsIDOMWindow* contentDom);

  
  void Update(nsIDOMGeoPosition* aPosition);

  
  PRBool   HasActiveCallbacks();

  
  void     RemoveRequest(nsGeolocationRequest* request);

  
  void     Shutdown();

  
  nsIURI* GetURI() { return mURI; }

  
  nsIDOMWindow* GetOwner() { return mOwner; }

  
  PRBool OwnerStillExists();

private:

  ~nsGeolocation();

  
  
  
  

  nsCOMArray<nsGeolocationRequest> mPendingCallbacks;
  nsCOMArray<nsGeolocationRequest> mWatchingCallbacks;

  PRBool mUpdateInProgress;

  
  nsPIDOMWindow* mOwner;

  
  nsCOMPtr<nsIURI> mURI;

  
  nsRefPtr<nsGeolocationService> mService;
};
