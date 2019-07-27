




#ifndef nsGeolocationSettings_h
#define nsGeolocationSettings_h

#include "mozilla/Attributes.h"
#include "mozilla/StaticPtr.h"
#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "nsClassHashtable.h"
#include "nsString.h"
#include "nsIObserver.h"
#include "nsJSUtils.h"
#include "nsTArray.h"

#if (defined(MOZ_GPS_DEBUG) && defined(ANDROID))
#include <android/log.h>
#define GPSLOG(fmt, ...) __android_log_print(ANDROID_LOG_WARN, "GPS", "%12s:%-5d " fmt,  __FILE__, __LINE__, ##__VA_ARGS__)
#else
#define GPSLOG(...) {;}
#endif 


#define GEO_ENABLED             "geolocation.enabled"
#define GEO_ALA_ENABLED         "ala.settings.enabled"
#define GEO_ALA_TYPE            "geolocation.type"
#define GEO_ALA_FIXED_COORDS    "geolocation.fixed_coords"
#define GEO_ALA_APP_SETTINGS    "geolocation.app_settings"
#define GEO_ALA_ALWAYS_PRECISE  "geolocation.always_precise"
#ifdef MOZ_APPROX_LOCATION
#define GEO_ALA_APPROX_DISTANCE "geolocation.approx_distance"
#endif

enum GeolocationFuzzMethod {
  GEO_ALA_TYPE_PRECISE, 
  GEO_ALA_TYPE_FIXED,   
  GEO_ALA_TYPE_NONE,    
#ifdef MOZ_APPROX_LOCATION
  GEO_ALA_TYPE_APPROX   
#endif
};

#define GEO_ALA_TYPE_DEFAULT    (GEO_ALA_TYPE_PRECISE)
#define GEO_ALA_TYPE_FIRST      (GEO_ALA_TYPE_PRECISE)
#ifdef MOZ_APPROX_LOCATION
#define GEO_ALA_TYPE_LAST       (GEO_ALA_TYPE_APPROX)
#else
#define GEO_ALA_TYPE_LAST       (GEO_ALA_TYPE_NONE)
#endif





class GeolocationSetting MOZ_FINAL {
public:
  GeolocationSetting(const nsString& aOrigin) :
    mFuzzMethod(GEO_ALA_TYPE_DEFAULT),
#ifdef MOZ_APPROX_LOCATION
    mDistance(0),
#endif
    mLatitude(0.0),
    mLongitude(0.0),
    mOrigin(aOrigin) {}

  GeolocationSetting(const GeolocationSetting& rhs) :
    mFuzzMethod(rhs.mFuzzMethod),
#ifdef MOZ_APPROX_LOCATION
    mDistance(rhs.mDistance),
#endif
    mLatitude(rhs.mLatitude),
    mLongitude(rhs.mLongitude),
    mOrigin(rhs.mOrigin) {}

  ~GeolocationSetting() {}

  GeolocationSetting& operator=(const GeolocationSetting& rhs) {
    mFuzzMethod = rhs.mFuzzMethod;
#ifdef MOZ_APPROX_LOCATION
    mDistance = rhs.mDistance;
#endif
    mLatitude = rhs.mLatitude;
    mLongitude = rhs.mLongitude;
    mOrigin = rhs.mOrigin;
    return *this;
  }

  void HandleTypeChange(const JS::Value& aVal);
  void HandleApproxDistanceChange(const JS::Value& aVal);
  void HandleFixedCoordsChange(const JS::Value& aVal);

  inline GeolocationFuzzMethod GetType() const { return mFuzzMethod; }
#ifdef MOZ_APPROX_LOCATION
  inline int32_t GetApproxDistance() const { return mDistance; }
#endif
  inline double GetFixedLatitude() const { return mLatitude; }
  inline double GetFixedLongitude() const { return mLongitude; }
  inline const nsString& GetOrigin() const { return mOrigin; }

private:
  GeolocationSetting() {} 
  GeolocationFuzzMethod mFuzzMethod;
#ifdef MOZ_APPROX_LOCATION
  int32_t         mDistance;
#endif
  double          mLatitude,
                  mLongitude;
  nsString        mOrigin;
};




class nsGeolocationSettings MOZ_FINAL : public nsIObserver
{
public:
  static already_AddRefed<nsGeolocationSettings> GetGeolocationSettings();
  static mozilla::StaticRefPtr<nsGeolocationSettings> sSettings;

  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIOBSERVER

  nsGeolocationSettings() : mAlaEnabled(false), mGlobalSetting(NullString()) {}
  nsresult Init();

  void HandleGeolocationSettingsChange(const nsAString& aKey, const JS::Value& aVal);
  void HandleGeolocationSettingsError(const nsAString& aName);

  void PutWatchOrigin(int32_t aWatchID, const nsCString& aOrigin);
  void RemoveWatchOrigin(int32_t aWatchID);
  void GetWatchOrigin(int32_t aWatchID, nsCString& aOrigin);
  inline bool IsAlaEnabled() const { return mAlaEnabled; }

  
  
  
  
  
  
  
  GeolocationSetting LookupGeolocationSetting(int32_t aWatchID);

private:
  ~nsGeolocationSettings() {}
  nsGeolocationSettings(const nsGeolocationSettings&) :
    mGlobalSetting(NullString()) {} 

  void HandleMozsettingsChanged(nsISupports* aSubject);
  void HandleGeolocationAlaEnabledChange(const JS::Value& aVal);
  void HandleGeolocationPerOriginSettingsChange(const JS::Value& aVal);
  void HandleGeolocationAlwaysPreciseChange(const JS::Value& aVal);

private:
  bool mAlaEnabled;
  GeolocationSetting mGlobalSetting;
  nsClassHashtable<nsStringHashKey, GeolocationSetting> mPerOriginSettings;
  nsTArray<nsString> mAlwaysPreciseApps;
  nsClassHashtable<nsUint32HashKey, nsCString> mCurrentWatches;
};

#endif 

