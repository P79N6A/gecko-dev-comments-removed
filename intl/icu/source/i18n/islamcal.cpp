














#include "islamcal.h"

#if !UCONFIG_NO_FORMATTING

#include "umutex.h"
#include <float.h>
#include "gregoimp.h" 
#include "astro.h" 
#include "uhash.h"
#include "ucln_in.h"

static const UDate HIJRA_MILLIS = -42521587200000.0;    


#ifdef U_DEBUG_ISLAMCAL
# include <stdio.h>
# include <stdarg.h>
static void debug_islamcal_loc(const char *f, int32_t l)
{
    fprintf(stderr, "%s:%d: ", f, l);
}

static void debug_islamcal_msg(const char *pat, ...)
{
    va_list ap;
    va_start(ap, pat);
    vfprintf(stderr, pat, ap);
    fflush(stderr);
}

#define U_DEBUG_ISLAMCAL_MSG(x) {debug_islamcal_loc(__FILE__,__LINE__);debug_islamcal_msg x;}
#else
#define U_DEBUG_ISLAMCAL_MSG(x)
#endif




static UMutex astroLock = U_MUTEX_INITIALIZER;  
static icu::CalendarCache *gMonthCache = NULL;
static icu::CalendarAstronomer *gIslamicCalendarAstro = NULL;

U_CDECL_BEGIN
static UBool calendar_islamic_cleanup(void) {
    if (gMonthCache) {
        delete gMonthCache;
        gMonthCache = NULL;
    }
    if (gIslamicCalendarAstro) {
        delete gIslamicCalendarAstro;
        gIslamicCalendarAstro = NULL;
    }
    return TRUE;
}
U_CDECL_END

U_NAMESPACE_BEGIN







const char *IslamicCalendar::getType() const { 
    if(civil==CIVIL) {
        return "islamic-civil";
    } else {
        return "islamic";
    }
}

Calendar* IslamicCalendar::clone() const {
    return new IslamicCalendar(*this);
}

IslamicCalendar::IslamicCalendar(const Locale& aLocale, UErrorCode& success, ECivil beCivil)
:   Calendar(TimeZone::createDefault(), aLocale, success),
civil(beCivil)
{
    setTimeInMillis(getNow(), success); 
}

IslamicCalendar::IslamicCalendar(const IslamicCalendar& other) : Calendar(other), civil(other.civil) {
}

IslamicCalendar::~IslamicCalendar()
{
}









void IslamicCalendar::setCivil(ECivil beCivil, UErrorCode &status)
{
    if (civil != beCivil) {
        
        
        UDate m = getTimeInMillis(status);
        civil = beCivil;
        clear();
        setTimeInMillis(m, status);
    }
}







UBool IslamicCalendar::isCivil() {
    return (civil == CIVIL);
}












static const int32_t LIMITS[UCAL_FIELD_COUNT][4] = {
    
    
    {        0,        0,        0,        0}, 
    {        1,        1,  5000000,  5000000}, 
    {        0,        0,       11,       11}, 
    {        1,        1,       50,       51}, 
    {-1,-1,-1,-1}, 
    {        1,        1,       29,       31}, 
    {        1,        1,      354,      355}, 
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
    {        1,        1,  5000000,  5000000}, 
    {-1,-1,-1,-1}, 
    {        1,        1,  5000000,  5000000}, 
    {-1,-1,-1,-1}, 
    {-1,-1,-1,-1}, 
    {-1,-1,-1,-1}, 
};




int32_t IslamicCalendar::handleGetLimit(UCalendarDateFields field, ELimitType limitType) const {
    return LIMITS[field][limitType];
}








UBool IslamicCalendar::civilLeapYear(int32_t year)
{
    return (14 + 11 * year) % 30 < 11;
}





int32_t IslamicCalendar::yearStart(int32_t year) {
    if (civil == CIVIL) {
        return (year-1)*354 + ClockMath::floorDivide((3+11*year),30);
    } else {
        return trueMonthStart(12*(year-1));
    }
}








