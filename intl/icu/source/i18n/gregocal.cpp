






































#include "unicode/utypes.h"
#include <float.h>

#if !UCONFIG_NO_FORMATTING

#include "unicode/gregocal.h"
#include "gregoimp.h"
#include "umutex.h"
#include "uassert.h"











static const int16_t kNumDays[]
= {0,31,59,90,120,151,181,212,243,273,304,334}; 
static const int16_t kLeapNumDays[]
= {0,31,60,91,121,152,182,213,244,274,305,335}; 
static const int8_t kMonthLength[]
= {31,28,31,30,31,30,31,31,30,31,30,31}; 
static const int8_t kLeapMonthLength[]
= {31,29,31,30,31,30,31,31,30,31,30,31}; 








static const int32_t kGregorianCalendarLimits[UCAL_FIELD_COUNT][4] = {
    
    
    {        0,        0,        1,        1}, 
    {        1,        1,   140742,   144683}, 
    {        0,        0,       11,       11}, 
    {        1,        1,       52,       53}, 
    {-1,-1,-1,-1}, 
    {        1,        1,       28,       31}, 
    {        1,        1,      365,      366}, 
    {-1,-1,-1,-1}, 
    {       -1,       -1,        4,        5}, 
    {-1,-1,-1,-1}, 
    {-1,-1,-1,-1}, 
    {-1,-1,-1,-1}, 
    {-1,-1,-1,-1}, 
    {-1,-1,-1,-1}, 
    {-1,-1,-1,-1}, 
    {-1,-1,-1,-1}, 
    {-1,-1,-1,-1}, 
    {  -140742,  -140742,   140742,   144683}, 
    {-1,-1,-1,-1}, 
    {  -140742,  -140742,   140742,   144683}, 
    {-1,-1,-1,-1}, 
    {-1,-1,-1,-1}, 
    {-1,-1,-1,-1}, 
};





























#if defined( U_DEBUG_CALSVC ) || defined (U_DEBUG_CAL)
#include <stdio.h>
#endif

U_NAMESPACE_BEGIN

UOBJECT_DEFINE_RTTI_IMPLEMENTATION(GregorianCalendar)







static const uint32_t kCutoverJulianDay = 2299161;
static const UDate kPapalCutover = (2299161.0 - kEpochStartAsJulianDay) * U_MILLIS_PER_DAY;




GregorianCalendar::GregorianCalendar(UErrorCode& status)
:   Calendar(status),
fGregorianCutover(kPapalCutover),
fCutoverJulianDay(kCutoverJulianDay), fNormalizedGregorianCutover(fGregorianCutover), fGregorianCutoverYear(1582),
fIsGregorian(TRUE), fInvertGregorian(FALSE)
{
    setTimeInMillis(getNow(), status);
}



GregorianCalendar::GregorianCalendar(TimeZone* zone, UErrorCode& status)
:   Calendar(zone, Locale::getDefault(), status),
fGregorianCutover(kPapalCutover),
fCutoverJulianDay(kCutoverJulianDay), fNormalizedGregorianCutover(fGregorianCutover), fGregorianCutoverYear(1582),
fIsGregorian(TRUE), fInvertGregorian(FALSE)
{
    setTimeInMillis(getNow(), status);
}



GregorianCalendar::GregorianCalendar(const TimeZone& zone, UErrorCode& status)
:   Calendar(zone, Locale::getDefault(), status),
fGregorianCutover(kPapalCutover),
fCutoverJulianDay(kCutoverJulianDay), fNormalizedGregorianCutover(fGregorianCutover), fGregorianCutoverYear(1582),
fIsGregorian(TRUE), fInvertGregorian(FALSE)
{
    setTimeInMillis(getNow(), status);
}



GregorianCalendar::GregorianCalendar(const Locale& aLocale, UErrorCode& status)
:   Calendar(TimeZone::createDefault(), aLocale, status),
fGregorianCutover(kPapalCutover),
fCutoverJulianDay(kCutoverJulianDay), fNormalizedGregorianCutover(fGregorianCutover), fGregorianCutoverYear(1582),
fIsGregorian(TRUE), fInvertGregorian(FALSE)
{
    setTimeInMillis(getNow(), status);
}



