



































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

  nsCOMPtr<nsIGeolocationUpdate> mCallback;

  LocationGPSDevice *mGPSDevice;

  gulong mCallbackChanged;

  PRBool mHasSeenLocation;
  PRTime mLastSeenTime;

};
