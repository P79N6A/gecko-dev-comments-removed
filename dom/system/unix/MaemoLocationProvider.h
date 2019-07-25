





































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
#include "nsIDOMGeoPositionCoords.h"

#include "nsPIDOMWindow.h"

#include "nsIGeolocationProvider.h"


extern "C"
{
#include <location/location-gps-device.h>
#include <location/location-gpsd-control.h>
#include <location/location-distance-utils.h>
#include <location/location-misc.h>
}

class MaemoLocationProvider : public nsIGeolocationProvider,
                              public nsITimerCallback

{
 public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIGEOLOCATIONPROVIDER
  NS_DECL_NSITIMERCALLBACK

  MaemoLocationProvider();

  void Update(nsIDOMGeoPosition* aPosition);

 private:
  ~MaemoLocationProvider();

  nsresult StartControl();
  nsresult StartDevice();
  nsresult LocationUpdate(LocationGPSDevice* aDev);

  static void DeviceDisconnected(LocationGPSDevice* device, void* self);
  static void ControlStopped(LocationGPSDControl* device, void* self);
  static void ControlError(LocationGPSDControl* control, void* self);
  static void LocationChanged(LocationGPSDevice* device, void* self);

  gulong mLocationChanged;
  gulong mControlError;
  gulong mDeviceDisconnected;
  gulong mControlStopped;

  nsCOMPtr<nsIGeolocationUpdate> mCallback;
  bool mHasSeenLocation;
  bool mHasGPS;

  nsCOMPtr<nsITimer> mUpdateTimer;
  LocationGPSDControl* mGPSControl;
  LocationGPSDevice* mGPSDevice;

  bool mIgnoreMinorChanges;

  double mPrevLat;
  double mPrevLong;

  bool mIgnoreBigHErr;
  PRInt32 mMaxHErr;
  bool mIgnoreBigVErr;
  PRInt32 mMaxVErr;

};
