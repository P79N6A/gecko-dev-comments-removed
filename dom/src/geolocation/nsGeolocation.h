



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
#include "nsIGeolocation.h"

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
                       mozilla::idl::GeoPositionOptions* aOptions,
                       bool watchPositionRequest = false,
                       int32_t watchId = 0);
  void Shutdown();

  
  
  bool Update(nsIDOMGeoPosition* aPosition, bool aIsBetter);

  void SendLocation(nsIDOMGeoPosition* location);
  void MarkCleared();
  bool WantsHighAccuracy() {return mOptions && mOptions->enableHighAccuracy;}
  bool IsActive() {return !mCleared;}
  bool Allowed() {return mAllowed;}
  void SetTimeoutTimer();
  nsIPrincipal* GetPrincipal();

  ~nsGeolocationRequest();

  bool Recv__delete__(const bool& allow);
  void IPDLRelease() { Release(); }

  int32_t WatchId() { return mWatchId; }
 private:

  void NotifyError(int16_t errorCode);
  bool mAllowed;
  bool mCleared;
  bool mIsFirstUpdate;
  bool mIsWatchPositionRequest;

  nsCOMPtr<nsITimer> mTimeoutTimer;
  nsCOMPtr<nsIDOMGeoPositionCallback> mCallback;
  nsCOMPtr<nsIDOMGeoPositionErrorCallback> mErrorCallback;
  nsAutoPtr<mozilla::idl::GeoPositionOptions> mOptions;

  nsRefPtr<nsGeolocation> mLocator;

  int32_t mWatchId;
};




class nsGeolocationService MOZ_FINAL : public nsIGeolocationUpdate, public nsIObserver
{
public:

  static already_AddRefed<nsGeolocationService> GetGeolocationService();
  static nsRefPtr<nsGeolocationService> sService;

  NS_DECL_ISUPPORTS
  NS_DECL_NSIGEOLOCATIONUPDATE
  NS_DECL_NSIOBSERVER

  nsGeolocationService() {
      mHigherAccuracy = false;
  }

  nsresult Init();

  void HandleMozsettingChanged(const PRUnichar* aData);
  void HandleMozsettingValue(const bool aValue);

  
  void AddLocator(nsGeolocation* locator);
  void RemoveLocator(nsGeolocation* locator);

  void SetCachedPosition(nsIDOMGeoPosition* aPosition);
  nsIDOMGeoPosition* GetCachedPosition();
  PRBool IsBetterPosition(nsIDOMGeoPosition *aSomewhere);

  
  nsresult StartDevice(nsIPrincipal* aPrincipal, bool aRequestPrivate);

  
  void     StopDevice();

  
  void     SetDisconnectTimer();

  
  void     SetHigherAccuracy(bool aEnable);
  bool     HighAccuracyRequested();

private:

  ~nsGeolocationService();

  
  
  
  nsCOMPtr<nsITimer> mDisconnectTimer;

  
  nsCOMArray<nsIGeolocationProvider> mProviders;

  
  
  
  nsTArray<nsGeolocation*> mGeolocators;

  
  nsCOMPtr<nsIDOMGeoPosition> mLastPosition;

  
  bool mHigherAccuracy;
};





class nsGeolocation MOZ_FINAL : public nsIDOMGeoGeolocation,
                                public nsIGeolocation
{
public:

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_NSIDOMGEOGEOLOCATION
  NS_DECL_NSIGEOLOCATION

  NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(nsGeolocation, nsIDOMGeoGeolocation)

  nsGeolocation();

  nsresult Init(nsIDOMWindow* contentDom=nullptr);

  
  void Update(nsIDOMGeoPosition* aPosition, bool aIsBetter);

  
  bool HasActiveCallbacks();

  
  void RemoveRequest(nsGeolocationRequest* request);

  
  void Shutdown();

  
  nsIPrincipal* GetPrincipal() { return mPrincipal; }

  
  nsIWeakReference* GetOwner() { return mOwner; }

  
  bool WindowOwnerStillExists();

  
  bool HighAccuracyRequested();

  
  void ServiceReady();

private:

  ~nsGeolocation();

  bool RegisterRequestWithPrompt(nsGeolocationRequest* request);

  
  nsresult GetCurrentPositionReady(nsGeolocationRequest* aRequest);
  nsresult WatchPositionReady(nsGeolocationRequest* aRequest);

  
  
  
  

  nsTArray<nsRefPtr<nsGeolocationRequest> > mPendingCallbacks;
  nsTArray<nsRefPtr<nsGeolocationRequest> > mWatchingCallbacks;

  
  nsWeakPtr mOwner;

  
  nsCOMPtr<nsIPrincipal> mPrincipal;

  
  nsRefPtr<nsGeolocationService> mService;

  
  uint32_t mLastWatchId;

  
  class PendingRequest
  {
  public:
    nsRefPtr<nsGeolocationRequest> request;
    enum {
      GetCurrentPosition,
      WatchPosition
    } type;
  };

  nsTArray<PendingRequest> mPendingRequests;
};

#endif 