GregorianCalendar::GregorianCalendar(TimeZone* zone, const Locale& aLocale,
                                     UErrorCode& status)
                                     :   Calendar(zone, aLocale, status),
                                     fGregorianCutover(kPapalCutover),
                                     fCutoverJulianDay(kCutoverJulianDay), fNormalizedGregorianCutover(fGregorianCutover), fGregorianCutoverYear(1582),
                                     fIsGregorian(TRUE), fInvertGregorian(FALSE)
{
    setTimeInMillis(getNow(), status);
}



GregorianCalendar::GregorianCalendar(const TimeZone& zone, const Locale& aLocale,
                                     UErrorCode& status)
                                     :   Calendar(zone, aLocale, status),
                                     fGregorianCutover(kPapalCutover),
                                     fCutoverJulianDay(kCutoverJulianDay), fNormalizedGregorianCutover(fGregorianCutover), fGregorianCutoverYear(1582),
                                     fIsGregorian(TRUE), fInvertGregorian(FALSE)
{
    setTimeInMillis(getNow(), status);
}



GregorianCalendar::GregorianCalendar(int32_t year, int32_t month, int32_t date,
                                     UErrorCode& status)
                                     :   Calendar(TimeZone::createDefault(), Locale::getDefault(), status),
                                     fGregorianCutover(kPapalCutover),
                                     fCutoverJulianDay(kCutoverJulianDay), fNormalizedGregorianCutover(fGregorianCutover), fGregorianCutoverYear(1582),
                                     fIsGregorian(TRUE), fInvertGregorian(FALSE)
{
    set(UCAL_ERA, AD);
    set(UCAL_YEAR, year);
    set(UCAL_MONTH, month);
    set(UCAL_DATE, date);
}



GregorianCalendar::GregorianCalendar(int32_t year, int32_t month, int32_t date,
                                     int32_t hour, int32_t minute, UErrorCode& status)
                                     :   Calendar(TimeZone::createDefault(), Locale::getDefault(), status),
                                     fGregorianCutover(kPapalCutover),
                                     fCutoverJulianDay(kCutoverJulianDay), fNormalizedGregorianCutover(fGregorianCutover), fGregorianCutoverYear(1582),
                                     fIsGregorian(TRUE), fInvertGregorian(FALSE)
{
    set(UCAL_ERA, AD);
    set(UCAL_YEAR, year);
    set(UCAL_MONTH, month);
    set(UCAL_DATE, date);
    set(UCAL_HOUR_OF_DAY, hour);
    set(UCAL_MINUTE, minute);
}



GregorianCalendar::GregorianCalendar(int32_t year, int32_t month, int32_t date,
                                     int32_t hour, int32_t minute, int32_t second,
                                     UErrorCode& status)
                                     :   Calendar(TimeZone::createDefault(), Locale::getDefault(), status),
                                     fGregorianCutover(kPapalCutover),
                                     fCutoverJulianDay(kCutoverJulianDay), fNormalizedGregorianCutover(fGregorianCutover), fGregorianCutoverYear(1582),
                                     fIsGregorian(TRUE), fInvertGregorian(FALSE)
{
    set(UCAL_ERA, AD);
    set(UCAL_YEAR, year);
    set(UCAL_MONTH, month);
    set(UCAL_DATE, date);
    set(UCAL_HOUR_OF_DAY, hour);
    set(UCAL_MINUTE, minute);
    set(UCAL_SECOND, second);
}



GregorianCalendar::~GregorianCalendar()
{
}



GregorianCalendar::GregorianCalendar(const GregorianCalendar &source)
:   Calendar(source),
fGregorianCutover(source.fGregorianCutover),
fCutoverJulianDay(source.fCutoverJulianDay), fNormalizedGregorianCutover(source.fNormalizedGregorianCutover), fGregorianCutoverYear(source.fGregorianCutoverYear),
fIsGregorian(source.fIsGregorian), fInvertGregorian(source.fInvertGregorian)
{
}



Calendar* GregorianCalendar::clone() const
{
    return new GregorianCalendar(*this);
}



GregorianCalendar &
GregorianCalendar::operator=(const GregorianCalendar &right)
{
    if (this != &right)
    {
        Calendar::operator=(right);
        fGregorianCutover = right.fGregorianCutover;
        fNormalizedGregorianCutover = right.fNormalizedGregorianCutover;
        fGregorianCutoverYear = right.fGregorianCutoverYear;
        fCutoverJulianDay = right.fCutoverJulianDay;
    }
    return *this;
}



