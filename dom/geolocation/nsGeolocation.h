



#ifndef nsGeoLocation_h
#define nsGeoLocation_h


#undef CreateEvent

#include "mozilla/StaticPtr.h"
#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "nsTArray.h"
#include "nsITimer.h"
#include "nsIObserver.h"
#include "nsWrapperCache.h"

#include "nsWeakPtr.h"
#include "nsCycleCollectionParticipant.h"

#include "nsGeoPosition.h"
#include "nsIDOMGeoGeolocation.h"
#include "nsIDOMGeoPosition.h"
#include "nsIDOMGeoPositionError.h"
#include "nsIDOMGeoPositionCallback.h"
#include "nsIDOMGeoPositionErrorCallback.h"
#include "mozilla/dom/GeolocationBinding.h"
#include "mozilla/dom/PositionErrorBinding.h"
#include "mozilla/dom/CallbackObject.h"

#include "nsIGeolocationProvider.h"
#include "nsIContentPermissionPrompt.h"
#include "nsIDOMWindow.h"
#include "mozilla/Attributes.h"

class nsGeolocationService;
class nsGeolocationRequest;

namespace mozilla {
namespace dom {
class Geolocation;
typedef CallbackObjectHolder<PositionCallback, nsIDOMGeoPositionCallback> GeoPositionCallback;
typedef CallbackObjectHolder<PositionErrorCallback, nsIDOMGeoPositionErrorCallback> GeoPositionErrorCallback;
}
}

struct CachedPositionAndAccuracy {
  nsCOMPtr<nsIDOMGeoPosition> position;
  bool isHighAccuracy;
};




class nsGeolocationService MOZ_FINAL : public nsIGeolocationUpdate, public nsIObserver
{
public:

  static already_AddRefed<nsGeolocationService> GetGeolocationService();
  static mozilla::StaticRefPtr<nsGeolocationService> sService;

  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIGEOLOCATIONUPDATE
  NS_DECL_NSIOBSERVER

  nsGeolocationService() {
      mHigherAccuracy = false;
  }

  nsresult Init();

  void HandleMozsettingChanged(nsISupports* aSubject);
  void HandleMozsettingValue(const bool aValue);

  
  void AddLocator(mozilla::dom::Geolocation* locator);
  void RemoveLocator(mozilla::dom::Geolocation* locator);

  void SetCachedPosition(nsIDOMGeoPosition* aPosition);
  CachedPositionAndAccuracy GetCachedPosition();

  
  nsresult StartDevice(nsIPrincipal* aPrincipal);

  
  void     StopDevice();

  
  void     SetDisconnectTimer();

  
  void     UpdateAccuracy(bool aForceHigh = false);
  bool     HighAccuracyRequested();

private:

  ~nsGeolocationService();

  
  
  
  nsCOMPtr<nsITimer> mDisconnectTimer;

  
  nsCOMPtr<nsIGeolocationProvider> mProvider;

  
  
  
  nsTArray<mozilla::dom::Geolocation*> mGeolocators;

  
  CachedPositionAndAccuracy mLastPosition;

  
  bool mHigherAccuracy;
};

namespace mozilla {
namespace dom {




class Geolocation MOZ_FINAL : public nsIDOMGeoGeolocation,
                              public nsIGeolocationUpdate,
                              public nsWrapperCache
{
public:

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS_AMBIGUOUS(Geolocation, nsIDOMGeoGeolocation)

  NS_DECL_NSIGEOLOCATIONUPDATE
  NS_DECL_NSIDOMGEOGEOLOCATION

  Geolocation();

  nsresult Init(nsIDOMWindow* contentDom=nullptr);

  nsIDOMWindow* GetParentObject() const;
  virtual JSObject* WrapObject(JSContext *aCtx) MOZ_OVERRIDE;

  int32_t WatchPosition(PositionCallback& aCallback, PositionErrorCallback* aErrorCallback, const PositionOptions& aOptions, ErrorResult& aRv);
  void GetCurrentPosition(PositionCallback& aCallback, PositionErrorCallback* aErrorCallback, const PositionOptions& aOptions, ErrorResult& aRv);

  
  bool HasActiveCallbacks();

  
  void NotifyAllowedRequest(nsGeolocationRequest* aRequest);

  
  void RemoveRequest(nsGeolocationRequest* request);

  
  void Shutdown();

  
  nsIPrincipal* GetPrincipal() { return mPrincipal; }

  
  nsIWeakReference* GetOwner() { return mOwner; }

  
  bool WindowOwnerStillExists();

  
  bool HighAccuracyRequested();

  
  void ServiceReady();

private:

  ~Geolocation();

  nsresult GetCurrentPosition(GeoPositionCallback& aCallback, GeoPositionErrorCallback& aErrorCallback, PositionOptions* aOptions);
  nsresult WatchPosition(GeoPositionCallback& aCallback, GeoPositionErrorCallback& aErrorCallback, PositionOptions* aOptions, int32_t* aRv);

  bool RegisterRequestWithPrompt(nsGeolocationRequest* request);

  
  nsresult GetCurrentPositionReady(nsGeolocationRequest* aRequest);
  nsresult WatchPositionReady(nsGeolocationRequest* aRequest);

  
  
  
  
  

  nsTArray<nsRefPtr<nsGeolocationRequest> > mPendingCallbacks;
  nsTArray<nsRefPtr<nsGeolocationRequest> > mWatchingCallbacks;

  
  nsWeakPtr mOwner;

  
  nsCOMPtr<nsIPrincipal> mPrincipal;

  
  nsRefPtr<nsGeolocationService> mService;

  
  uint32_t mLastWatchId;

  
  nsTArray<nsRefPtr<nsGeolocationRequest> > mPendingRequests;
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

  virtual JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE;

  int16_t Code() const {
    return mCode;
  }

  void NotifyCallback(const GeoPositionErrorCallback& callback);
private:
  ~PositionError();
  int16_t mCode;
  nsRefPtr<Geolocation> mParent;
};

}

inline nsISupports*
ToSupports(dom::Geolocation* aGeolocation)
{
  return ToSupports(static_cast<nsIDOMGeoGeolocation*>(aGeolocation));
}
}

#endif 
