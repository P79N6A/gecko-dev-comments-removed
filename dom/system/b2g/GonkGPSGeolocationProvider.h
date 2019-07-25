



































#ifndef GonkGPSGeolocationProvider_h
#define GonkGPSGeolocationProvider_h

#include <hardware/gps.h> 
#include "nsIGeolocationProvider.h"

class GonkGPSGeolocationProvider : public nsIGeolocationProvider
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIGEOLOCATIONPROVIDER

  static already_AddRefed<GonkGPSGeolocationProvider> GetSingleton();

  already_AddRefed<nsIGeolocationUpdate> GetLocationCallback();

private:

  
  GonkGPSGeolocationProvider();
  GonkGPSGeolocationProvider(const GonkGPSGeolocationProvider &);
  GonkGPSGeolocationProvider & operator = (const GonkGPSGeolocationProvider &);
  ~GonkGPSGeolocationProvider();

  const GpsInterface* GetGPSInterface();

  static GonkGPSGeolocationProvider* sSingleton;

  bool mStarted;
  const GpsInterface* mGpsInterface;
  nsCOMPtr<nsIGeolocationUpdate> mLocationCallback;
};

#endif 
