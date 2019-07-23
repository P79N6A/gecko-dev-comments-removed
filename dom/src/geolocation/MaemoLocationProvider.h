



































#include "nsIGeolocationProvider.h"
#include "nsIDOMGeoPosition.h"
#include "nsCOMPtr.h"

#include <glib.h>
#include <errno.h>
#include <gpsbt.h>
#include <gpsmgr.h>

extern "C" {
  
  
  #include <location/location-gps-device.h>
  #include <location/location-gpsd-control.h>
}

class MaemoLocationProvider : public nsIGeolocationProvider
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIGEOLOCATIONPROVIDER

  MaemoLocationProvider();

  void Update(nsIDOMGeoPosition* aPosition);

private:
  ~MaemoLocationProvider();

  nsCOMPtr<nsIDOMGeoPosition> mLastPosition;

  nsIGeolocationUpdate* mCallback; 


  gpsbt_t mGPSBT;
  LocationGPSDevice *mGPSDevice;
  gulong mLocationCallbackHandle;
  PRBool mHasSeenLocation;
};
