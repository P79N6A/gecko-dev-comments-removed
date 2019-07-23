





































#include "MaemoLocationProvider.h"
#include "nsGeolocation.h"








 
class nsGeoPosition : public nsIDOMGeoPosition
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMGEOPOSITION

    nsGeoPosition(double aLat, double aLong, double aAlt, double aHError, double aVError, double aHeading, double aSpeed, long long aTimestamp)
    : mLat(aLat), mLong(aLong), mAlt(aAlt), mHError(aHError), mVError(aVError), mHeading(aHeading), mSpeed(aSpeed), mTimestamp(aTimestamp){};

private:
  ~nsGeoPosition(){}
  double mLat, mLong, mAlt, mHError, mVError, mHeading, mSpeed;
  long long mTimestamp;
};

NS_INTERFACE_MAP_BEGIN(nsGeoPosition)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDOMGeoPosition)
  NS_INTERFACE_MAP_ENTRY(nsIDOMGeoPosition)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(GeoPosition)
NS_INTERFACE_MAP_END

NS_IMPL_THREADSAFE_ADDREF(nsGeoPosition)
NS_IMPL_THREADSAFE_RELEASE(nsGeoPosition)

NS_IMETHODIMP
nsGeoPosition::GetLatitude(double *aLatitude)
{
  *aLatitude = mLat;
  return NS_OK;
}

NS_IMETHODIMP
nsGeoPosition::GetLongitude(double *aLongitude)
{
  *aLongitude = mLong;
  return NS_OK;
}

NS_IMETHODIMP
nsGeoPosition::GetAltitude(double *aAltitude)
{
  *aAltitude = mAlt;
  return NS_OK;
}

NS_IMETHODIMP
nsGeoPosition::GetAccuracy(double *aAccuracy)
{
  *aAccuracy = mHError;
  return NS_OK;
}

NS_IMETHODIMP
nsGeoPosition::GetAltitudeAccuracy(double *aAltitudeAccuracy)
{
  *aAltitudeAccuracy = mVError;
  return NS_OK;
}

NS_IMETHODIMP
nsGeoPosition::GetHeading(double *aHeading)
{
  *aHeading = mHeading;
  return NS_OK;
}

NS_IMETHODIMP
nsGeoPosition::GetSpeed(double *aSpeed)
{
  *aSpeed = mSpeed;
  return NS_OK;
}

NS_IMETHODIMP
nsGeoPosition::GetTimestamp(DOMTimeStamp* aTimestamp)
{
  *aTimestamp = mTimestamp;
  return NS_OK;
}


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
  nsRefPtr<nsGeoPosition> somewhere = new nsGeoPosition(device->fix->latitude,
                                                        device->fix->longitude,
                                                        device->fix->altitude,
                                                        device->fix->eph,
                                                        device->fix->epv,
                                                        0,0,
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


NS_IMETHODIMP MaemoLocationProvider::GetCurrentPosition(nsIDOMGeoPosition * *aCurrentPosition)
{
  NS_IF_ADDREF(*aCurrentPosition = mLastPosition);
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

void MaemoLocationProvider::Update(nsIDOMGeoPosition* aPosition)
{
  mHasSeenLocation = PR_TRUE;
  mLastPosition = aPosition;
  if (mCallback)
    mCallback->Update(aPosition);
}
