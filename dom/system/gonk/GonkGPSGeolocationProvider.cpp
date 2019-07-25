




































#include <pthread.h>
#include <hardware/gps.h>

#include "mozilla/Preferences.h"
#include "nsGeoPosition.h"
#include "GonkGPSGeolocationProvider.h"

using namespace mozilla;

NS_IMPL_ISUPPORTS1(GonkGPSGeolocationProvider, nsIGeolocationProvider)

GonkGPSGeolocationProvider* GonkGPSGeolocationProvider::sSingleton;

static void
LocationCallback(GpsLocation* location)
{
  nsRefPtr<GonkGPSGeolocationProvider> provider =
    GonkGPSGeolocationProvider::GetSingleton();
  nsCOMPtr<nsIGeolocationUpdate> callback = provider->GetLocationCallback();
  
  if (!callback)
    return;

  nsRefPtr<nsGeoPosition> somewhere = new nsGeoPosition(location->latitude,
                                                        location->longitude,
                                                        location->altitude,
                                                        location->accuracy,
                                                        location->accuracy,
                                                        location->bearing,
                                                        location->speed,
                                                        location->timestamp);
  callback->Update(somewhere);
}

typedef void *(*pthread_func)(void *);



static pthread_t
CreateThreadCallback(const char* name, void (*start)(void *), void* arg)
{
  pthread_t thread;
  pthread_attr_t attr;

  pthread_attr_init(&attr);

  


  pthread_create(&thread, &attr, reinterpret_cast<pthread_func>(start), arg);

  return thread;
}

static GpsCallbacks gCallbacks = {
  sizeof(GpsCallbacks),
  LocationCallback,
  NULL, 
  NULL, 
  NULL, 
  NULL, 
  NULL, 
  NULL, 
  CreateThreadCallback,
};

GonkGPSGeolocationProvider::GonkGPSGeolocationProvider()
  : mStarted(false)
{
  mGpsInterface = GetGPSInterface();
}

GonkGPSGeolocationProvider::~GonkGPSGeolocationProvider()
{
  Shutdown();
  sSingleton = NULL;
}

 already_AddRefed<GonkGPSGeolocationProvider>
GonkGPSGeolocationProvider::GetSingleton()
{
  if (!sSingleton)
    sSingleton = new GonkGPSGeolocationProvider();

  NS_ADDREF(sSingleton);
  return sSingleton;
}

already_AddRefed<nsIGeolocationUpdate>
GonkGPSGeolocationProvider::GetLocationCallback()
{
  nsCOMPtr<nsIGeolocationUpdate> callback = mLocationCallback;
  return callback.forget();
}

const GpsInterface*
GonkGPSGeolocationProvider::GetGPSInterface()
{
  hw_module_t* module;

  if (hw_get_module(GPS_HARDWARE_MODULE_ID, (hw_module_t const**)&module))
    return NULL;

  hw_device_t* device;
  if (module->methods->open(module, GPS_HARDWARE_MODULE_ID, &device))
    return NULL;

  gps_device_t* gps_device = (gps_device_t *)device;
  return gps_device->get_gps_interface(gps_device);
}

NS_IMETHODIMP
GonkGPSGeolocationProvider::Startup()
{
  if (mStarted)
    return NS_OK;

  NS_ENSURE_TRUE(mGpsInterface, NS_ERROR_FAILURE);

  PRInt32 update = Preferences::GetInt("geo.default.update", 1000);

  mGpsInterface->init(&gCallbacks);
  mGpsInterface->start();
  mGpsInterface->set_position_mode(GPS_POSITION_MODE_STANDALONE,
                                   GPS_POSITION_RECURRENCE_PERIODIC,
                                   update, 0, 0);
  return NS_OK;
}

NS_IMETHODIMP
GonkGPSGeolocationProvider::Watch(nsIGeolocationUpdate* aCallback)
{
  mLocationCallback = aCallback;

  return NS_OK;
}

NS_IMETHODIMP
GonkGPSGeolocationProvider::Shutdown()
{
  if (!mStarted)
    return NS_OK;

  mGpsInterface->stop();
  mGpsInterface->cleanup();

  return NS_OK;
}