int32_t IslamicCalendar::monthStart(int32_t year, int32_t month) const {
    if (civil == CIVIL) {
        return (int32_t)uprv_ceil(29.5*month)
            + (year-1)*354 + (int32_t)ClockMath::floorDivide((3+11*year),30);
    } else {
        return trueMonthStart(12*(year-1) + month);
    }
}









int32_t IslamicCalendar::trueMonthStart(int32_t month) const
{
    UErrorCode status = U_ZERO_ERROR;
    int32_t start = CalendarCache::get(&gMonthCache, month, status);

    if (start==0) {
        
        UDate origin = HIJRA_MILLIS 
            + uprv_floor(month * CalendarAstronomer::SYNODIC_MONTH) * kOneDay;

        
        double age = moonAge(origin, status);
        if (U_FAILURE(status)) {
            goto trueMonthStartEnd;
        }

        if (age >= 0) {
            
            do {
                origin -= kOneDay;
                age = moonAge(origin, status);
                if (U_FAILURE(status)) {
                    goto trueMonthStartEnd;
                }
            } while (age >= 0);
        }
        else {
            
            do {
                origin += kOneDay;
                age = moonAge(origin, status);
                if (U_FAILURE(status)) {
                    goto trueMonthStartEnd;
                }
            } while (age < 0);
        }
        start = (int32_t)ClockMath::floorDivide((origin - HIJRA_MILLIS), (double)kOneDay) + 1;
        CalendarCache::put(&gMonthCache, month, start, status);
    }
trueMonthStartEnd :
    if(U_FAILURE(status)) {
        start = 0;
    }
    return start;
}










double IslamicCalendar::moonAge(UDate time, UErrorCode &status)
{
    double age = 0;

    umtx_lock(&astroLock);
    if(gIslamicCalendarAstro == NULL) {
        gIslamicCalendarAstro = new CalendarAstronomer();
        if (gIslamicCalendarAstro == NULL) {
            status = U_MEMORY_ALLOCATION_ERROR;
            return age;
        }
        ucln_i18n_registerCleanup(UCLN_I18N_ISLAMIC_CALENDAR, calendar_islamic_cleanup);
    }
    gIslamicCalendarAstro->setTime(time);
    age = gIslamicCalendarAstro->getMoonAge();
    umtx_unlock(&astroLock);

    
    age = age * 180 / CalendarAstronomer::PI;
    if (age > 180) {
        age = age - 360;
    }

    return age;
}












int32_t IslamicCalendar::handleGetMonthLength(int32_t extendedYear, int32_t month) const {

    int32_t length = 0;

    if (civil == CIVIL) {
        length = 29 + (month+1) % 2;
        if (month == DHU_AL_HIJJAH && civilLeapYear(extendedYear)) {
            length++;
        }
    } else {
        month = 12*(extendedYear-1) + month;
        length =  trueMonthStart(month+1) - trueMonthStart(month) ;
    }
    return length;
}





int32_t IslamicCalendar::handleGetYearLength(int32_t extendedYear) const {
    if (civil == CIVIL) {
        return 354 + (civilLeapYear(extendedYear) ? 1 : 0);
    } else {
        int32_t month = 12*(extendedYear-1);
        return (trueMonthStart(month + 12) - trueMonthStart(month));
    }
}









int32_t IslamicCalendar::handleComputeMonthStart(int32_t eyear, int32_t month, UBool ) const {
    return monthStart(eyear, month) + 1948439;
}    








int32_t IslamicCalendar::handleGetExtendedYear() {
    int32_t year;
    if (newerField(UCAL_EXTENDED_YEAR, UCAL_YEAR) == UCAL_EXTENDED_YEAR) {
        year = internalGet(UCAL_EXTENDED_YEAR, 1); 
    } else {
        year = internalGet(UCAL_YEAR, 1); 
    }
    return year;
}

















