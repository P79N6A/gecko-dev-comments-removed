






#ifndef ASTRO_H
#define ASTRO_H

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "gregoimp.h"  
#include "unicode/unistr.h"

U_NAMESPACE_BEGIN



































class U_I18N_API CalendarAstronomer : public UMemory {
public:
  

public:
  
















  class U_I18N_API Ecliptic : public UMemory {
  public:
    






    Ecliptic(double lat = 0, double lon = 0) {
      latitude = lat;
      longitude = lon;
    }

    





    void set(double lat, double lon) {
      latitude = lat;
      longitude = lon;
    }

    



    UnicodeString toString() const;

    





    double latitude;

    










    double longitude;
  };

  















  class U_I18N_API Equatorial : public UMemory {
  public:
    






    Equatorial(double asc = 0, double dec = 0)
      : ascension(asc), declination(dec) { }

    





    void set(double asc, double dec) {
      ascension = asc;
      declination = dec;
    }

    




    UnicodeString toString() const;

    




    
    
    

    






    double ascension;

    





    double declination;
  };

  
















  class U_I18N_API Horizon : public UMemory {
  public:
    






    Horizon(double alt=0, double azim=0)
      : altitude(alt), azimuth(azim) { }

    





    void set(double alt, double azim) {
      altitude = alt;
      azimuth = azim;
    }

    




    UnicodeString toString() const;

    



    double altitude;

    



    double azimuth;
  };

public:
  
  
  

  
  static const double PI;

  










  static const double SYNODIC_MONTH;

  
  
  

  




  CalendarAstronomer();

  




  CalendarAstronomer(UDate d);

  













  CalendarAstronomer(double longitude, double latitude);

  



  ~CalendarAstronomer();

  
  
  

  










  void setTime(UDate aTime);


  









  void setDate(UDate aDate) { setTime(aDate); }

  













  void setJulianDay(double jdn);

  








  UDate getTime();

  








  double getJulianDay();

  






  double getJulianCentury();

  



  double getGreenwichSidereal();

private:
  double getSiderealOffset();
public:
  



  double getLocalSidereal();

  








  
  double lstToUT(double lst);

  







  Equatorial& eclipticToEquatorial(Equatorial& result, const Ecliptic& ecliptic);

  








  Equatorial& eclipticToEquatorial(Equatorial& result, double eclipLong, double eclipLat);

  







  Equatorial& eclipticToEquatorial(Equatorial& result, double eclipLong) ;

  


  Horizon& eclipticToHorizon(Horizon& result, double eclipLong) ;

  
  
  

  










  double getSunLongitude();

  


   void getSunLongitude(double julianDay, double &longitude, double &meanAnomaly);

  





  Equatorial& getSunPosition(Equatorial& result);

public:
  







  





  static double SUMMER_SOLSTICE();

  







  





  static double WINTER_SOLSTICE();

  




  UDate getSunTime(double desired, UBool next);

  











  UDate getSunRiseSet(UBool rise);

  
  
  

  





  const Equatorial& getMoonPosition();

  








  double getMoonAge();

  













  double getMoonPhase();

  class U_I18N_API MoonAge : public UMemory {
  public:
    MoonAge(double l)
      :  value(l) { }
    void set(double l) { value = l; }
    double value;
  };

  




  static const MoonAge NEW_MOON();

  






  




  static const MoonAge FULL_MOON();

  






  








  UDate getMoonTime(double desired, UBool next);
  UDate getMoonTime(const MoonAge& desired, UBool next);

  




  UDate getMoonRiseSet(UBool rise);

  
  
  

  
  class AngleFunc : public UMemory {
  public:
    virtual double eval(CalendarAstronomer&) = 0;
    virtual ~AngleFunc();
  };
  friend class AngleFunc;

  UDate timeOfAngle(AngleFunc& func, double desired,
                    double periodDays, double epsilon, UBool next);

  class CoordFunc : public UMemory {
  public:
    virtual void eval(Equatorial& result, CalendarAstronomer&) = 0;
    virtual ~CoordFunc();
  };
  friend class CoordFunc;

  double riseOrSet(CoordFunc& func, UBool rise,
                   double diameter, double refraction,
                   double epsilon);

  
  
  
private:

  







  double eclipticObliquity();

  
  
  
private:
  



  UDate fTime;

  


  double fLongitude;
  double fLatitude;
  double fGmtOffset;

  
  
  
  
  

  double    julianDay;
  double    julianCentury;
  double    sunLongitude;
  double    meanAnomalySun;
  double    moonLongitude;
  double    moonEclipLong;
  double    meanAnomalyMoon;
  double    eclipObliquity;
  double    siderealT0;
  double    siderealTime;

  void clearCache();

  Equatorial  moonPosition;
  UBool       moonPositionSet;

  



};

U_NAMESPACE_END

struct UHashtable;

U_NAMESPACE_BEGIN





class CalendarCache : public UMemory {
public:
  static int32_t get(CalendarCache** cache, int32_t key, UErrorCode &status);
  static void put(CalendarCache** cache, int32_t key, int32_t value, UErrorCode &status);
  virtual ~CalendarCache();
private:
  CalendarCache(int32_t size, UErrorCode& status);
  static void createCache(CalendarCache** cache, UErrorCode& status);
  


  CalendarCache();
  UHashtable *fTable;
};

U_NAMESPACE_END

#endif
#endif
