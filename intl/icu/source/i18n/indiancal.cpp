








#include "indiancal.h"
#include <stdlib.h>
#if !UCONFIG_NO_FORMATTING

#include "mutex.h"
#include <float.h>
#include "gregoimp.h" 
#include "astro.h" 
#include "uhash.h"
#include "ucln_in.h"


#ifdef U_DEBUG_INDIANCAL
#include <stdio.h>
#include <stdarg.h>

#endif

U_NAMESPACE_BEGIN








Calendar* IndianCalendar::clone() const {
  return new IndianCalendar(*this);
}

IndianCalendar::IndianCalendar(const Locale& aLocale, UErrorCode& success)
  :   Calendar(TimeZone::createDefault(), aLocale, success)
{
  setTimeInMillis(getNow(), success); 
}

IndianCalendar::IndianCalendar(const IndianCalendar& other) : Calendar(other) {
}

IndianCalendar::~IndianCalendar()
{
}
const char *IndianCalendar::getType() const { 
   return "indian";
}
  
static const int32_t LIMITS[UCAL_FIELD_COUNT][4] = {
    
    
    {        0,        0,        0,        0}, 
    { -5000000, -5000000,  5000000,  5000000}, 
    {        0,        0,       11,       11}, 
    {        1,        1,       52,       53}, 
    {-1,-1,-1,-1}, 
    {        1,        1,       30,       31}, 
    {        1,        1,      365,      366}, 
    {-1,-1,-1,-1}, 
    {       -1,       -1,        5,        5}, 
    {-1,-1,-1,-1}, 
    {-1,-1,-1,-1}, 
    {-1,-1,-1,-1}, 
    {-1,-1,-1,-1}, 
    {-1,-1,-1,-1}, 
    {-1,-1,-1,-1}, 
    {-1,-1,-1,-1}, 
    {-1,-1,-1,-1}, 
    { -5000000, -5000000,  5000000,  5000000}, 
    {-1,-1,-1,-1}, 
    { -5000000, -5000000,  5000000,  5000000}, 
    {-1,-1,-1,-1}, 
    {-1,-1,-1,-1}, 
    {-1,-1,-1,-1}, 
};

static const double JULIAN_EPOCH = 1721425.5;
static const int32_t INDIAN_ERA_START  = 78;
static const int32_t INDIAN_YEAR_START = 80;

int32_t IndianCalendar::handleGetLimit(UCalendarDateFields field, ELimitType limitType) const {
  return LIMITS[field][limitType];
}




static UBool isGregorianLeap(int32_t year)
{
    return ((year % 4) == 0) && (!(((year % 100) == 0) && ((year % 400) != 0))); 
}
  










int32_t IndianCalendar::handleGetMonthLength(int32_t eyear, int32_t month) const {
   if (month < 0 || month > 11) {
      eyear += ClockMath::floorDivide(month, 12, month);
   }

   if (isGregorianLeap(eyear + INDIAN_ERA_START) && month == 0) {
       return 31;
   }

   if (month >= 1 && month <= 5) {
       return 31;
   }

   return 30;
}






int32_t IndianCalendar::handleGetYearLength(int32_t eyear) const {
    return isGregorianLeap(eyear + INDIAN_ERA_START) ? 366 : 365;
}







static double gregorianToJD(int32_t year, int32_t month, int32_t date) {
   double julianDay = (JULIAN_EPOCH - 1) +
      (365 * (year - 1)) +
      uprv_floor((year - 1) / 4) +
      (-uprv_floor((year - 1) / 100)) +
      uprv_floor((year - 1) / 400) +
      uprv_floor((((367 * month) - 362) / 12) +
            ((month <= 2) ? 0 :
             (isGregorianLeap(year) ? -1 : -2)
            ) +
            date);

   return julianDay;
}





