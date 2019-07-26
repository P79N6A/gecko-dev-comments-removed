















#ifndef GonkGPSGeolocationProvider_h
#define GonkGPSGeolocationProvider_h

#include <hardware/gps.h> 
#include "nsCOMPtr.h"
#include "nsIGeolocationProvider.h"
#include "nsIRadioInterfaceLayer.h"
#include "nsString.h"

class nsIThread;

class GonkGPSGeolocationProvider : public nsIGeolocationProvider
                                 , public nsIRILDataCallback
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIGEOLOCATIONPROVIDER
  NS_DECL_NSIRILDATACALLBACK

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
  static void AGPSStatusCallback(AGpsStatus* status);
  static void AGPSRILSetIDCallback(uint32_t flags);
  static void AGPSRILRefLocCallback(uint32_t flags);

  static GpsCallbacks mCallbacks;
  static AGpsCallbacks mAGPSCallbacks;
  static AGpsRilCallbacks mAGPSRILCallbacks;

  void Init();
  void SetupAGPS();
  void StartGPS();
  void ShutdownGPS();
  void RequestDataConnection();
  void ReleaseDataConnection();
  void RequestSetID(uint32_t flags);
  void SetReferenceLocation();

  const GpsInterface* GetGPSInterface();

  static GonkGPSGeolocationProvider* sSingleton;

  bool mStarted;

  bool mSupportsScheduling;
  bool mSupportsMSB;
  bool mSupportsMSA;
  bool mSupportsSingleShot;
  bool mSupportsTimeInjection;

  const GpsInterface* mGpsInterface;
  const AGpsInterface* mAGpsInterface;
  const AGpsRilInterface* mAGpsRilInterface;
  nsCOMPtr<nsIGeolocationUpdate> mLocationCallback;
  nsCOMPtr<nsIThread> mInitThread;
  nsCOMPtr<nsIRadioInterfaceLayer> mRIL;
  nsAutoString mCid;
};

#endif 
