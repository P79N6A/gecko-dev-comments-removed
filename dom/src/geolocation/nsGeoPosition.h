




#ifndef nsGeoPosition_h
#define nsGeoPosition_h

#include "nsAutoPtr.h"
#include "nsIDOMGeoPositionCoords.h"
#include "nsIDOMGeoPosition.h"
#include "nsString.h"
#include "mozilla/Attributes.h"
#include "nsCycleCollectionParticipant.h"
#include "nsWrapperCache.h"
#include "mozilla/dom/Nullable.h"
#include "js/TypeDecls.h"








class nsGeoPositionCoords MOZ_FINAL : public nsIDOMGeoPositionCoords
{
public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIDOMGEOPOSITIONCOORDS
  
  nsGeoPositionCoords(double aLat, double aLong,
                      double aAlt, double aHError,
                      double aVError, double aHeading,
                      double aSpeed);
private:
  ~nsGeoPositionCoords();
  const double mLat, mLong, mAlt, mHError, mVError, mHeading, mSpeed;
};






class nsGeoPosition MOZ_FINAL : public nsIDOMGeoPosition
{
public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIDOMGEOPOSITION
  
  nsGeoPosition(double aLat, double aLong,
                double aAlt, double aHError,
                double aVError, double aHeading,
                double aSpeed, long long aTimestamp);
  

  nsGeoPosition(nsIDOMGeoPositionCoords *aCoords,
                long long aTimestamp);

  nsGeoPosition(nsIDOMGeoPositionCoords *aCoords,
                DOMTimeStamp aTimestamp);

private:
  ~nsGeoPosition();
  long long mTimestamp;
  nsRefPtr<nsIDOMGeoPositionCoords> mCoords;
};





namespace mozilla {
namespace dom {

class Coordinates;

class Position MOZ_FINAL : public nsISupports,
                           public nsWrapperCache
{
  ~Position();

public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(Position)

public:
  Position(nsISupports* aParent, nsIDOMGeoPosition* aGeoPosition);

  nsISupports* GetParentObject() const;

  virtual JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE;

  Coordinates* Coords();

  uint64_t Timestamp() const;

  nsIDOMGeoPosition* GetWrappedGeoPosition() { return mGeoPosition; }

private:
  nsRefPtr<Coordinates> mCoordinates;
  nsCOMPtr<nsISupports> mParent;
  nsCOMPtr<nsIDOMGeoPosition> mGeoPosition;
};

class Coordinates MOZ_FINAL : public nsISupports,
                              public nsWrapperCache
{
  ~Coordinates();

public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(Coordinates)

public:
  Coordinates(Position* aPosition, nsIDOMGeoPositionCoords* aCoords);

  Position* GetParentObject() const;

  virtual JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE;

  double Latitude() const;

  double Longitude() const;

  Nullable<double> GetAltitude() const;

  double Accuracy() const;

  Nullable<double> GetAltitudeAccuracy() const;

  Nullable<double> GetHeading() const;

  Nullable<double> GetSpeed() const;
private:
  nsRefPtr<Position> mPosition;
  nsCOMPtr<nsIDOMGeoPositionCoords> mCoords;
};

} 
} 

#endif 