UBool GregorianCalendar::isEquivalentTo(const Calendar& other) const
{
    
    return Calendar::isEquivalentTo(other) &&
        fGregorianCutover == ((GregorianCalendar*)&other)->fGregorianCutover;
}



void
GregorianCalendar::setGregorianChange(UDate date, UErrorCode& status)
{
    if (U_FAILURE(status)) 
        return;

    fGregorianCutover = date;

    
    
    
    
    
    
    int32_t cutoverDay = (int32_t)ClockMath::floorDivide(fGregorianCutover, (double)kOneDay);
    fNormalizedGregorianCutover = cutoverDay * kOneDay;

    
    
    
    
    
    
    
    if (cutoverDay < 0 && fNormalizedGregorianCutover > 0) {
        fNormalizedGregorianCutover = (cutoverDay + 1) * kOneDay;
    }

    
    
    GregorianCalendar *cal = new GregorianCalendar(getTimeZone(), status);
    
    if (cal == 0) {
        status = U_MEMORY_ALLOCATION_ERROR;
        return;
    }
    if(U_FAILURE(status))
        return;
    cal->setTime(date, status);
    fGregorianCutoverYear = cal->get(UCAL_YEAR, status);
    if (cal->get(UCAL_ERA, status) == BC) 
        fGregorianCutoverYear = 1 - fGregorianCutoverYear;
    fCutoverJulianDay = cutoverDay;
    delete cal;
}


void GregorianCalendar::handleComputeFields(int32_t julianDay, UErrorCode& status) {
    int32_t eyear, month, dayOfMonth, dayOfYear, unusedRemainder;


    if(U_FAILURE(status)) { 
        return; 
    }

#if defined (U_DEBUG_CAL)
    fprintf(stderr, "%s:%d: jd%d- (greg's %d)- [cut=%d]\n", 
        __FILE__, __LINE__, julianDay, getGregorianDayOfYear(), fCutoverJulianDay);
#endif


    if (julianDay >= fCutoverJulianDay) {
        month = getGregorianMonth();
        dayOfMonth = getGregorianDayOfMonth();
        dayOfYear = getGregorianDayOfYear();
        eyear = getGregorianYear();
    } else {
        
        
        int32_t julianEpochDay = julianDay - (kJan1_1JulianDay - 2);
		eyear = (int32_t) ClockMath::floorDivide((4.0*julianEpochDay) + 1464.0, (int32_t) 1461, unusedRemainder);

        
        int32_t january1 = 365*(eyear-1) + ClockMath::floorDivide(eyear-1, (int32_t)4);
        dayOfYear = (julianEpochDay - january1); 

        
        
        
        
        
        
        UBool isLeap = ((eyear&0x3) == 0); 

        
        int32_t correction = 0;
        int32_t march1 = isLeap ? 60 : 59; 
        if (dayOfYear >= march1) {
            correction = isLeap ? 1 : 2;
        }
        month = (12 * (dayOfYear + correction) + 6) / 367; 
        dayOfMonth = dayOfYear - (isLeap?kLeapNumDays[month]:kNumDays[month]) + 1; 
        ++dayOfYear;
#if defined (U_DEBUG_CAL)
        
        
        
        
        
        fprintf(stderr, "%s:%d: doy %d (greg's %d)- [cut=%d]\n", 
            __FILE__, __LINE__, dayOfYear, getGregorianDayOfYear(), fCutoverJulianDay);
#endif

    }

    
    if((eyear == fGregorianCutoverYear) && (julianDay >= fCutoverJulianDay)) {
        
        int32_t gregShift = Grego::gregorianShift(eyear);
#if defined (U_DEBUG_CAL)
        fprintf(stderr, "%s:%d:  gregorian shift %d :::  doy%d => %d [cut=%d]\n",
            __FILE__, __LINE__,gregShift, dayOfYear, dayOfYear+gregShift, fCutoverJulianDay);
#endif
        dayOfYear += gregShift;
    }

    internalSet(UCAL_MONTH, month);
    internalSet(UCAL_DAY_OF_MONTH, dayOfMonth);
    internalSet(UCAL_DAY_OF_YEAR, dayOfYear);
    internalSet(UCAL_EXTENDED_YEAR, eyear);
    int32_t era = AD;
    if (eyear < 1) {
        era = BC;
        eyear = 1 - eyear;
    }
    internalSet(UCAL_ERA, era);
    internalSet(UCAL_YEAR, eyear);
}




