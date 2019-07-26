










#include "gregoimp.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/ucal.h"
#include "uresimp.h"
#include "cstring.h"
#include "uassert.h"

#if defined(U_DEBUG_CALDATA)
#include <stdio.h>
#endif

U_NAMESPACE_BEGIN

int32_t ClockMath::floorDivide(int32_t numerator, int32_t denominator) {
    return (numerator >= 0) ?
        numerator / denominator : ((numerator + 1) / denominator) - 1;
}

int32_t ClockMath::floorDivide(double numerator, int32_t denominator,
                          int32_t& remainder) {
    double quotient;
    quotient = uprv_floor(numerator / denominator);
    remainder = (int32_t) (numerator - (quotient * denominator));
    return (int32_t) quotient;
}

double ClockMath::floorDivide(double dividend, double divisor,
                         double& remainder) {
    
    U_ASSERT(divisor > 0);
    double quotient = floorDivide(dividend, divisor);
    remainder = dividend - (quotient * divisor);
    
    
    
    if (remainder < 0 || remainder >= divisor) {
        
        
        
        double q = quotient;
        quotient += (remainder < 0) ? -1 : +1;
        if (q == quotient) {
            
            
            
            
            
            
            
            
            
            remainder = 0;
        } else {
            remainder = dividend - (quotient * divisor);
        }
    }
    U_ASSERT(0 <= remainder && remainder < divisor);
    return quotient;
}

const int32_t JULIAN_1_CE    = 1721426; 
const int32_t JULIAN_1970_CE = 2440588; 

const int16_t Grego::DAYS_BEFORE[24] =
    {0,31,59,90,120,151,181,212,243,273,304,334,
     0,31,60,91,121,152,182,213,244,274,305,335};

const int8_t Grego::MONTH_LENGTH[24] =
    {31,28,31,30,31,30,31,31,30,31,30,31,
     31,29,31,30,31,30,31,31,30,31,30,31};

double Grego::fieldsToDay(int32_t year, int32_t month, int32_t dom) {

    int32_t y = year - 1;

    double julian = 365 * y + ClockMath::floorDivide(y, 4) + (JULIAN_1_CE - 3) + 
        ClockMath::floorDivide(y, 400) - ClockMath::floorDivide(y, 100) + 2 + 
        DAYS_BEFORE[month + (isLeapYear(year) ? 12 : 0)] + dom; 

    return julian - JULIAN_1970_CE; 
}

void Grego::dayToFields(double day, int32_t& year, int32_t& month,
                        int32_t& dom, int32_t& dow, int32_t& doy) {

    
    day += JULIAN_1970_CE - JULIAN_1_CE;

    
    
    
    
    int32_t n400 = ClockMath::floorDivide(day, 146097, doy); 
    int32_t n100 = ClockMath::floorDivide(doy, 36524, doy); 
    int32_t n4   = ClockMath::floorDivide(doy, 1461, doy); 
    int32_t n1   = ClockMath::floorDivide(doy, 365, doy);
    year = 400*n400 + 100*n100 + 4*n4 + n1;
    if (n100 == 4 || n1 == 4) {
        doy = 365; 
    } else {
        ++year;
    }
    
    UBool isLeap = isLeapYear(year);
    
    
    dow = (int32_t) uprv_fmod(day + 1, 7);
    dow += (dow < 0) ? (UCAL_SUNDAY + 7) : UCAL_SUNDAY;

    
    int32_t correction = 0;
    int32_t march1 = isLeap ? 60 : 59; 
    if (doy >= march1) {
        correction = isLeap ? 1 : 2;
    }
    month = (12 * (doy + correction) + 6) / 367; 
    dom = doy - DAYS_BEFORE[month + (isLeap ? 12 : 0)] + 1; 
    doy++; 
}

void Grego::timeToFields(UDate time, int32_t& year, int32_t& month,
                        int32_t& dom, int32_t& dow, int32_t& doy, int32_t& mid) {
    double millisInDay;
    double day = ClockMath::floorDivide((double)time, (double)U_MILLIS_PER_DAY, millisInDay);
    mid = (int32_t)millisInDay;
    dayToFields(day, year, month, dom, dow, doy);
}

int32_t Grego::dayOfWeek(double day) {
    int32_t dow;
    ClockMath::floorDivide(day + UCAL_THURSDAY, 7, dow);
    return (dow == 0) ? UCAL_SATURDAY : dow;
}

int32_t Grego::dayOfWeekInMonth(int32_t year, int32_t month, int32_t dom) {
    int32_t weekInMonth = (dom + 6)/7;
    if (weekInMonth == 4) {
        if (dom + 7 > monthLength(year, month)) {
            weekInMonth = -1;
        }
    } else if (weekInMonth == 5) {
        weekInMonth = -1;
    }
    return weekInMonth;
}



#define U_CALENDAR_KEY "calendar"
#define U_GREGORIAN_KEY "gregorian"
#define U_FORMAT_KEY "format"
#define U_DEFAULT_KEY "default"
#define U_CALENDAR_DATA ((char*)0)







CalendarData::CalendarData(const Locale& loc, const char *type, UErrorCode& status)
  : fFillin(NULL), fOtherFillin(NULL), fBundle(NULL), fFallback(NULL) {
  initData(loc.getBaseName(), type, status);
}

