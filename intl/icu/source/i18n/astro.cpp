






#include "astro.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/calendar.h"
#include <math.h>
#include <float.h>
#include "unicode/putil.h"
#include "uhash.h"
#include "umutex.h"
#include "ucln_in.h"
#include "putilimp.h"
#include <stdio.h>  

#if defined (PI) 
#undef PI
#endif

#ifdef U_DEBUG_ASTRO
# include "uresimp.h" 

static void debug_astro_loc(const char *f, int32_t l)
{
  fprintf(stderr, "%s:%d: ", f, l);
}

static void debug_astro_msg(const char *pat, ...)
{
  va_list ap;
  va_start(ap, pat);
  vfprintf(stderr, pat, ap);
  fflush(stderr);
}
#include "unicode/datefmt.h"
#include "unicode/ustring.h"
static const char * debug_astro_date(UDate d) {
  static char gStrBuf[1024];
  static DateFormat *df = NULL;
  if(df == NULL) {
    df = DateFormat::createDateTimeInstance(DateFormat::MEDIUM, DateFormat::MEDIUM, Locale::getUS());
    df->adoptTimeZone(TimeZone::getGMT()->clone());
  }
  UnicodeString str;
  df->format(d,str);
  u_austrncpy(gStrBuf,str.getTerminatedBuffer(),sizeof(gStrBuf)-1);
  return gStrBuf;
}


#define U_DEBUG_ASTRO_MSG(x) {debug_astro_loc(__FILE__,__LINE__);debug_astro_msg x;}
#else
#define U_DEBUG_ASTRO_MSG(x)
#endif

static inline UBool isINVALID(double d) {
  return(uprv_isNaN(d));
}

static UMutex ccLock = U_MUTEX_INITIALIZER;

U_CDECL_BEGIN
static UBool calendar_astro_cleanup(void) {
  return TRUE;
}
U_CDECL_END

U_NAMESPACE_BEGIN







#define SIDEREAL_DAY (23.93446960027)







#define SOLAR_DAY  (24.065709816)












const double CalendarAstronomer::SYNODIC_MONTH  = 29.530588853;













#define SIDEREAL_MONTH  27.32166











#define TROPICAL_YEAR  365.242191














#define SIDEREAL_YEAR  365.25636










#define SECOND_MS  U_MILLIS_PER_SECOND






#define MINUTE_MS  U_MILLIS_PER_MINUTE






#define HOUR_MS   U_MILLIS_PER_HOUR






#define DAY_MS U_MILLIS_PER_DAY











#define JULIAN_EPOCH_MS  -210866760000000.0





#define EPOCH_2000_MS  946598400000.0






const double CalendarAstronomer::PI = 3.14159265358979323846;

#define CalendarAstronomer_PI2  (CalendarAstronomer::PI*2.0)
#define RAD_HOUR  ( 12 / CalendarAstronomer::PI )     // radians -> hours
#define DEG_RAD ( CalendarAstronomer::PI / 180 )      // degrees -> radians
#define RAD_DEG  ( 180 / CalendarAstronomer::PI )     // radians -> degrees





inline static double normalize(double value, double range)  {
    return value - range * ClockMath::floorDivide(value, range);
}






inline static double norm2PI(double angle)  {
    return normalize(angle, CalendarAstronomer::PI * 2.0);
}




inline static  double normPI(double angle)  {
    return normalize(angle + CalendarAstronomer::PI, CalendarAstronomer::PI * 2.0) - CalendarAstronomer::PI;
}











CalendarAstronomer::CalendarAstronomer():
  fTime(Calendar::getNow()), fLongitude(0.0), fLatitude(0.0), fGmtOffset(0.0), moonPosition(0,0), moonPositionSet(FALSE) {
  clearCache();
}







CalendarAstronomer::CalendarAstronomer(UDate d): fTime(d), fLongitude(0.0), fLatitude(0.0), fGmtOffset(0.0), moonPosition(0,0), moonPositionSet(FALSE) {
  clearCache();
}
















