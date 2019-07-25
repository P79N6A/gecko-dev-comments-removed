



#ifndef GonkGPSGeolocationProvider_h
#define GonkGPSGeolocationProvider_h

#include <hardware/gps.h> 
#include "nsIGeolocationProvider.h"

class nsIThread;

class GonkGPSGeolocationProvider : public nsIGeolocationProvider
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIGEOLOCATIONPROVIDER

  static already_AddRefed<GonkGPSGeolocationProvider> GetSingleton();

private:

  
  GonkGPSGeolocationProvider();
  GonkGPSGeolocationProvider(const GonkGPSGeolocationProvider &);
  GonkGPSGeolocationProvider & operator = (const GonkGPSGeolocationProvider &);
  ~GonkGPSGeolocationProvider();

  static void LocationCallback(GpsLocation* location);
  static void StatusCallback(GpsStatus* status);
  static void SvStatusCallback(GpsSvStatus* sv_info);
  static void NmeaCallback(GpsUtcTime timestamp, const char* nmea, int length);
  static void SetCapabilitiesCallback(uint32_t capabilities);
  static void AcquireWakelockCallback();
  static void ReleaseWakelockCallback();
  static pthread_t CreateThreadCallback(const char* name, void (*start)(void*), void* arg);
  static void RequestUtcTimeCallback();

  static GpsCallbacks mCallbacks;

  void Init();
  void StartGPS();
  void ShutdownNow();

  const GpsInterface* GetGPSInterface();

  static GonkGPSGeolocationProvider* sSingleton;

  bool mStarted;
  const GpsInterface* mGpsInterface;
  nsCOMPtr<nsIGeolocationUpdate> mLocationCallback;
  nsCOMPtr<nsIThread> mInitThread;
};

#endif 
