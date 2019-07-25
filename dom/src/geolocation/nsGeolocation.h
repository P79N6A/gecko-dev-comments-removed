



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
#include "nsIDOMNavigatorGeolocation.h"

#include "nsPIDOMWindow.h"

#include "nsIGeolocationProvider.h"
#include "nsIContentPermissionPrompt.h"
#include "DictionaryHelpers.h"
#include "PCOMContentPermissionRequestChild.h"
#include "mozilla/Attributes.h"

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
                       bool watchPositionRequest = false);
  nsresult Init(JSContext* aCx, const jsval& aOptions);
  void Shutdown();

  
  bool Update(nsIDOMGeoPosition* aPosition);

  void SendLocation(nsIDOMGeoPosition* location);
  void MarkCleared();
  bool IsActive() {return !mCleared;}
  bool Allowed() {return mAllowed;}
  void SetTimeoutTimer();

  ~nsGeolocationRequest();

  bool Recv__delete__(const bool& allow);
  void IPDLRelease() { Release(); }

 private:

  void NotifyError(PRInt16 errorCode);
  bool mAllowed;
  bool mCleared;
  bool mIsWatchPositionRequest;

  nsCOMPtr<nsITimer> mTimeoutTimer;
  nsCOMPtr<nsIDOMGeoPositionCallback> mCallback;
  nsCOMPtr<nsIDOMGeoPositionErrorCallback> mErrorCallback;
  nsAutoPtr<mozilla::dom::GeoPositionOptions> mOptions;

  nsRefPtr<nsGeolocation> mLocator;
};




class nsGeolocationService MOZ_FINAL : public nsIGeolocationUpdate, public nsIObserver
{
public:

  static nsGeolocationService* GetGeolocationService();
  static nsGeolocationService* GetInstance();  
  static nsGeolocationService* gService;

  NS_DECL_ISUPPORTS
  NS_DECL_NSIGEOLOCATIONUPDATE
  NS_DECL_NSIOBSERVER

  nsGeolocationService() {
      mHigherAccuracy = false;
  }

  nsresult Init();

  void HandleMozsettingChanged(const PRUnichar* aData);

  
  void AddLocator(nsGeolocation* locator);
  void RemoveLocator(nsGeolocation* locator);

  void SetCachedPosition(nsIDOMGeoPosition* aPosition);
  nsIDOMGeoPosition* GetCachedPosition();

  
  nsresult StartDevice();

  
  void     StopDevice();
  
  
  void     SetDisconnectTimer();

  
  void     SetHigherAccuracy(bool aEnable);

private:

  ~nsGeolocationService();

  
  
  
  nsCOMPtr<nsITimer> mDisconnectTimer;

  
  nsCOMArray<nsIGeolocationProvider> mProviders;

  
  
  
  nsTArray<nsGeolocation*> mGeolocators;

  
  nsCOMPtr<nsIDOMGeoPosition> mLastPosition;

  
  bool mHigherAccuracy;
};




 
class nsGeolocation MOZ_FINAL : public nsIDOMGeoGeolocation
{
public:

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_NSIDOMGEOGEOLOCATION

  NS_DECL_CYCLE_COLLECTION_CLASS(nsGeolocation)

  nsGeolocation();

  nsresult Init(nsIDOMWindow* contentDom=nullptr);

  
  void Update(nsIDOMGeoPosition* aPosition);

  
  bool HasActiveCallbacks();

  
  void RemoveRequest(nsGeolocationRequest* request);

  
  void Shutdown();

  
  nsIURI* GetURI() { return mURI; }

  
  nsIWeakReference* GetOwner() { return mOwner; }

  
  bool WindowOwnerStillExists();

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