CalendarAstronomer::CalendarAstronomer(double longitude, double latitude) :
  fTime(Calendar::getNow()), moonPosition(0,0), moonPositionSet(FALSE) {
  fLongitude = normPI(longitude * (double)DEG_RAD);
  fLatitude  = normPI(latitude  * (double)DEG_RAD);
  fGmtOffset = (double)(fLongitude * 24. * (double)HOUR_MS / (double)CalendarAstronomer_PI2);
  clearCache();
}

CalendarAstronomer::~CalendarAstronomer()
{
}

















void CalendarAstronomer::setTime(UDate aTime) {
    fTime = aTime;
    U_DEBUG_ASTRO_MSG(("setTime(%.1lf, %sL)\n", aTime, debug_astro_date(aTime+fGmtOffset)));
    clearCache();
}
















void CalendarAstronomer::setJulianDay(double jdn) {
    fTime = (double)(jdn * DAY_MS) + JULIAN_EPOCH_MS;
    clearCache();
    julianDay = jdn;
}











UDate CalendarAstronomer::getTime() {
    return fTime;
}











double CalendarAstronomer::getJulianDay() {
    if (isINVALID(julianDay)) {
        julianDay = (fTime - (double)JULIAN_EPOCH_MS) / (double)DAY_MS;
    }
    return julianDay;
}









double CalendarAstronomer::getJulianCentury() {
    if (isINVALID(julianCentury)) {
        julianCentury = (getJulianDay() - 2415020.0) / 36525.0;
    }
    return julianCentury;
}






double CalendarAstronomer::getGreenwichSidereal() {
    if (isINVALID(siderealTime)) {
        
        

        double UT = normalize(fTime/(double)HOUR_MS, 24.);

        siderealTime = normalize(getSiderealOffset() + UT*1.002737909, 24.);
    }
    return siderealTime;
}

double CalendarAstronomer::getSiderealOffset() {
    if (isINVALID(siderealT0)) {
        double JD  = uprv_floor(getJulianDay() - 0.5) + 0.5;
        double S   = JD - 2451545.0;
        double T   = S / 36525.0;
        siderealT0 = normalize(6.697374558 + 2400.051336*T + 0.000025862*T*T, 24);
    }
    return siderealT0;
}






double CalendarAstronomer::getLocalSidereal() {
    return normalize(getGreenwichSidereal() + (fGmtOffset/(double)HOUR_MS), 24.);
}










double CalendarAstronomer::lstToUT(double lst) {
    
    double lt = normalize((lst - getSiderealOffset()) * 0.9972695663, 24);

    
    double base = (DAY_MS * ClockMath::floorDivide(fTime + fGmtOffset,(double)DAY_MS)) - fGmtOffset;

    
    

    return base + (long)(lt * HOUR_MS);
}














CalendarAstronomer::Equatorial& CalendarAstronomer::eclipticToEquatorial(CalendarAstronomer::Equatorial& result, const CalendarAstronomer::Ecliptic& ecliptic)
{
    return eclipticToEquatorial(result, ecliptic.longitude, ecliptic.latitude);
}











CalendarAstronomer::Equatorial& CalendarAstronomer::eclipticToEquatorial(CalendarAstronomer::Equatorial& result, double eclipLong, double eclipLat)
{
    
    

    double obliq = eclipticObliquity();
    double sinE = ::sin(obliq);
    double cosE = cos(obliq);

    double sinL = ::sin(eclipLong);
    double cosL = cos(eclipLong);

    double sinB = ::sin(eclipLat);
    double cosB = cos(eclipLat);
    double tanB = tan(eclipLat);

    result.set(atan2(sinL*cosE - tanB*sinE, cosL),
        asin(sinB*cosE + cosB*sinE*sinL) );
    return result;
}










CalendarAstronomer::Equatorial& CalendarAstronomer::eclipticToEquatorial(CalendarAstronomer::Equatorial& result, double eclipLong)
{
    return eclipticToEquatorial(result, eclipLong, 0);  
}