UDate
GregorianCalendar::getGregorianChange() const
{
    return fGregorianCutover;
}



UBool 
GregorianCalendar::isLeapYear(int32_t year) const
{
    
    
    return (year >= fGregorianCutoverYear ?
        (((year&0x3) == 0) && ((year%100 != 0) || (year%400 == 0))) : 
    ((year&0x3) == 0)); 
}



int32_t GregorianCalendar::handleComputeJulianDay(UCalendarDateFields bestField) 
{
    fInvertGregorian = FALSE;

    int32_t jd = Calendar::handleComputeJulianDay(bestField);

    if((bestField == UCAL_WEEK_OF_YEAR) &&  
        (internalGet(UCAL_EXTENDED_YEAR)==fGregorianCutoverYear) && 
        jd >= fCutoverJulianDay) { 
            fInvertGregorian = TRUE;  
            return Calendar::handleComputeJulianDay(bestField);
        }


        
        
        
        if ((fIsGregorian==TRUE) != (jd >= fCutoverJulianDay)) {  
#if defined (U_DEBUG_CAL)
            fprintf(stderr, "%s:%d: jd [invert] %d\n", 
                __FILE__, __LINE__, jd);
#endif
            fInvertGregorian = TRUE;
            jd = Calendar::handleComputeJulianDay(bestField);
#if defined (U_DEBUG_CAL)
            fprintf(stderr, "%s:%d:  fIsGregorian %s, fInvertGregorian %s - ", 
                __FILE__, __LINE__,fIsGregorian?"T":"F", fInvertGregorian?"T":"F");
            fprintf(stderr, " jd NOW %d\n", 
                jd);
#endif
        } else {
#if defined (U_DEBUG_CAL)
            fprintf(stderr, "%s:%d: jd [==] %d - %sfIsGregorian %sfInvertGregorian, %d\n", 
                __FILE__, __LINE__, jd, fIsGregorian?"T":"F", fInvertGregorian?"T":"F", bestField);
#endif
        }

        if(fIsGregorian && (internalGet(UCAL_EXTENDED_YEAR) == fGregorianCutoverYear)) {
            int32_t gregShift = Grego::gregorianShift(internalGet(UCAL_EXTENDED_YEAR));
            if (bestField == UCAL_DAY_OF_YEAR) {
#if defined (U_DEBUG_CAL)
                fprintf(stderr, "%s:%d: [DOY%d] gregorian shift of JD %d += %d\n", 
                    __FILE__, __LINE__, fFields[bestField],jd, gregShift);
#endif
                jd -= gregShift;
            } else if ( bestField == UCAL_WEEK_OF_MONTH ) {
                int32_t weekShift = 14;
#if defined (U_DEBUG_CAL)
                fprintf(stderr, "%s:%d: [WOY/WOM] gregorian week shift of %d += %d\n", 
                    __FILE__, __LINE__, jd, weekShift);
#endif
                jd += weekShift; 
            }
        }

        return jd;
}

int32_t GregorianCalendar::handleComputeMonthStart(int32_t eyear, int32_t month,

                                                   UBool ) const
{
    GregorianCalendar *nonConstThis = (GregorianCalendar*)this; 

    
    
    if (month < 0 || month > 11) {
        eyear += ClockMath::floorDivide(month, 12, month);
    }

    UBool isLeap = eyear%4 == 0;
    int32_t y = eyear-1;
    int32_t julianDay = 365*y + ClockMath::floorDivide(y, 4) + (kJan1_1JulianDay - 3);

    nonConstThis->fIsGregorian = (eyear >= fGregorianCutoverYear);
#if defined (U_DEBUG_CAL)
    fprintf(stderr, "%s:%d: (hcms%d/%d) fIsGregorian %s, fInvertGregorian %s\n", 
        __FILE__, __LINE__, eyear,month, fIsGregorian?"T":"F", fInvertGregorian?"T":"F");
#endif
    if (fInvertGregorian) {
        nonConstThis->fIsGregorian = !fIsGregorian;
    }
    if (fIsGregorian) {
        isLeap = isLeap && ((eyear%100 != 0) || (eyear%400 == 0));
        
        
        int32_t gregShift = Grego::gregorianShift(eyear);
#if defined (U_DEBUG_CAL)
        fprintf(stderr, "%s:%d: (hcms%d/%d) gregorian shift of %d += %d\n", 
            __FILE__, __LINE__, eyear, month, julianDay, gregShift);
#endif
        julianDay += gregShift;
    }

    
    
    

    if (month != 0) {
        julianDay += isLeap?kLeapNumDays[month]:kNumDays[month];
    }

    return julianDay;
}

