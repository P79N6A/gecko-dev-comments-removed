






#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "cecal.h"
#include "gregoimp.h"   

U_NAMESPACE_BEGIN

static const int32_t LIMITS[UCAL_FIELD_COUNT][4] = {
    
    
    {        0,        0,        1,        1}, 
    {        1,        1,  5000000,  5000000}, 
    {        0,        0,       12,       12}, 
    {        1,        1,       52,       53}, 
    {-1,-1,-1,-1}, 
    {        1,        1,        5,       30}, 
    {        1,        1,      365,      366}, 
    {-1,-1,-1,-1}, 
    {       -1,       -1,        1,        5}, 
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





CECalendar::CECalendar(const Locale& aLocale, UErrorCode& success)
:   Calendar(TimeZone::createDefault(), aLocale, success)
{
    setTimeInMillis(getNow(), success);
}

CECalendar::CECalendar (const CECalendar& other) 
:   Calendar(other)
{
}

CECalendar::~CECalendar()
{
}

CECalendar&
CECalendar::operator=(const CECalendar& right)
{
    Calendar::operator=(right);
    return *this;
}





int32_t
CECalendar::handleComputeMonthStart(int32_t eyear,int32_t emonth, UBool ) const
{
    return ceToJD(eyear, emonth, 0, getJDEpochOffset());
}

int32_t
CECalendar::handleGetLimit(UCalendarDateFields field, ELimitType limitType) const
{
    return LIMITS[field][limitType];
}

UBool
CECalendar::inDaylightTime(UErrorCode& status) const
{
    if (U_FAILURE(status) || !getTimeZone().useDaylightTime()) {
        return FALSE;
    }

    
    ((CECalendar*)this)->complete(status); 

    return (UBool)(U_SUCCESS(status) ? (internalGet(UCAL_DST_OFFSET) != 0) : FALSE);
}

UBool
CECalendar::haveDefaultCentury() const
{
    return TRUE;
}




int32_t
CECalendar::ceToJD(int32_t year, int32_t month, int32_t date, int32_t jdEpochOffset)
{
    
    if ( month >= 0 ) {
        year += month/13;
        month %= 13;
    } else {
        ++month;
        year += month/13 - 1;
        month = month%13 + 12;
    }
    return (int32_t) (
        jdEpochOffset                   
        + 365 * year                    
        + ClockMath::floorDivide(year, 4)    
        + 30 * month                    
        + date - 1                      
        );
}

void
CECalendar::jdToCE(int32_t julianDay, int32_t jdEpochOffset, int32_t& year, int32_t& month, int32_t& day)
{
    int32_t c4; 
    int32_t r4; 

    c4 = ClockMath::floorDivide(julianDay - jdEpochOffset, 1461, r4);

    year = 4 * c4 + (r4/365 - r4/1460); 

    int32_t doy = (r4 == 1460) ? 365 : (r4 % 365); 

    month = doy / 30;       
    day = (doy % 30) + 1;   
}

U_NAMESPACE_END

#endif 

