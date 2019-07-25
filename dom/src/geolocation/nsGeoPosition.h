






































#ifndef nsGeoPosition_h
#define nsGeoPosition_h

#include "nsAutoPtr.h"
#include "nsIClassInfo.h"
#include "nsDOMClassInfoID.h"
#include "nsIDOMGeoPositionAddress.h"
#include "nsIDOMGeoPositionCoords.h"
#include "nsIDOMGeoPosition.h"
#include "nsString.h"





class nsGeoPositionAddress : public nsIDOMGeoPositionAddress
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMGEOPOSITIONADDRESS

  nsGeoPositionAddress( const nsAString &aStreetNumber,
                        const nsAString &aStreet,
                        const nsAString &aPremises,
                        const nsAString &aCity,
                        const nsAString &aCounty,
                        const nsAString &aRegion,
                        const nsAString &aCountry,
                        const nsAString &aCountryCode,
                        const nsAString &aPostalCode);

  private:
    ~nsGeoPositionAddress();
    const nsString mStreetNumber;
    const nsString mStreet;
    const nsString mPremises;
    const nsString mCity;
    const nsString mCounty;
    const nsString mRegion;
    const nsString mCountry;
    const nsString mCountryCode;
    const nsString mPostalCode;
};








class nsGeoPositionCoords : public nsIDOMGeoPositionCoords
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMGEOPOSITIONCOORDS
  
  nsGeoPositionCoords(const double aLat,
                      const double aLong,
                      const double aAlt,
                      const double aHError,
                      const double aVError,
                      const double aHeading,
                      const double aSpeed);
private:
  ~nsGeoPositionCoords();
  const double mLat, mLong, mAlt, mHError, mVError, mHeading, mSpeed;
};






class nsGeoPosition : public nsIDOMGeoPosition
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMGEOPOSITION
  
  nsGeoPosition(double aLat, double aLong,
                double aAlt, double aHError,
                double aVError, double aHeading,
                double aSpeed, long long aTimestamp);
  

  nsGeoPosition(nsIDOMGeoPositionCoords *aCoords,
                long long aTimestamp);

  nsGeoPosition(nsIDOMGeoPositionCoords *aCoords,
                nsIDOMGeoPositionAddress *aAddress,
                DOMTimeStamp aTimestamp);

private:
  ~nsGeoPosition();
  long long mTimestamp;
  nsRefPtr<nsIDOMGeoPositionCoords> mCoords;
  nsRefPtr<nsIDOMGeoPositionAddress> mAddress;
};

#endif 