int32_t GregorianCalendar::handleGetMonthLength(int32_t extendedYear, int32_t month)  const
{
    
    
    if (month < 0 || month > 11) {
        extendedYear += ClockMath::floorDivide(month, 12, month);
    }

    return isLeapYear(extendedYear) ? kLeapMonthLength[month] : kMonthLength[month];
}

int32_t GregorianCalendar::handleGetYearLength(int32_t eyear) const {
    return isLeapYear(eyear) ? 366 : 365;
}


int32_t
GregorianCalendar::monthLength(int32_t month) const
{
    int32_t year = internalGet(UCAL_EXTENDED_YEAR);
    return handleGetMonthLength(year, month);
}



int32_t
GregorianCalendar::monthLength(int32_t month, int32_t year) const
{
    return isLeapYear(year) ? kLeapMonthLength[month] : kMonthLength[month];
}



int32_t
GregorianCalendar::yearLength(int32_t year) const
{
    return isLeapYear(year) ? 366 : 365;
}



int32_t
GregorianCalendar::yearLength() const
{
    return isLeapYear(internalGet(UCAL_YEAR)) ? 366 : 365;
}









void 
GregorianCalendar::pinDayOfMonth() 
{
    int32_t monthLen = monthLength(internalGet(UCAL_MONTH));
    int32_t dom = internalGet(UCAL_DATE);
    if(dom > monthLen) 
        set(UCAL_DATE, monthLen);
}




UBool
GregorianCalendar::validateFields() const
{
    for (int32_t field = 0; field < UCAL_FIELD_COUNT; field++) {
        
        if (field != UCAL_DATE &&
            field != UCAL_DAY_OF_YEAR &&
            isSet((UCalendarDateFields)field) &&
            ! boundsCheck(internalGet((UCalendarDateFields)field), (UCalendarDateFields)field))
            return FALSE;
    }

    
    
    if (isSet(UCAL_DATE)) {
        int32_t date = internalGet(UCAL_DATE);
        if (date < getMinimum(UCAL_DATE) ||
            date > monthLength(internalGet(UCAL_MONTH))) {
                return FALSE;
            }
    }

    if (isSet(UCAL_DAY_OF_YEAR)) {
        int32_t days = internalGet(UCAL_DAY_OF_YEAR);
        if (days < 1 || days > yearLength()) {
            return FALSE;
        }
    }

    
    
    if (isSet(UCAL_DAY_OF_WEEK_IN_MONTH) &&
        0 == internalGet(UCAL_DAY_OF_WEEK_IN_MONTH)) {
            return FALSE;
        }

        return TRUE;
}



UBool
GregorianCalendar::boundsCheck(int32_t value, UCalendarDateFields field) const
{
    return value >= getMinimum(field) && value <= getMaximum(field);
}



UDate 
GregorianCalendar::getEpochDay(UErrorCode& status) 
{
    complete(status);
    
    
    double wallSec = internalGetTime()/1000 + (internalGet(UCAL_ZONE_OFFSET) + internalGet(UCAL_DST_OFFSET))/1000;

    return ClockMath::floorDivide(wallSec, kOneDay/1000.0);
}












double GregorianCalendar::computeJulianDayOfYear(UBool isGregorian,
                                                 int32_t year, UBool& isLeap)
{
    isLeap = year%4 == 0;
    int32_t y = year - 1;
    double julianDay = 365.0*y + ClockMath::floorDivide(y, 4) + (kJan1_1JulianDay - 3);

    if (isGregorian) {
        isLeap = isLeap && ((year%100 != 0) || (year%400 == 0));
        
        julianDay += Grego::gregorianShift(year);
    }

    return julianDay;
}
























































