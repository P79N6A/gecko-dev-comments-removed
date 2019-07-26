
















#include "persncal.h"

#if !UCONFIG_NO_FORMATTING

#include "umutex.h"
#include "gregoimp.h" 
#include <float.h>

static const int16_t kPersianNumDays[]
= {0,31,62,93,124,155,186,216,246,276,306,336}; 
static const int8_t kPersianMonthLength[]
= {31,31,31,31,31,31,30,30,30,30,30,29}; 
static const int8_t kPersianLeapMonthLength[]
= {31,31,31,31,31,31,30,30,30,30,30,30}; 

static const int32_t kPersianCalendarLimits[UCAL_FIELD_COUNT][4] = {
    
    
    {        0,        0,        0,        0}, 
    { -5000000, -5000000,  5000000,  5000000}, 
    {        0,        0,       11,       11}, 
    {        1,        1,       52,       53}, 
    {-1,-1,-1,-1}, 
    {        1,       1,        29,       31}, 
    {        1,       1,       365,      366}, 
    {-1,-1,-1,-1}, 
    {        1,       1,         5,        5}, 
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

U_NAMESPACE_BEGIN

static const int32_t PERSIAN_EPOCH = 1948320;







const char *PersianCalendar::getType() const { 
    return "persian";
}

Calendar* PersianCalendar::clone() const {
    return new PersianCalendar(*this);
}

PersianCalendar::PersianCalendar(const Locale& aLocale, UErrorCode& success)
  :   Calendar(TimeZone::createDefault(), aLocale, success)
{
    setTimeInMillis(getNow(), success); 
}

PersianCalendar::PersianCalendar(const PersianCalendar& other) : Calendar(other) {
}

PersianCalendar::~PersianCalendar()
{
}






int32_t PersianCalendar::handleGetLimit(UCalendarDateFields field, ELimitType limitType) const {
    return kPersianCalendarLimits[field][limitType];
}








UBool PersianCalendar::isLeapYear(int32_t year)
{
    int32_t remainder;
    ClockMath::floorDivide(25 * year + 11, 33, remainder);
    return (remainder < 8);
}
    




int32_t PersianCalendar::yearStart(int32_t year) {
    return handleComputeMonthStart(year,0,FALSE);
}
    







int32_t PersianCalendar::monthStart(int32_t year, int32_t month) const {
    return handleComputeMonthStart(year,month,TRUE);
}
    










int32_t PersianCalendar::handleGetMonthLength(int32_t extendedYear, int32_t month) const {
    
    
    if (month < 0 || month > 11) {
        extendedYear += ClockMath::floorDivide(month, 12, month);
    }

    return isLeapYear(extendedYear) ? kPersianLeapMonthLength[month] : kPersianMonthLength[month];
}




int32_t PersianCalendar::handleGetYearLength(int32_t extendedYear) const {
    return isLeapYear(extendedYear) ? 366 : 365;
}
    





int32_t PersianCalendar::handleComputeMonthStart(int32_t eyear, int32_t month, UBool ) const {
    
    
    if (month < 0 || month > 11) {
        eyear += ClockMath::floorDivide(month, 12, month);
    }

    int32_t julianDay = PERSIAN_EPOCH - 1 + 365 * (eyear - 1) + ClockMath::floorDivide(8 * eyear + 21, 33);

    if (month != 0) {
        julianDay += kPersianNumDays[month];
    }

    return julianDay;
}





int32_t PersianCalendar::handleGetExtendedYear() {
    int32_t year;
    if (newerField(UCAL_EXTENDED_YEAR, UCAL_YEAR) == UCAL_EXTENDED_YEAR) {
        year = internalGet(UCAL_EXTENDED_YEAR, 1); 
    } else {
        year = internalGet(UCAL_YEAR, 1); 
    }
    return year;
}















void PersianCalendar::handleComputeFields(int32_t julianDay, UErrorCode &) {
    int32_t year, month, dayOfMonth, dayOfYear;

    int32_t daysSinceEpoch = julianDay - PERSIAN_EPOCH;
    year = 1 + ClockMath::floorDivide(33 * daysSinceEpoch + 3, 12053);

    int32_t farvardin1 = 365 * (year - 1) + ClockMath::floorDivide(8 * year + 21, 33);
    dayOfYear = (daysSinceEpoch - farvardin1); 
    if (dayOfYear < 216) { 
        month = dayOfYear / 31;
    } else {
        month = (dayOfYear - 6) / 30;
    }
    dayOfMonth = dayOfYear - kPersianNumDays[month] + 1;
    ++dayOfYear; 

    internalSet(UCAL_ERA, 0);
    internalSet(UCAL_YEAR, year);
    internalSet(UCAL_EXTENDED_YEAR, year);
    internalSet(UCAL_MONTH, month);
    internalSet(UCAL_DAY_OF_MONTH, dayOfMonth);
    internalSet(UCAL_DAY_OF_YEAR, dayOfYear);
}    

UBool
PersianCalendar::inDaylightTime(UErrorCode& status) const
{
    
    if (U_FAILURE(status) || !getTimeZone().useDaylightTime()) 
        return FALSE;

    
    ((PersianCalendar*)this)->complete(status); 

    return (UBool)(U_SUCCESS(status) ? (internalGet(UCAL_DST_OFFSET) != 0) : FALSE);
}


const UDate     PersianCalendar::fgSystemDefaultCentury        = DBL_MIN;
const int32_t   PersianCalendar::fgSystemDefaultCenturyYear    = -1;

UDate           PersianCalendar::fgSystemDefaultCenturyStart       = DBL_MIN;
int32_t         PersianCalendar::fgSystemDefaultCenturyStartYear   = -1;

UBool PersianCalendar::haveDefaultCentury() const
{
    return TRUE;
}

UDate PersianCalendar::defaultCenturyStart() const
{
    return internalGetDefaultCenturyStart();
}

int32_t PersianCalendar::defaultCenturyStartYear() const
{
    return internalGetDefaultCenturyStartYear();
}

UDate
PersianCalendar::internalGetDefaultCenturyStart() const
{
    
    UBool needsUpdate;
    UMTX_CHECK(NULL, (fgSystemDefaultCenturyStart == fgSystemDefaultCentury), needsUpdate);

    if (needsUpdate) {
        initializeSystemDefaultCentury();
    }

    
    

    return fgSystemDefaultCenturyStart;
}

int32_t
PersianCalendar::internalGetDefaultCenturyStartYear() const
{
    
    UBool needsUpdate;
    UMTX_CHECK(NULL, (fgSystemDefaultCenturyStart == fgSystemDefaultCentury), needsUpdate);

    if (needsUpdate) {
        initializeSystemDefaultCentury();
    }

    
    

    return    fgSystemDefaultCenturyStartYear;
}

void
PersianCalendar::initializeSystemDefaultCentury()
{
    
    
    
    UErrorCode status = U_ZERO_ERROR;
    PersianCalendar calendar(Locale("@calendar=persian"),status);
    if (U_SUCCESS(status))
    {
        calendar.setTime(Calendar::getNow(), status);
        calendar.add(UCAL_YEAR, -80, status);
        UDate    newStart =  calendar.getTime(status);
        int32_t  newYear  =  calendar.get(UCAL_YEAR, status);
        umtx_lock(NULL);
        if (fgSystemDefaultCenturyStart == fgSystemDefaultCentury)
        {
            fgSystemDefaultCenturyStartYear = newYear;
            fgSystemDefaultCenturyStart = newStart;
        }
        umtx_unlock(NULL);
    }
    
    
}

UOBJECT_DEFINE_RTTI_IMPLEMENTATION(PersianCalendar)

U_NAMESPACE_END

#endif

