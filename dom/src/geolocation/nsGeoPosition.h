





































#ifndef nsGeoPosition_h
#define nsGeoPosition_h

#include "nsAutoPtr.h"
#include "nsIClassInfo.h"
#include "nsDOMClassInfoID.h"
#include "nsIDOMGeoPositionCoords.h"
#include "nsIDOMGeoPosition.h"








class nsGeoPositionCoords : public nsIDOMGeoPositionCoords
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMGEOPOSITIONCOORDS
  
  nsGeoPositionCoords(double aLat, double aLong,
                      double aAlt, double aHError,
                      double aVError, double aHeading,
                      double aSpeed);
private:
  ~nsGeoPositionCoords();
  double mLat, mLong, mAlt, mHError, mVError, mHeading, mSpeed;
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
  
private:
  ~nsGeoPosition();
  long long mTimestamp;
  nsRefPtr<nsGeoPositionCoords> mCoords;
};

#endif 