static int32_t* jdToGregorian(double jd, int32_t gregorianDate[3]) {
   double wjd, depoch, quadricent, dqc, cent, dcent, quad, dquad, yindex, yearday, leapadj;
   int32_t year, month, day;
   wjd = uprv_floor(jd - 0.5) + 0.5;
   depoch = wjd - JULIAN_EPOCH;
   quadricent = uprv_floor(depoch / 146097);
   dqc = (int32_t)uprv_floor(depoch) % 146097;
   cent = uprv_floor(dqc / 36524);
   dcent = (int32_t)uprv_floor(dqc) % 36524;
   quad = uprv_floor(dcent / 1461);
   dquad = (int32_t)uprv_floor(dcent) % 1461;
   yindex = uprv_floor(dquad / 365);
   year = (int32_t)((quadricent * 400) + (cent * 100) + (quad * 4) + yindex);
   if (!((cent == 4) || (yindex == 4))) {
      year++;
   }
   yearday = wjd - gregorianToJD(year, 1, 1);
   leapadj = ((wjd < gregorianToJD(year, 3, 1)) ? 0
         :
         (isGregorianLeap(year) ? 1 : 2)
         );
   month = (int32_t)uprv_floor((((yearday + leapadj) * 12) + 373) / 367);
   day = (int32_t)(wjd - gregorianToJD(year, month, 1)) + 1;

   gregorianDate[0] = year;
   gregorianDate[1] = month;
   gregorianDate[2] = day;

   return gregorianDate;
}

   



static double IndianToJD(int32_t year, int32_t month, int32_t date) {
   int32_t leapMonth, gyear, m;
   double start, jd;

   gyear = year + INDIAN_ERA_START;


   if(isGregorianLeap(gyear)) {
      leapMonth = 31;
      start = gregorianToJD(gyear, 3, 21);
   } 
   else {
      leapMonth = 30;
      start = gregorianToJD(gyear, 3, 22);
   }

   if (month == 1) {
      jd = start + (date - 1);
   } else {
      jd = start + leapMonth;
      m = month - 2;

      
      if (m > 5) {
          m = 5;
      }

      jd += m * 31;

      if (month >= 8) {
         m = month - 7;
         jd += m * 30;
      }
      jd += date - 1;
   }

   return jd;
}






int32_t IndianCalendar::handleComputeMonthStart(int32_t eyear, int32_t month, UBool  ) const {

   
   int32_t imonth;

    
   if (month < 0 || month > 11) {
      eyear += (int32_t)ClockMath::floorDivide(month, 12, month);
   }

   if(month == 12){
       imonth = 1;
   } else {
       imonth = month + 1; 
   }
   
   double jd = IndianToJD(eyear ,imonth, 1);

   return (int32_t)jd;
}





int32_t IndianCalendar::handleGetExtendedYear() {
    int32_t year;

    if (newerField(UCAL_EXTENDED_YEAR, UCAL_YEAR) == UCAL_EXTENDED_YEAR) {
        year = internalGet(UCAL_EXTENDED_YEAR, 1); 
    } else {
        year = internalGet(UCAL_YEAR, 1); 
    }

    return year;
}