CalendarAstronomer::Horizon& CalendarAstronomer::eclipticToHorizon(CalendarAstronomer::Horizon& result, double eclipLong)
{
    Equatorial equatorial;
    eclipticToEquatorial(equatorial, eclipLong);

    double H = getLocalSidereal()*CalendarAstronomer::PI/12 - equatorial.ascension;     

    double sinH = ::sin(H);
    double cosH = cos(H);
    double sinD = ::sin(equatorial.declination);
    double cosD = cos(equatorial.declination);
    double sinL = ::sin(fLatitude);
    double cosL = cos(fLatitude);

    double altitude = asin(sinD*sinL + cosD*cosL*cosH);
    double azimuth  = atan2(-cosD*cosL*sinH, sinD - sinL * ::sin(altitude));

    result.set(azimuth, altitude);
    return result;
}










#define JD_EPOCH  2447891.5 // Julian day of epoch

#define SUN_ETA_G    (279.403303 * CalendarAstronomer::PI/180) // Ecliptic longitude at epoch
#define SUN_OMEGA_G  (282.768422 * CalendarAstronomer::PI/180) // Ecliptic longitude of perigee
#define SUN_E         0.016713          // Eccentricity of orbit





























































static double trueAnomaly(double meanAnomaly, double eccentricity)
{
    
    
    double delta;
    double E = meanAnomaly;
    do {
        delta = E - eccentricity * ::sin(E) - meanAnomaly;
        E = E - delta / (1 - eccentricity * ::cos(E));
    }
    while (uprv_fabs(delta) > 1e-5); 

    return 2.0 * ::atan( ::tan(E/2) * ::sqrt( (1+eccentricity)
                                             /(1-eccentricity) ) );
}













double CalendarAstronomer::getSunLongitude()
{
    
    

    if (isINVALID(sunLongitude)) {
        getSunLongitude(getJulianDay(), sunLongitude, meanAnomalySun);
    }
    return sunLongitude;
}




 void CalendarAstronomer::getSunLongitude(double jDay, double &longitude, double &meanAnomaly)
{
    
    

    double day = jDay - JD_EPOCH;       

    
    
    double epochAngle = norm2PI(CalendarAstronomer_PI2/TROPICAL_YEAR*day);

    
    
    meanAnomaly = norm2PI(epochAngle + SUN_ETA_G - SUN_OMEGA_G);

    
    
    
    
    longitude =  norm2PI(trueAnomaly(meanAnomaly, SUN_E) + SUN_OMEGA_G);
}







CalendarAstronomer::Equatorial& CalendarAstronomer::getSunPosition(CalendarAstronomer::Equatorial& result) {
    return eclipticToEquatorial(result, getSunLongitude(), 0);
}




















double CalendarAstronomer::SUMMER_SOLSTICE() {
    return  (CalendarAstronomer::PI/2);
}



















double CalendarAstronomer::WINTER_SOLSTICE() {
    return  ((CalendarAstronomer::PI*3)/2);
}

CalendarAstronomer::AngleFunc::~AngleFunc() {}







class SunTimeAngleFunc : public CalendarAstronomer::AngleFunc {
public:
    virtual ~SunTimeAngleFunc();
    virtual double eval(CalendarAstronomer& a) { return a.getSunLongitude(); }
};

SunTimeAngleFunc::~SunTimeAngleFunc() {}

UDate CalendarAstronomer::getSunTime(double desired, UBool next)
{
    SunTimeAngleFunc func;
    return timeOfAngle( func,
                        desired,
                        TROPICAL_YEAR,
                        MINUTE_MS,
                        next);
}

CalendarAstronomer::CoordFunc::~CoordFunc() {}

class RiseSetCoordFunc : public CalendarAstronomer::CoordFunc {
public:
    virtual ~RiseSetCoordFunc();
    virtual void eval(CalendarAstronomer::Equatorial& result, CalendarAstronomer&a) {  a.getSunPosition(result); }
};

RiseSetCoordFunc::~RiseSetCoordFunc() {}

UDate CalendarAstronomer::getSunRiseSet(UBool rise)
{
    UDate t0 = fTime;

    
    double noon = ClockMath::floorDivide(fTime + fGmtOffset, (double)DAY_MS)*DAY_MS - fGmtOffset + (12*HOUR_MS);

    U_DEBUG_ASTRO_MSG(("Noon=%.2lf, %sL, gmtoff %.2lf\n", noon, debug_astro_date(noon+fGmtOffset), fGmtOffset));
    setTime(noon +  ((rise ? -6 : 6) * HOUR_MS));
    U_DEBUG_ASTRO_MSG(("added %.2lf ms as a guess,\n", ((rise ? -6. : 6.) * HOUR_MS)));

    RiseSetCoordFunc func;
    double t = riseOrSet(func,
                         rise,
                         .533 * DEG_RAD,        
                         34. /60.0 * DEG_RAD,    
                         MINUTE_MS / 12.);       

    setTime(t0);
    return t;
}





















































































































































































































































































