




































#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "nsCOMArray.h"
#include "nsTArray.h"
#include "nsITimer.h"
#include "nsIObserver.h"

#include "nsIDOMGeolocation.h"
#include "nsIDOMGeolocation.h"
#include "nsIDOMGeolocationCallback.h"
#include "nsIDOMGeolocator.h"
#include "nsIDOMNavigatorGeolocator.h"

#include "nsPIDOMWindow.h"

#include "nsIGeolocationProvider.h"

class nsGeolocator;
class nsGeolocatorService;


class nsGeolocationRequest : public nsIGeolocationRequest
{
 public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIGEOLOCATIONREQUEST

  nsGeolocationRequest(nsGeolocator* locator, nsIDOMGeolocationCallback* callback);

  void SetURI();
  void SetOwner();
  PRBool OwnerStillExists();

  void SendLocation(nsIDOMGeolocation* location);
  void MarkCleared();
  PRBool Allowed() {return mAllowed;}

  ~nsGeolocationRequest();

 private:
  PRBool mAllowed;
  PRBool mCleared;
  PRBool mFuzzLocation;

  nsCOMPtr<nsIDOMGeolocationCallback> mCallback;
  nsCOMPtr<nsPIDOMWindow> mOwner;
  nsCOMPtr<nsIURI> mURI;
  nsGeolocator* mLocator; 
};



 
class nsGeolocation : public nsIDOMGeolocation
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMGEOLOCATION

    nsGeolocation(double aLat, double aLong, double aAlt, double aHError, double aVError, long long aTimestamp)
    : mLat(aLat), mLong(aLong), mAlt(aAlt), mHError(aHError), mVError(aVError), mTimestamp(aTimestamp){};

private:
  ~nsGeolocation(){}
  double mLat, mLong, mAlt, mHError, mVError;
  long long mTimestamp;
};




class nsGeolocatorService : public nsIGeolocationService, public nsIGeolocationUpdate, public nsIObserver
{
public:
  static already_AddRefed<nsGeolocatorService> GetGeolocationService();
  static nsGeolocatorService* gService;

  NS_DECL_ISUPPORTS
  NS_DECL_NSIGEOLOCATIONUPDATE
  NS_DECL_NSIOBSERVER
  NS_DECL_NSIGEOLOCATIONSERVICE

  nsGeolocatorService();

  
  void AddLocator(nsGeolocator* locator);
  void RemoveLocator(nsGeolocator* locator);

  
  already_AddRefed<nsIDOMGeolocation> GetLastKnownPosition();
  
  
  nsIGeolocationPrompt* GetPrompt() { return mPrompt; };

  
  
  PRBool   IsDeviceReady();

  
  nsresult StartDevice();

  
  void     StopDevice();
  
  
  void     SetDisconnectTimer();

private:

  ~nsGeolocatorService();

  
  
  
  nsCOMPtr<nsITimer> mDisconnectTimer;

  
  PRInt32 mTimeout;

  
  nsCOMPtr<nsIGeolocationProvider> mProvider;

  
  
  
  nsTArray<nsGeolocator*> mGeolocators;

  
  nsCOMPtr<nsIGeolocationPrompt> mPrompt;
};




 
class nsGeolocator : public nsIDOMGeolocator
{
public:

  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMGEOLOCATOR

  nsGeolocator();

  
  void Update(nsIDOMGeolocation* aLocation);

  
  PRBool   HasActiveCallbacks();

  
  void     RemoveRequest(nsGeolocationRequest* request);

  
  void     Shutdown();

private:

  ~nsGeolocator();

  
  
  
  

  nsCOMArray<nsGeolocationRequest> mPendingCallbacks;
  nsCOMArray<nsGeolocationRequest> mWatchingCallbacks;

  PRBool mUpdateInProgress;

  
  nsRefPtr<nsGeolocatorService> mService;
};
