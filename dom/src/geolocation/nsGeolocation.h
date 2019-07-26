



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
#include "nsWrapperCache.h"

#include "nsWeakPtr.h"
#include "nsCycleCollectionParticipant.h"

#include "nsGeoPosition.h"
#include "nsIDOMGeoGeolocation.h"
#include "nsIDOMGeoPosition.h"
#include "nsIDOMGeoPositionError.h"
#include "nsIDOMGeoPositionCallback.h"
#include "nsIDOMGeoPositionErrorCallback.h"
#include "nsIDOMNavigatorGeolocation.h"
#include "mozilla/dom/GeolocationBinding.h"
#include "mozilla/dom/PositionErrorBinding.h"
#include "mozilla/dom/CallbackObject.h"

#include "nsPIDOMWindow.h"

#include "nsIGeolocationProvider.h"
#include "nsIContentPermissionPrompt.h"
#include "DictionaryHelpers.h"
#include "PCOMContentPermissionRequestChild.h"
#include "mozilla/Attributes.h"

class nsGeolocationService;

namespace mozilla {
namespace dom {
class Geolocation;
typedef CallbackObjectHolder<PositionCallback, nsIDOMGeoPositionCallback> GeoPositionCallback;
typedef CallbackObjectHolder<PositionErrorCallback, nsIDOMGeoPositionErrorCallback> GeoPositionErrorCallback;
}
}

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

  nsGeolocationRequest(mozilla::dom::Geolocation* locator,
                       const mozilla::dom::GeoPositionCallback& callback,
                       const mozilla::dom::GeoPositionErrorCallback& errorCallback,
                       mozilla::idl::GeoPositionOptions* aOptions,
                       bool watchPositionRequest = false,
                       int32_t watchId = 0);
  void Shutdown();

  
  
  bool Update(nsIDOMGeoPosition* aPosition, bool aIsBetter);

  void SendLocation(nsIDOMGeoPosition* location, bool aCachePosition);
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
  mozilla::dom::GeoPositionCallback mCallback;
  mozilla::dom::GeoPositionErrorCallback mErrorCallback;
  nsAutoPtr<mozilla::idl::GeoPositionOptions> mOptions;

  nsRefPtr<mozilla::dom::Geolocation> mLocator;

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

  
  void AddLocator(mozilla::dom::Geolocation* locator);
  void RemoveLocator(mozilla::dom::Geolocation* locator);

  void SetCachedPosition(nsIDOMGeoPosition* aPosition);
  nsIDOMGeoPosition* GetCachedPosition();
  bool IsBetterPosition(nsIDOMGeoPosition *aSomewhere);

  
  nsresult StartDevice(nsIPrincipal* aPrincipal, bool aRequestPrivate);

  
  void     StopDevice();

  
  void     SetDisconnectTimer();

  
  void     SetHigherAccuracy(bool aEnable);
  bool     HighAccuracyRequested();

private:

  ~nsGeolocationService();

  
  
  
  nsCOMPtr<nsITimer> mDisconnectTimer;

  
  nsCOMArray<nsIGeolocationProvider> mProviders;

  
  
  
  nsTArray<mozilla::dom::Geolocation*> mGeolocators;

  
  nsCOMPtr<nsIDOMGeoPosition> mLastPosition;

  
  bool mHigherAccuracy;
};

namespace mozilla {
namespace dom {




class Geolocation MOZ_FINAL : public nsIDOMGeoGeolocation,
                                public nsWrapperCache
{
public:

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(Geolocation)

  NS_DECL_NSIDOMGEOGEOLOCATION

  Geolocation();

  nsresult Init(nsIDOMWindow* contentDom=nullptr);

  nsIDOMWindow* GetParentObject() const;
  virtual JSObject* WrapObject(JSContext *aCtx,
                               JS::Handle<JSObject*> aScope) MOZ_OVERRIDE;

  int32_t WatchPosition(PositionCallback& aCallback, PositionErrorCallback* aErrorCallback, const PositionOptions& aOptions, ErrorResult& aRv);
  void GetCurrentPosition(PositionCallback& aCallback, PositionErrorCallback* aErrorCallback, const PositionOptions& aOptions, ErrorResult& aRv);

  
  void Update(nsIDOMGeoPosition* aPosition, bool aIsBetter);

  void SetCachedPosition(Position* aPosition);
  Position* GetCachedPosition();

  
  bool HasActiveCallbacks();

  
  void RemoveRequest(nsGeolocationRequest* request);

  
  void Shutdown();

  
  nsIPrincipal* GetPrincipal() { return mPrincipal; }

  
  nsIWeakReference* GetOwner() { return mOwner; }

  
  bool WindowOwnerStillExists();

  
  bool HighAccuracyRequested();

  
  void ServiceReady();

private:

  ~Geolocation();

  nsresult GetCurrentPosition(GeoPositionCallback& aCallback, GeoPositionErrorCallback& aErrorCallback, mozilla::idl::GeoPositionOptions* aOptions);
  nsresult WatchPosition(GeoPositionCallback& aCallback, GeoPositionErrorCallback& aErrorCallback, mozilla::idl::GeoPositionOptions* aOptions, int32_t* aRv);

  bool RegisterRequestWithPrompt(nsGeolocationRequest* request);

  
  nsresult GetCurrentPositionReady(nsGeolocationRequest* aRequest);
  nsresult WatchPositionReady(nsGeolocationRequest* aRequest);

  
  
  
  

  nsTArray<nsRefPtr<nsGeolocationRequest> > mPendingCallbacks;
  nsTArray<nsRefPtr<nsGeolocationRequest> > mWatchingCallbacks;

  
  nsWeakPtr mOwner;

  
  nsCOMPtr<nsIPrincipal> mPrincipal;

  
  nsRefPtr<nsGeolocationService> mService;

  
  nsRefPtr<Position> mCachedPosition;

  
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

class PositionError MOZ_FINAL : public nsIDOMGeoPositionError,
                                public nsWrapperCache
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(PositionError)

  NS_DECL_NSIDOMGEOPOSITIONERROR

  PositionError(Geolocation* aParent, int16_t aCode);

  Geolocation* GetParentObject() const;

  virtual JSObject* WrapObject(JSContext* aCx,
                               JS::Handle<JSObject*> aScope) MOZ_OVERRIDE;

  int16_t Code() const {
    return mCode;
  }

  void GetMessage(nsString& aRetVal) const {
    aRetVal.Truncate();
  }

  void NotifyCallback(const GeoPositionErrorCallback& callback);
private:
  ~PositionError();
  int16_t mCode;
  nsRefPtr<Geolocation> mParent;
};

}
}

#endif 