void IslamicCalendar::handleComputeFields(int32_t julianDay, UErrorCode &status) {
    int32_t year, month, dayOfMonth, dayOfYear;
    UDate startDate;
    int32_t days = julianDay - 1948440;

    if (civil == CIVIL) {
        
        year  = (int)ClockMath::floorDivide( (double)(30 * days + 10646) , 10631.0 );
        month = (int32_t)uprv_ceil((days - 29 - yearStart(year)) / 29.5 );
        month = month<11?month:11;
        startDate = monthStart(year, month);
    } else {
        
        int32_t months = (int32_t)uprv_floor((double)days / CalendarAstronomer::SYNODIC_MONTH);

        startDate = uprv_floor(months * CalendarAstronomer::SYNODIC_MONTH);

        double age = moonAge(internalGetTime(), status);
        if (U_FAILURE(status)) {
        	status = U_MEMORY_ALLOCATION_ERROR;
        	return;
        }
        if ( days - startDate >= 25 && age > 0) {
            
            months++;
        }

        
        
        while ((startDate = trueMonthStart(months)) > days) {
            
            months--;
        }

        year = months / 12 + 1;
        month = months % 12;
    }

    dayOfMonth = (days - monthStart(year, month)) + 1;

    
    dayOfYear = (days - monthStart(year, 0) + 1);

    internalSet(UCAL_ERA, 0);
    internalSet(UCAL_YEAR, year);
    internalSet(UCAL_EXTENDED_YEAR, year);
    internalSet(UCAL_MONTH, month);
    internalSet(UCAL_DAY_OF_MONTH, dayOfMonth);
    internalSet(UCAL_DAY_OF_YEAR, dayOfYear);       
}    

UBool
IslamicCalendar::inDaylightTime(UErrorCode& status) const
{
    
    if (U_FAILURE(status) || (&(getTimeZone()) == NULL && !getTimeZone().useDaylightTime())) 
        return FALSE;

    
    ((IslamicCalendar*)this)->complete(status); 

    return (UBool)(U_SUCCESS(status) ? (internalGet(UCAL_DST_OFFSET) != 0) : FALSE);
}


const UDate     IslamicCalendar::fgSystemDefaultCentury        = DBL_MIN;
const int32_t   IslamicCalendar::fgSystemDefaultCenturyYear    = -1;

UDate           IslamicCalendar::fgSystemDefaultCenturyStart       = DBL_MIN;
int32_t         IslamicCalendar::fgSystemDefaultCenturyStartYear   = -1;


UBool IslamicCalendar::haveDefaultCentury() const
{
    return TRUE;
}

UDate IslamicCalendar::defaultCenturyStart() const
{
    return internalGetDefaultCenturyStart();
}

int32_t IslamicCalendar::defaultCenturyStartYear() const
{
    return internalGetDefaultCenturyStartYear();
}

UDate
IslamicCalendar::internalGetDefaultCenturyStart() const
{
    
    UBool needsUpdate;
    UMTX_CHECK(NULL, (fgSystemDefaultCenturyStart == fgSystemDefaultCentury), needsUpdate);

    if (needsUpdate) {
        initializeSystemDefaultCentury();
    }

    
    

    return fgSystemDefaultCenturyStart;
}

int32_t
IslamicCalendar::internalGetDefaultCenturyStartYear() const
{
    
    UBool needsUpdate;
    UMTX_CHECK(NULL, (fgSystemDefaultCenturyStart == fgSystemDefaultCentury), needsUpdate);

    if (needsUpdate) {
        initializeSystemDefaultCentury();
    }

    
    

    return    fgSystemDefaultCenturyStartYear;
}

void
IslamicCalendar::initializeSystemDefaultCentury()
{
    
    
    
    UErrorCode status = U_ZERO_ERROR;
    IslamicCalendar calendar(Locale("@calendar=islamic-civil"),status);
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

UOBJECT_DEFINE_RTTI_IMPLEMENTATION(IslamicCalendar)

U_NAMESPACE_END

#endif