#define moonL0  (318.351648 * CalendarAstronomer::PI/180 )   // Mean long. at epoch
#define moonP0 ( 36.340410 * CalendarAstronomer::PI/180 )   // Mean long. of perigee
#define moonN0 ( 318.510107 * CalendarAstronomer::PI/180 )   // Mean long. of node
#define moonI  (   5.145366 * CalendarAstronomer::PI/180 )   // Inclination of orbit
#define moonE  (   0.054900 )            // Eccentricity of orbit


#define moonA  (   3.84401e5 )           // semi-major axis (km)
#define moonT0 (   0.5181 * CalendarAstronomer::PI/180 )     // Angular size at distance A
#define moonPi (   0.9507 * CalendarAstronomer::PI/180 )     // Parallax at distance A







const CalendarAstronomer::Equatorial& CalendarAstronomer::getMoonPosition()
{
    
    
    
    
    if (moonPositionSet == FALSE) {
        
        
        getSunLongitude();

        
        
        
        
        double day = getJulianDay() - JD_EPOCH;       

        
        
        double meanLongitude = norm2PI(13.1763966*PI/180*day + moonL0);
        meanAnomalyMoon = norm2PI(meanLongitude - 0.1114041*PI/180 * day - moonP0);

        
        
        
        
        
        
        double evection = 1.2739*PI/180 * ::sin(2 * (meanLongitude - sunLongitude)
            - meanAnomalyMoon);
        double annual   = 0.1858*PI/180 * ::sin(meanAnomalySun);
        double a3       = 0.3700*PI/180 * ::sin(meanAnomalySun);

        meanAnomalyMoon += evection - annual - a3;

        
        
        
        
        
        
        
        double center = 6.2886*PI/180 * ::sin(meanAnomalyMoon);
        double a4 =     0.2140*PI/180 * ::sin(2 * meanAnomalyMoon);

        
        moonLongitude = meanLongitude + evection + center - annual + a4;

        
        
        
        
        
        double variation = 0.6583*CalendarAstronomer::PI/180 * ::sin(2*(moonLongitude - sunLongitude));

        moonLongitude += variation;

        
        
        
        
        
        
        
        double nodeLongitude = norm2PI(moonN0 - 0.0529539*PI/180 * day);

        nodeLongitude -= 0.16*PI/180 * ::sin(meanAnomalySun);

        double y = ::sin(moonLongitude - nodeLongitude);
        double x = cos(moonLongitude - nodeLongitude);

        moonEclipLong = ::atan2(y*cos(moonI), x) + nodeLongitude;
        double moonEclipLat = ::asin(y * ::sin(moonI));

        eclipticToEquatorial(moonPosition, moonEclipLong, moonEclipLat);
        moonPositionSet = TRUE;
    }
    return moonPosition;
}











double CalendarAstronomer::getMoonAge() {
    
    
    
    
    
    
    getMoonPosition();

    return norm2PI(moonEclipLong - sunLongitude);
}
















double CalendarAstronomer::getMoonPhase() {
    
    
    return 0.5 * (1 - cos(getMoonAge()));
}







const CalendarAstronomer::MoonAge CalendarAstronomer::NEW_MOON() {
    return  CalendarAstronomer::MoonAge(0);
}

















const CalendarAstronomer::MoonAge CalendarAstronomer::FULL_MOON() {
    return   CalendarAstronomer::MoonAge(CalendarAstronomer::PI);
}







class MoonTimeAngleFunc : public CalendarAstronomer::AngleFunc {
public:
    virtual ~MoonTimeAngleFunc();
    virtual double eval(CalendarAstronomer&a) { return a.getMoonAge(); }
};

MoonTimeAngleFunc::~MoonTimeAngleFunc() {}















