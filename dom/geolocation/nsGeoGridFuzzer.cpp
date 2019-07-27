




#include <math.h>
#include "nsGeoGridFuzzer.h"
#include "nsGeoPosition.h"


#ifdef MOZ_APPROX_LOCATION







#define WGS84_a         (6378137.0)           // equitorial axis
#define WGS84_b         (6356752.314245179)   // polar axis (a * (1-f))
#define WGS84_f         (1.0/298.257223563)   // inverse flattening
#define WGS84_EPSILON   (5.72957795e-9)       // 1e-10 radians in degrees
#define sq(f)           ((f) * (f))
#define sign(f)         (((f) < 0) ? -1 : 1)





#define LON_RADIUS(phi) (sqrt((sq(sq(WGS84_a) * cos(phi)) + sq(sq(WGS84_b) * sin(phi))) / \
                              (sq(WGS84_a * cos(phi)) + sq(WGS84_b * sin(phi)))))




#define LAT_RADIUS          (6367489.543863)





static double GridAlgorithmLat(int32_t aDistMeters, double aLatDeg)
{
  
  double phi = (aLatDeg * M_PI) / 180;

  
  phi = atan(sin(phi) / fabs(cos(phi)));

  
  double gridSizeRad = aDistMeters / LAT_RADIUS;

  

  double gridCenterPhi = gridSizeRad * floor(phi / gridSizeRad) + gridSizeRad / 2;

  
  return atan(sin(gridCenterPhi) / fabs(cos(gridCenterPhi))) * (180.0 / M_PI);
}





static double GridAlgorithmLon(int32_t aDistMeters, double aLatDeg, double aLonDeg)
{
  
  double phi = (aLatDeg * M_PI) / 180;
  double theta = (aLonDeg * M_PI) / 180;

  
  phi = atan(sin(phi) / fabs(cos(phi)));
  theta = atan2(sin(theta), cos(theta));

  
  double gridSizeRad = aDistMeters / LON_RADIUS(phi);

  

  double gridCenterTheta = gridSizeRad * floor(theta / gridSizeRad) + gridSizeRad / 2;

  
  return atan2(sin(gridCenterTheta), cos(gridCenterTheta)) * (180.0 / M_PI);
}





static void CalculateGridCoords(int32_t aDistKm, double&  aLatDeg, double& aLonDeg)
{
  
  if (aDistKm == 0) {
    return;
  }
  aLonDeg = GridAlgorithmLon(aDistKm * 1000, aLatDeg, aLonDeg);
  aLatDeg = GridAlgorithmLat(aDistKm * 1000, aLatDeg);
}

already_AddRefed<nsIDOMGeoPosition>
nsGeoGridFuzzer::FuzzLocation(const GeolocationSetting & aSetting,
                              nsIDOMGeoPosition * aPosition)
{
  if (!aPosition) {
    return nullptr;
  }

  nsCOMPtr<nsIDOMGeoPositionCoords> coords;
  nsresult rv = aPosition->GetCoords(getter_AddRefs(coords));
  NS_ENSURE_SUCCESS(rv, nullptr);
  if (!coords) {
   return nullptr;
  }

  double lat = 0.0, lon = 0.0;
  coords->GetLatitude(&lat);
  coords->GetLongitude(&lon);

  
  CalculateGridCoords(aSetting.GetApproxDistance(), lat, lon);
  GPSLOG("approximate location with delta %d is %f, %f",
         aSetting.GetApproxDistance(), lat, lon);

  
  DOMTimeStamp ts;
  rv = aPosition->GetTimestamp(&ts);
  NS_ENSURE_SUCCESS(rv, nullptr);

  
  nsRefPtr<nsGeoPosition> pos = new nsGeoPosition(lat, lon, 0.0, 0.0,
                                                  0.0, 0.0, 0.0, ts);
  return pos.forget();
}

#endif