double 
GregorianCalendar::millisToJulianDay(UDate millis)
{
    return (double)kEpochStartAsJulianDay + ClockMath::floorDivide(millis, (double)kOneDay);
}



UDate
GregorianCalendar::julianDayToMillis(double julian)
{
    return (UDate) ((julian - kEpochStartAsJulianDay) * (double) kOneDay);
}



int32_t
GregorianCalendar::aggregateStamp(int32_t stamp_a, int32_t stamp_b) 
{
    return (((stamp_a != kUnset && stamp_b != kUnset) 
        ? uprv_max(stamp_a, stamp_b)
        : (int32_t)kUnset));
}








void 
GregorianCalendar::roll(EDateFields field, int32_t amount, UErrorCode& status) {
    roll((UCalendarDateFields) field, amount, status); 
}

void
GregorianCalendar::roll(UCalendarDateFields field, int32_t amount, UErrorCode& status)
{
    if((amount == 0) || U_FAILURE(status)) {
        return;
    }

    
    UBool inCutoverMonth = FALSE;
    int32_t cMonthLen=0; 
    int32_t cDayOfMonth=0; 
    double cMonthStart=0.0; 

    
    if(get(UCAL_EXTENDED_YEAR, status) == fGregorianCutoverYear) {
        switch (field) {
        case UCAL_DAY_OF_MONTH:
        case UCAL_WEEK_OF_MONTH:
            {
                int32_t max = monthLength(internalGet(UCAL_MONTH));
                UDate t = internalGetTime();
                
                
                
                cDayOfMonth = internalGet(UCAL_DAY_OF_MONTH) - ((t >= fGregorianCutover) ? 10 : 0);
                cMonthStart = t - ((cDayOfMonth - 1) * kOneDay);
                
                if ((cMonthStart < fGregorianCutover) &&
                    (cMonthStart + (cMonthLen=(max-10))*kOneDay >= fGregorianCutover)) {
                        inCutoverMonth = TRUE;
                    }
            }
        default:
            ;
        }
    }

    switch (field) {
    case UCAL_WEEK_OF_YEAR: {
        
        
        
        
        
        
        
        int32_t woy = get(UCAL_WEEK_OF_YEAR, status);
        
        
        int32_t isoYear = get(UCAL_YEAR_WOY, status);
        int32_t isoDoy = internalGet(UCAL_DAY_OF_YEAR);
        if (internalGet(UCAL_MONTH) == UCAL_JANUARY) {
            if (woy >= 52) {
                isoDoy += handleGetYearLength(isoYear);
            }
        } else {
            if (woy == 1) {
                isoDoy -= handleGetYearLength(isoYear - 1);
            }
        }
        woy += amount;
        
        if (woy < 1 || woy > 52) {
            
            
            
            
            
            
            int32_t lastDoy = handleGetYearLength(isoYear);
            int32_t lastRelDow = (lastDoy - isoDoy + internalGet(UCAL_DAY_OF_WEEK) -
                getFirstDayOfWeek()) % 7;
            if (lastRelDow < 0) lastRelDow += 7;
            if ((6 - lastRelDow) >= getMinimalDaysInFirstWeek()) lastDoy -= 7;
            int32_t lastWoy = weekNumber(lastDoy, lastRelDow + 1);
            woy = ((woy + lastWoy - 1) % lastWoy) + 1;
        }
        set(UCAL_WEEK_OF_YEAR, woy);
        set(UCAL_YEAR_WOY,isoYear);
        return;
                            }

    case UCAL_DAY_OF_MONTH:
        if( !inCutoverMonth ) { 
            Calendar::roll(field, amount, status);
            return;
        } else {
            
            
            
            
            double monthLen = cMonthLen * kOneDay;
            double msIntoMonth = uprv_fmod(internalGetTime() - cMonthStart +
                amount * kOneDay, monthLen);
            if (msIntoMonth < 0) {
                msIntoMonth += monthLen;
            }
#if defined (U_DEBUG_CAL)
            fprintf(stderr, "%s:%d: roll DOM %d  -> %.0lf ms  \n", 
                __FILE__, __LINE__,amount, cMonthLen, cMonthStart+msIntoMonth);
#endif
            setTimeInMillis(cMonthStart + msIntoMonth, status);
            return;
        }

    case UCAL_WEEK_OF_MONTH:
        if( !inCutoverMonth ) { 
            Calendar::roll(field, amount, status);
            return;
        } else {
#if defined (U_DEBUG_CAL)
            fprintf(stderr, "%s:%d: roll WOM %d ??????????????????? \n", 
                __FILE__, __LINE__,amount);
#endif
            
            

            
            

            
            
            

            
            
            

            
            
            
            
            

            
            
            
            
            
            
            
            
            

            
            
            
            
            

            
            
            int32_t dow = internalGet(UCAL_DAY_OF_WEEK) - getFirstDayOfWeek();
            if (dow < 0) 
                dow += 7;

            
            int32_t dom = cDayOfMonth;

            
            
            int32_t fdm = (dow - dom + 1) % 7;
            if (fdm < 0) 
                fdm += 7;

            
            
            
            
            int32_t start;
            if ((7 - fdm) < getMinimalDaysInFirstWeek())
                start = 8 - fdm; 
            else
                start = 1 - fdm; 

            
            
            int32_t monthLen = cMonthLen;
            int32_t ldm = (monthLen - dom + dow) % 7;
            

            
            
            
            
            int32_t limit = monthLen + 7 - ldm;

            
            int32_t gap = limit - start;
            int32_t newDom = (dom + amount*7 - start) % gap;
            if (newDom < 0) 
                newDom += gap;
            newDom += start;

            
            if (newDom < 1) 
                newDom = 1;
            if (newDom > monthLen) 
                newDom = monthLen;

            
            
            
            
            
            

            
            
            
            setTimeInMillis(cMonthStart + (newDom-1)*kOneDay, status);                
            return;
        }

    default:
        Calendar::roll(field, amount, status);
        return;
    }
}