UDate CalendarAstronomer::getMoonTime(double desired, UBool next)
{
    MoonTimeAngleFunc func;
    return timeOfAngle( func,
                        desired,
                        SYNODIC_MONTH,
                        MINUTE_MS,
                        next);
}











UDate CalendarAstronomer::getMoonTime(const CalendarAstronomer::MoonAge& desired, UBool next) {
    return getMoonTime(desired.value, next);
}

class MoonRiseSetCoordFunc : public CalendarAstronomer::CoordFunc {
public:
    virtual ~MoonRiseSetCoordFunc();
    virtual void eval(CalendarAstronomer::Equatorial& result, CalendarAstronomer&a) { result = a.getMoonPosition(); }
};

MoonRiseSetCoordFunc::~MoonRiseSetCoordFunc() {}







UDate CalendarAstronomer::getMoonRiseSet(UBool rise)
{
    MoonRiseSetCoordFunc func;
    return riseOrSet(func,
                     rise,
                     .533 * DEG_RAD,        
                     34 /60.0 * DEG_RAD,    
                     MINUTE_MS);            
}





UDate CalendarAstronomer::timeOfAngle(AngleFunc& func, double desired,
                                      double periodDays, double epsilon, UBool next)
{
    
    double lastAngle = func.eval(*this);

    
    double deltaAngle = norm2PI(desired - lastAngle) ;

    
    
    double deltaT =  (deltaAngle + (next ? 0.0 : - CalendarAstronomer_PI2 )) * (periodDays*DAY_MS) / CalendarAstronomer_PI2;

    double lastDeltaT = deltaT; 
    UDate startTime = fTime; 

    setTime(fTime + uprv_ceil(deltaT));

    
    
    
    do {
        
        double angle = func.eval(*this);

        
        double factor = uprv_fabs(deltaT / normPI(angle-lastAngle));

        
        deltaT = normPI(desired - angle) * factor;

        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        if (uprv_fabs(deltaT) > uprv_fabs(lastDeltaT)) {
            double delta = uprv_ceil (periodDays * DAY_MS / 8.0);
            setTime(startTime + (next ? delta : -delta));
            return timeOfAngle(func, desired, periodDays, epsilon, next);
        }

        lastDeltaT = deltaT;
        lastAngle = angle;

        setTime(fTime + uprv_ceil(deltaT));
    }
    while (uprv_fabs(deltaT) > epsilon);

    return fTime;
}

UDate CalendarAstronomer::riseOrSet(CoordFunc& func, UBool rise,
                                    double diameter, double refraction,
                                    double epsilon)
{
    Equatorial pos;
    double      tanL   = ::tan(fLatitude);
    double     deltaT = 0;
    int32_t         count = 0;

    
    
    
    
    
    U_DEBUG_ASTRO_MSG(("setup rise=%s, dia=%.3lf, ref=%.3lf, eps=%.3lf\n",
        rise?"T":"F", diameter, refraction, epsilon));
    do {
        
        func.eval(pos, *this);
        double angle = ::acos(-tanL * ::tan(pos.declination));
        double lst = ((rise ? CalendarAstronomer_PI2-angle : angle) + pos.ascension ) * 24 / CalendarAstronomer_PI2;

        
        UDate newTime = lstToUT( lst );

        deltaT = newTime - fTime;
        setTime(newTime);
        U_DEBUG_ASTRO_MSG(("%d] dT=%.3lf, angle=%.3lf, lst=%.3lf,   A=%.3lf/D=%.3lf\n",
            count, deltaT, angle, lst, pos.ascension, pos.declination));
    }
    while (++ count < 5 && uprv_fabs(deltaT) > epsilon);

    
    double cosD  = ::cos(pos.declination);
    double psi   = ::acos(sin(fLatitude) / cosD);
    double x     = diameter / 2 + refraction;
    double y     = ::asin(sin(x) / ::sin(psi));
    long  delta  = (long)((240 * y * RAD_DEG / cosD)*SECOND_MS);

    return fTime + (rise ? -delta : delta);
}
											   