void IndianCalendar::handleComputeFields(int32_t julianDay, UErrorCode&  ) {
    double jdAtStartOfGregYear;
    int32_t leapMonth, IndianYear, yday, IndianMonth, IndianDayOfMonth, mday;
    int32_t gregorianYear;      
    int32_t gd[3];

    gregorianYear = jdToGregorian(julianDay, gd)[0];          
    IndianYear = gregorianYear - INDIAN_ERA_START;            
    jdAtStartOfGregYear = gregorianToJD(gregorianYear, 1, 1); 
    yday = (int32_t)(julianDay - jdAtStartOfGregYear);        

    if (yday < INDIAN_YEAR_START) {
        
        IndianYear -= 1;
        leapMonth = isGregorianLeap(gregorianYear - 1) ? 31 : 30; 
        yday += leapMonth + (31 * 5) + (30 * 3) + 10;
    } else {
        leapMonth = isGregorianLeap(gregorianYear) ? 31 : 30; 
        yday -= INDIAN_YEAR_START;
    }

    if (yday < leapMonth) {
        IndianMonth = 0;
        IndianDayOfMonth = yday + 1;
    } else {
        mday = yday - leapMonth;
        if (mday < (31 * 5)) {
            IndianMonth = (int32_t)uprv_floor(mday / 31) + 1;
            IndianDayOfMonth = (mday % 31) + 1;
        } else {
            mday -= 31 * 5;
            IndianMonth = (int32_t)uprv_floor(mday / 30) + 6;
            IndianDayOfMonth = (mday % 30) + 1;
        }
   }

   internalSet(UCAL_ERA, 0);
   internalSet(UCAL_EXTENDED_YEAR, IndianYear);
   internalSet(UCAL_YEAR, IndianYear);
   internalSet(UCAL_MONTH, IndianMonth);
   internalSet(UCAL_DAY_OF_MONTH, IndianDayOfMonth);
   internalSet(UCAL_DAY_OF_YEAR, yday + 1); 
}    

UBool
IndianCalendar::inDaylightTime(UErrorCode& status) const
{
    
    if (U_FAILURE(status) || !getTimeZone().useDaylightTime()) {
        return FALSE;
    }

    
    ((IndianCalendar*)this)->complete(status); 

    return (UBool)(U_SUCCESS(status) ? (internalGet(UCAL_DST_OFFSET) != 0) : FALSE);
}


const UDate     IndianCalendar::fgSystemDefaultCentury          = DBL_MIN;
const int32_t   IndianCalendar::fgSystemDefaultCenturyYear      = -1;

UDate           IndianCalendar::fgSystemDefaultCenturyStart     = DBL_MIN;
int32_t         IndianCalendar::fgSystemDefaultCenturyStartYear = -1;


UBool IndianCalendar::haveDefaultCentury() const
{
    return TRUE;
}

UDate IndianCalendar::defaultCenturyStart() const
{
    return internalGetDefaultCenturyStart();
}

int32_t IndianCalendar::defaultCenturyStartYear() const
{
    return internalGetDefaultCenturyStartYear();
}

UDate
IndianCalendar::internalGetDefaultCenturyStart() const
{
    
    UBool needsUpdate;
    { 
        Mutex m;
        needsUpdate = (fgSystemDefaultCenturyStart == fgSystemDefaultCentury);
    }

    if (needsUpdate) {
        initializeSystemDefaultCentury();
    }

    
    

    return fgSystemDefaultCenturyStart;
}

int32_t
IndianCalendar::internalGetDefaultCenturyStartYear() const
{
    
    UBool needsUpdate;
    { 
        Mutex m;

        needsUpdate = (fgSystemDefaultCenturyStart == fgSystemDefaultCentury);
    }

    if (needsUpdate) {
        initializeSystemDefaultCentury();
    }

    
    

    return    fgSystemDefaultCenturyStartYear;
}

void
IndianCalendar::initializeSystemDefaultCentury()
{
    
    
    
    
    if (fgSystemDefaultCenturyStart == fgSystemDefaultCentury) {
        UErrorCode status = U_ZERO_ERROR;

        IndianCalendar calendar(Locale("@calendar=Indian"),status);
        if (U_SUCCESS(status)) {
            calendar.setTime(Calendar::getNow(), status);
            calendar.add(UCAL_YEAR, -80, status);

            UDate    newStart = calendar.getTime(status);
            int32_t  newYear  = calendar.get(UCAL_YEAR, status);

            {
                Mutex m;

                fgSystemDefaultCenturyStart = newStart;
                fgSystemDefaultCenturyStartYear = newYear;
            }
        }

        
        
    }
}

UOBJECT_DEFINE_RTTI_IMPLEMENTATION(IndianCalendar)

U_NAMESPACE_END

#endif