int32_t GregorianCalendar::getActualMinimum(EDateFields field) const
{
    return getMinimum((UCalendarDateFields)field);
}

int32_t GregorianCalendar::getActualMinimum(EDateFields field, UErrorCode& ) const
{
    return getMinimum((UCalendarDateFields)field);
}








int32_t GregorianCalendar::getActualMinimum(UCalendarDateFields field, UErrorCode& ) const
{
    return getMinimum(field);
}











int32_t GregorianCalendar::handleGetLimit(UCalendarDateFields field, ELimitType limitType) const {
    return kGregorianCalendarLimits[field][limitType];
}








int32_t GregorianCalendar::getActualMaximum(UCalendarDateFields field, UErrorCode& status) const
{
    


















    switch (field) {

    case UCAL_YEAR:
        


















        {
            if(U_FAILURE(status)) return 0;
            Calendar *cal = clone();
            if(!cal) {
                status = U_MEMORY_ALLOCATION_ERROR;
                return 0;
            }

            cal->setLenient(TRUE);

            int32_t era = cal->get(UCAL_ERA, status);
            UDate d = cal->getTime(status);

            


            int32_t lowGood = kGregorianCalendarLimits[UCAL_YEAR][1];
            int32_t highBad = kGregorianCalendarLimits[UCAL_YEAR][2]+1;
            while ((lowGood + 1) < highBad) {
                int32_t y = (lowGood + highBad) / 2;
                cal->set(UCAL_YEAR, y);
                if (cal->get(UCAL_YEAR, status) == y && cal->get(UCAL_ERA, status) == era) {
                    lowGood = y;
                } else {
                    highBad = y;
                    cal->setTime(d, status); 
                }
            }

            delete cal;
            return lowGood;
        }

    default:
        return Calendar::getActualMaximum(field,status);
    }
}