double CalendarAstronomer::eclipticObliquity() {
    if (isINVALID(eclipObliquity)) {
        const double epoch = 2451545.0;     

        double T = (getJulianDay() - epoch) / 36525;

        eclipObliquity = 23.439292
            - 46.815/3600 * T
            - 0.0006/3600 * T*T
            + 0.00181/3600 * T*T*T;

        eclipObliquity *= DEG_RAD;
    }
    return eclipObliquity;
}





void CalendarAstronomer::clearCache() {
    const double INVALID = uprv_getNaN();

    julianDay       = INVALID;
    julianCentury   = INVALID;
    sunLongitude    = INVALID;
    meanAnomalySun  = INVALID;
    moonLongitude   = INVALID;
    moonEclipLong   = INVALID;
    meanAnomalyMoon = INVALID;
    eclipObliquity  = INVALID;
    siderealTime    = INVALID;
    siderealT0      = INVALID;
    moonPositionSet = FALSE;
}





























UnicodeString CalendarAstronomer::Ecliptic::toString() const
{
#ifdef U_DEBUG_ASTRO
    char tmp[800];
    sprintf(tmp, "[%.5f,%.5f]", longitude*RAD_DEG, latitude*RAD_DEG);
    return UnicodeString(tmp, "");
#else
    return UnicodeString();
#endif
}

UnicodeString CalendarAstronomer::Equatorial::toString() const
{
#ifdef U_DEBUG_ASTRO
    char tmp[400];
    sprintf(tmp, "%f,%f",
        (ascension*RAD_DEG), (declination*RAD_DEG));
    return UnicodeString(tmp, "");
#else
    return UnicodeString();
#endif
}

UnicodeString CalendarAstronomer::Horizon::toString() const
{
#ifdef U_DEBUG_ASTRO
    char tmp[800];
    sprintf(tmp, "[%.5f,%.5f]", altitude*RAD_DEG, azimuth*RAD_DEG);
    return UnicodeString(tmp, "");
#else
    return UnicodeString();
#endif
}




















void CalendarCache::createCache(CalendarCache** cache, UErrorCode& status) {
    ucln_i18n_registerCleanup(UCLN_I18N_ASTRO_CALENDAR, calendar_astro_cleanup);
    if(cache == NULL) {
        status = U_MEMORY_ALLOCATION_ERROR;
    } else {
        *cache = new CalendarCache(32, status);
        if(U_FAILURE(status)) {
            delete *cache;
            *cache = NULL;
        }
    }
}

int32_t CalendarCache::get(CalendarCache** cache, int32_t key, UErrorCode &status) {
    int32_t res;

    if(U_FAILURE(status)) {
        return 0;
    }
    umtx_lock(&ccLock);

    if(*cache == NULL) {
        createCache(cache, status);
        if(U_FAILURE(status)) {
            umtx_unlock(&ccLock);
            return 0;
        }
    }

    res = uhash_igeti((*cache)->fTable, key);
    U_DEBUG_ASTRO_MSG(("%p: GET: [%d] == %d\n", (*cache)->fTable, key, res));

    umtx_unlock(&ccLock);
    return res;
}

void CalendarCache::put(CalendarCache** cache, int32_t key, int32_t value, UErrorCode &status) {
    if(U_FAILURE(status)) {
        return;
    }
    umtx_lock(&ccLock);

    if(*cache == NULL) {
        createCache(cache, status);
        if(U_FAILURE(status)) {
            umtx_unlock(&ccLock);
            return;
        }
    }

    uhash_iputi((*cache)->fTable, key, value, &status);
    U_DEBUG_ASTRO_MSG(("%p: PUT: [%d] := %d\n", (*cache)->fTable, key, value));

    umtx_unlock(&ccLock);
}

CalendarCache::CalendarCache(int32_t size, UErrorCode &status) {
    fTable = uhash_openSize(uhash_hashLong, uhash_compareLong, NULL, size, &status);
    U_DEBUG_ASTRO_MSG(("%p: Opening.\n", fTable));
}

CalendarCache::~CalendarCache() {
    if(fTable != NULL) {
        U_DEBUG_ASTRO_MSG(("%p: Closing.\n", fTable));
        uhash_close(fTable);
    }
}

U_NAMESPACE_END

#endif 
