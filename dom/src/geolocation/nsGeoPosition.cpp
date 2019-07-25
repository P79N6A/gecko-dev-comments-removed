






































#include "nsGeoPosition.h"
#include "nsDOMClassInfoID.h"





nsGeoPositionAddress::nsGeoPositionAddress(const nsAString &aStreetNumber,
                                           const nsAString &aStreet,
                                           const nsAString &aPremises,
                                           const nsAString &aCity,
                                           const nsAString &aCounty,
                                           const nsAString &aRegion,
                                           const nsAString &aCountry,
                                           const nsAString &aCountryCode,
                                           const nsAString &aPostalCode)
    : mStreetNumber(aStreetNumber)
    , mStreet(aStreet)
    , mPremises(aPremises)
    , mCity(aCity)
    , mCounty(aCounty)
    , mRegion(aRegion)
    , mCountry(aCountry)
    , mCountryCode(aCountryCode)
    , mPostalCode(aPostalCode)
{
}

nsGeoPositionAddress::~nsGeoPositionAddress()
{
}

DOMCI_DATA(GeoPositionAddress, nsGeoPositionAddress)

NS_INTERFACE_MAP_BEGIN(nsGeoPositionAddress)
NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDOMGeoPositionAddress)
NS_INTERFACE_MAP_ENTRY(nsIDOMGeoPositionAddress)
NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(GeoPositionAddress)
NS_INTERFACE_MAP_END

NS_IMPL_THREADSAFE_ADDREF(nsGeoPositionAddress)
NS_IMPL_THREADSAFE_RELEASE(nsGeoPositionAddress)

NS_IMETHODIMP
nsGeoPositionAddress::GetStreetNumber(nsAString & aStreetNumber)
{
  aStreetNumber = mStreetNumber;
  return NS_OK;
}

NS_IMETHODIMP
nsGeoPositionAddress::GetStreet(nsAString & aStreet)
{
  aStreet = mStreet;
  return NS_OK;
}

NS_IMETHODIMP
nsGeoPositionAddress::GetPremises(nsAString & aPremises)
{
  aPremises = mPremises;
  return NS_OK;
}

NS_IMETHODIMP
nsGeoPositionAddress::GetCity(nsAString & aCity)
{
  aCity = mCity;
  return NS_OK;
}

NS_IMETHODIMP
nsGeoPositionAddress::GetCounty(nsAString & aCounty)
{
  aCounty = mCounty;
  return NS_OK;
}

NS_IMETHODIMP
nsGeoPositionAddress::GetRegion(nsAString & aRegion)
{
  aRegion = mRegion;
  return NS_OK;
}

NS_IMETHODIMP
nsGeoPositionAddress::GetCountry(nsAString & aCountry)
{
  aCountry = mCountry;
  return NS_OK;
}

NS_IMETHODIMP
nsGeoPositionAddress::GetCountryCode(nsAString & aCountryCode)
{
  aCountryCode = mCountryCode;
  return NS_OK;
}

NS_IMETHODIMP
nsGeoPositionAddress::GetPostalCode(nsAString & aPostalCode)
{
  aPostalCode = mPostalCode;
  return NS_OK;
}




nsGeoPositionCoords::nsGeoPositionCoords(double aLat, double aLong,
                                         double aAlt, double aHError,
                                         double aVError, double aHeading,
                                         double aSpeed)
  : mLat(aLat)
  , mLong(aLong)
  , mAlt(aAlt)
  , mHError(aHError)
  , mVError(aVError)
  , mHeading(aHeading)
  , mSpeed(aSpeed)
{
}

nsGeoPositionCoords::~nsGeoPositionCoords()
{
}

DOMCI_DATA(GeoPositionCoords, nsGeoPositionCoords)

NS_INTERFACE_MAP_BEGIN(nsGeoPositionCoords)
NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDOMGeoPositionCoords)
NS_INTERFACE_MAP_ENTRY(nsIDOMGeoPositionCoords)
NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(GeoPositionCoords)
NS_INTERFACE_MAP_END

NS_IMPL_THREADSAFE_ADDREF(nsGeoPositionCoords)
NS_IMPL_THREADSAFE_RELEASE(nsGeoPositionCoords)

NS_IMETHODIMP
nsGeoPositionCoords::GetLatitude(double *aLatitude)
{
  *aLatitude = mLat;
  return NS_OK;
}

NS_IMETHODIMP
nsGeoPositionCoords::GetLongitude(double *aLongitude)
{
  *aLongitude = mLong;
  return NS_OK;
}

NS_IMETHODIMP
nsGeoPositionCoords::GetAltitude(double *aAltitude)
{
  *aAltitude = mAlt;
  return NS_OK;
}

NS_IMETHODIMP
nsGeoPositionCoords::GetAccuracy(double *aAccuracy)
{
  *aAccuracy = mHError;
  return NS_OK;
}

NS_IMETHODIMP
nsGeoPositionCoords::GetAltitudeAccuracy(double *aAltitudeAccuracy)
{
  *aAltitudeAccuracy = mVError;
  return NS_OK;
}

NS_IMETHODIMP
nsGeoPositionCoords::GetHeading(double *aHeading)
{
  *aHeading = mHeading;
  return NS_OK;
}

NS_IMETHODIMP
nsGeoPositionCoords::GetSpeed(double *aSpeed)
{
  *aSpeed = mSpeed;
  return NS_OK;
}





nsGeoPosition::nsGeoPosition(double aLat, double aLong,
                             double aAlt, double aHError,
                             double aVError, double aHeading,
                             double aSpeed, long long aTimestamp) :
    mTimestamp(aTimestamp)
{
    mCoords = new nsGeoPositionCoords(aLat, aLong,
                                      aAlt, aHError,
                                      aVError, aHeading,
                                      aSpeed);
}

nsGeoPosition::nsGeoPosition(nsIDOMGeoPositionCoords *aCoords,
                             long long aTimestamp) :
    mTimestamp(aTimestamp),
    mCoords(aCoords)
{
}

nsGeoPosition::nsGeoPosition(nsIDOMGeoPositionCoords *aCoords,
                             nsIDOMGeoPositionAddress *aAddress,
                             DOMTimeStamp aTimestamp) :
  mTimestamp(aTimestamp),
  mCoords(aCoords),
  mAddress(aAddress)
{
}

nsGeoPosition::~nsGeoPosition()
{
}

DOMCI_DATA(GeoPosition, nsGeoPosition)

NS_INTERFACE_MAP_BEGIN(nsGeoPosition)
NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDOMGeoPosition)
NS_INTERFACE_MAP_ENTRY(nsIDOMGeoPosition)
NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(GeoPosition)
NS_INTERFACE_MAP_END

NS_IMPL_THREADSAFE_ADDREF(nsGeoPosition)
NS_IMPL_THREADSAFE_RELEASE(nsGeoPosition)

NS_IMETHODIMP
nsGeoPosition::GetTimestamp(DOMTimeStamp* aTimestamp)
{
  *aTimestamp = mTimestamp;
  return NS_OK;
}

NS_IMETHODIMP
nsGeoPosition::GetCoords(nsIDOMGeoPositionCoords * *aCoords)
{
  NS_IF_ADDREF(*aCoords = mCoords);
  return NS_OK;
}

NS_IMETHODIMP
nsGeoPosition::GetAddress(nsIDOMGeoPositionAddress** aAddress)
{
  NS_IF_ADDREF(*aAddress = mAddress);
  return NS_OK;
}