int32_t GregorianCalendar::handleGetExtendedYear() {
    
    int32_t year = kEpochYear;

    
    int32_t yearField = UCAL_EXTENDED_YEAR;

    
    
    if (fStamp[yearField] < fStamp[UCAL_YEAR])
        yearField = UCAL_YEAR;
    if (fStamp[yearField] < fStamp[UCAL_YEAR_WOY])
        yearField = UCAL_YEAR_WOY;

    
    switch(yearField) {
    case UCAL_EXTENDED_YEAR:
        year = internalGet(UCAL_EXTENDED_YEAR, kEpochYear);
        break;

    case UCAL_YEAR:
        {
            
            int32_t era = internalGet(UCAL_ERA, AD);
            if (era == BC) {
                year = 1 - internalGet(UCAL_YEAR, 1); 
            } else {
                year = internalGet(UCAL_YEAR, kEpochYear);
            }
        }
        break;

    case UCAL_YEAR_WOY:
        year = handleGetExtendedYearFromWeekFields(internalGet(UCAL_YEAR_WOY), internalGet(UCAL_WEEK_OF_YEAR));
#if defined (U_DEBUG_CAL)
        
        fprintf(stderr, "%s:%d: hGEYFWF[%d,%d] ->  %d\n", 
            __FILE__, __LINE__,internalGet(UCAL_YEAR_WOY),internalGet(UCAL_WEEK_OF_YEAR),year);
        
#endif
        break;

    default:
        year = kEpochYear;
    }
    return year;
}

int32_t GregorianCalendar::handleGetExtendedYearFromWeekFields(int32_t yearWoy, int32_t woy)
{
    
    int32_t era = internalGet(UCAL_ERA, AD);
    if(era == BC) {
        yearWoy = 1 - yearWoy;
    }
    return Calendar::handleGetExtendedYearFromWeekFields(yearWoy, woy);
}




UBool
GregorianCalendar::inDaylightTime(UErrorCode& status) const
{
    if (U_FAILURE(status) || !getTimeZone().useDaylightTime()) 
        return FALSE;

    
    ((GregorianCalendar*)this)->complete(status); 

    return (UBool)(U_SUCCESS(status) ? (internalGet(UCAL_DST_OFFSET) != 0) : FALSE);
}







int32_t
GregorianCalendar::internalGetEra() const {
    return isSet(UCAL_ERA) ? internalGet(UCAL_ERA) : (int32_t)AD;
}

const char *
GregorianCalendar::getType() const {
    

    return "gregorian";
}

const UDate     GregorianCalendar::fgSystemDefaultCentury        = DBL_MIN;
const int32_t   GregorianCalendar::fgSystemDefaultCenturyYear    = -1;

UDate           GregorianCalendar::fgSystemDefaultCenturyStart       = DBL_MIN;
int32_t         GregorianCalendar::fgSystemDefaultCenturyStartYear   = -1;


UBool GregorianCalendar::haveDefaultCentury() const
{
    return TRUE;
}

UDate GregorianCalendar::defaultCenturyStart() const
{
    return internalGetDefaultCenturyStart();
}

int32_t GregorianCalendar::defaultCenturyStartYear() const
{
    return internalGetDefaultCenturyStartYear();
}

UDate
GregorianCalendar::internalGetDefaultCenturyStart() const
{
    
    UBool needsUpdate;
    UMTX_CHECK(NULL, (fgSystemDefaultCenturyStart == fgSystemDefaultCentury), needsUpdate);

    if (needsUpdate) {
        initializeSystemDefaultCentury();
    }

    
    

    return fgSystemDefaultCenturyStart;
}

int32_t
GregorianCalendar::internalGetDefaultCenturyStartYear() const
{
    
    UBool needsUpdate;
    UMTX_CHECK(NULL, (fgSystemDefaultCenturyStart == fgSystemDefaultCentury), needsUpdate);

    if (needsUpdate) {
        initializeSystemDefaultCentury();
    }

    
    

    return fgSystemDefaultCenturyStartYear;
}

void
GregorianCalendar::initializeSystemDefaultCentury()
{
    
    
    
    UErrorCode status = U_ZERO_ERROR;
    Calendar *calendar = new GregorianCalendar(status);
    if (calendar != NULL && U_SUCCESS(status))
    {
        calendar->setTime(Calendar::getNow(), status);
        calendar->add(UCAL_YEAR, -80, status);

        UDate    newStart =  calendar->getTime(status);
        int32_t  newYear  =  calendar->get(UCAL_YEAR, status);
        umtx_lock(NULL);
        if (fgSystemDefaultCenturyStart == fgSystemDefaultCentury)
        {
            fgSystemDefaultCenturyStartYear = newYear;
            fgSystemDefaultCenturyStart = newStart;
        }
        umtx_unlock(NULL);
        delete calendar;
    }
    
    
}


U_NAMESPACE_END

#endif 


