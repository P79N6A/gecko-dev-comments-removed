





































#include "MaemoLocationProvider.h"
#include "nsGeolocation.h"

NS_IMPL_ISUPPORTS1(MaemoLocationProvider, nsIGeolocationProvider)

MaemoLocationProvider::MaemoLocationProvider()
: mGPSDevice(nsnull), mLocationCallbackHandle(0), mHasSeenLocation(PR_FALSE)
{
}

MaemoLocationProvider::~MaemoLocationProvider()
{
}

void location_changed (LocationGPSDevice *device, gpointer userdata)
{
  MaemoLocationProvider* provider = (MaemoLocationProvider*) userdata;
  nsRefCnt<nsGeolocation*> somewhere = new nsGeolocation(device->fix->latitude,
                                                         device->fix->longitude,
                                                         device->fix->altitude,
                                                         device->fix->eph,
                                                         device->fix->epv,
                                                         device->fix->time);
  provider->Update(somewhere);
}

NS_IMETHODIMP MaemoLocationProvider::Startup()
{
  if (!mGPSDevice)
  {
    
    memset(&mGPSBT, 0, sizeof(gpsbt_t));
    int result = gpsbt_start(NULL, 0, 0, 0, NULL, 0, 0, &mGPSBT);
    if (result <0)
      return NS_ERROR_NOT_AVAILABLE;
    
    mGPSDevice = (LocationGPSDevice*) g_object_new (LOCATION_TYPE_GPS_DEVICE, NULL);
    mLocationCallbackHandle = g_signal_connect (mGPSDevice, "changed", G_CALLBACK (location_changed), this->mCallback);
  }
  return NS_OK;
}

NS_IMETHODIMP MaemoLocationProvider::IsReady(PRBool *_retval NS_OUTPARAM)
{
  *_retval = mHasSeenLocation;
  return NS_OK;
}

NS_IMETHODIMP MaemoLocationProvider::Watch(nsIGeolocationUpdate *callback)
{
  mCallback = callback; 
  return NS_OK;
}


NS_IMETHODIMP MaemoLocationProvider::GetCurrentLocation(nsIDOMGeolocation * *aCurrentLocation)
{
  NS_IF_ADDREF(*aCurrentLocation = mLastLocation);
  return NS_OK;
}

NS_IMETHODIMP MaemoLocationProvider::Shutdown()
{
  if (mGPSDevice && mLocationCallbackHandle) {
    g_signal_handler_disconnect(mGPSDevice, mLocationCallbackHandle);
    g_object_unref(mGPSDevice);
    gpsbt_stop(&mGPSBT);
    mLocationCallbackHandle = 0;
    mGPSDevice = nsnull;
    mHasSeenLocation = PR_FALSE;
  }
  return NS_OK;
}

void MaemoLocationProvider::Update(nsIDOMGeolocation* aLocation)
{
  mHasSeenLocation = PR_TRUE;
  mLastLocation = aLocation;
  if (mCallback)
    mCallback->Update(aLocation);
}