void CalendarData::initData(const char *locale, const char *type, UErrorCode& status) {
  fOtherFillin = ures_open(U_CALENDAR_DATA, locale, &status);
  fFillin = ures_getByKey(fOtherFillin, U_CALENDAR_KEY, fFillin, &status);

  if((type != NULL) && 
     (*type != '\0') && 
     (uprv_strcmp(type, U_GREGORIAN_KEY)))
  {
    fBundle = ures_getByKeyWithFallback(fFillin, type, NULL, &status);
    fFallback = ures_getByKeyWithFallback(fFillin, U_GREGORIAN_KEY, NULL, &status);

#if defined (U_DEBUG_CALDATA)
    fprintf(stderr, "%p: CalendarData(%s, %s, %s) -> main(%p, %s)=%s, fallback(%p, %s)=%s\n", 
            this, locale, type, u_errorName(status), fBundle, type, fBundle?ures_getLocale(fBundle, &status):"", 
            fFallback, U_GREGORIAN_KEY, fFallback?ures_getLocale(fFallback, &status):"");
#endif

  } else {
    fBundle = ures_getByKeyWithFallback(fFillin, U_GREGORIAN_KEY, NULL, &status);
#if defined (U_DEBUG_CALDATA)
    fprintf(stderr, "%p: CalendarData(%s, %s, %s) -> main(%p, %s)=%s, fallback = NULL\n",
            this, locale, type, u_errorName(status), fBundle, U_GREGORIAN_KEY, fBundle?ures_getLocale(fBundle, &status):"" );
#endif
  }
}

CalendarData::~CalendarData() {
    ures_close(fFillin);
    ures_close(fBundle);
    ures_close(fFallback);
    ures_close(fOtherFillin);
}

UResourceBundle*
CalendarData::getByKey(const char *key, UErrorCode& status) {
    if(U_FAILURE(status)) {
        return NULL;
    }

    if(fBundle) {
        fFillin = ures_getByKeyWithFallback(fBundle, key, fFillin, &status);
#if defined (U_DEBUG_CALDATA)
        fprintf(stderr, "%p: get %s -> %s - from MAIN %s\n",this, key, u_errorName(status), ures_getLocale(fFillin, &status));
#endif
    }
    if(fFallback && (status == U_MISSING_RESOURCE_ERROR)) {
        status = U_ZERO_ERROR; 
        fFillin = ures_getByKeyWithFallback(fFallback, key, fFillin, &status);
#if defined (U_DEBUG_CALDATA)
        fprintf(stderr, "%p: get %s -> %s - from FALLBACK %s\n",this, key, u_errorName(status), ures_getLocale(fFillin, &status));
#endif
    }
    return fFillin;
}

UResourceBundle* CalendarData::getByKey2(const char *key, const char *subKey, UErrorCode& status) {
    if(U_FAILURE(status)) {
        return NULL;
    }

    if(fBundle) {
#if defined (U_DEBUG_CALDATA)
        fprintf(stderr, "%p: //\n");
#endif
        fFillin = ures_getByKeyWithFallback(fBundle, key, fFillin, &status);
        fOtherFillin = ures_getByKeyWithFallback(fFillin, U_FORMAT_KEY, fOtherFillin, &status);
        fFillin = ures_getByKeyWithFallback(fOtherFillin, subKey, fFillin, &status);
#if defined (U_DEBUG_CALDATA)
        fprintf(stderr, "%p: get %s/format/%s -> %s - from MAIN %s\n", this, key, subKey, u_errorName(status), ures_getLocale(fFillin, &status));
#endif
    }
    if(fFallback && (status == U_MISSING_RESOURCE_ERROR)) {
        status = U_ZERO_ERROR; 
        fFillin = ures_getByKeyWithFallback(fFallback, key, fFillin, &status);
        fOtherFillin = ures_getByKeyWithFallback(fFillin, U_FORMAT_KEY, fOtherFillin, &status);
        fFillin = ures_getByKeyWithFallback(fOtherFillin, subKey, fFillin, &status);
#if defined (U_DEBUG_CALDATA)
        fprintf(stderr, "%p: get %s/format/%s -> %s - from FALLBACK %s\n",this, key, subKey, u_errorName(status), ures_getLocale(fFillin,&status));
#endif
    }


































    return fFillin;
}

UResourceBundle* CalendarData::getByKey3(const char *key, const char *contextKey, const char *subKey, UErrorCode& status) {
    if(U_FAILURE(status)) {
        return NULL;
    }

    if(fBundle) {
#if defined (U_DEBUG_CALDATA)
        fprintf(stderr, "%p: //\n");
#endif
        fFillin = ures_getByKeyWithFallback(fBundle, key, fFillin, &status);
        fOtherFillin = ures_getByKeyWithFallback(fFillin, contextKey, fOtherFillin, &status);
        fFillin = ures_getByKeyWithFallback(fOtherFillin, subKey, fFillin, &status);
#if defined (U_DEBUG_CALDATA)
        fprintf(stderr, "%p: get %s/%s/%s -> %s - from MAIN %s\n", this, key, contextKey, subKey, u_errorName(status), ures_getLocale(fFillin, &status));
#endif
    }
    if(fFallback && (status == U_MISSING_RESOURCE_ERROR)) {
        status = U_ZERO_ERROR; 
        fFillin = ures_getByKeyWithFallback(fFallback, key, fFillin, &status);
        fOtherFillin = ures_getByKeyWithFallback(fFillin, contextKey, fOtherFillin, &status);
        fFillin = ures_getByKeyWithFallback(fOtherFillin, subKey, fFillin, &status);
#if defined (U_DEBUG_CALDATA)
        fprintf(stderr, "%p: get %s/%s/%s -> %s - from FALLBACK %s\n",this, key, contextKey, subKey, u_errorName(status), ures_getLocale(fFillin,&status));
#endif
    }

    return fFillin;
}

U_NAMESPACE_END

#endif

