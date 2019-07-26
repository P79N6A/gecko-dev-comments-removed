














#include "chnsecal.h"

#if !UCONFIG_NO_FORMATTING

#include "umutex.h"
#include <float.h>
#include "gregoimp.h" 
#include "astro.h" 
#include "unicode/simpletz.h"
#include "uhash.h"
#include "ucln_in.h"


#ifdef U_DEBUG_CHNSECAL
# include <stdio.h>
# include <stdarg.h>
static void debug_chnsecal_loc(const char *f, int32_t l)
{
    fprintf(stderr, "%s:%d: ", f, l);
}

static void debug_chnsecal_msg(const char *pat, ...)
{
    va_list ap;
    va_start(ap, pat);
    vfprintf(stderr, pat, ap);
    fflush(stderr);
}

#define U_DEBUG_CHNSECAL_MSG(x) {debug_chnsecal_loc(__FILE__,__LINE__);debug_chnsecal_msg x;}
#else
#define U_DEBUG_CHNSECAL_MSG(x)
#endif



static UMutex astroLock = U_MUTEX_INITIALIZER;  
static icu::CalendarAstronomer *gChineseCalendarAstro = NULL;
static icu::CalendarCache *gChineseCalendarWinterSolsticeCache = NULL;
static icu::CalendarCache *gChineseCalendarNewYearCache = NULL;
static icu::TimeZone *gChineseCalendarZoneAstroCalc = NULL;
static icu::UInitOnce gChineseCalendarZoneAstroCalcInitOnce = U_INITONCE_INITIALIZER;







static const int32_t CHINESE_EPOCH_YEAR = -2636; 






static const int32_t CHINA_OFFSET = 8 * kOneHour;






static const int32_t SYNODIC_GAP = 25;


U_CDECL_BEGIN
static UBool calendar_chinese_cleanup(void) {
    if (gChineseCalendarAstro) {
        delete gChineseCalendarAstro;
        gChineseCalendarAstro = NULL;
    }
    if (gChineseCalendarWinterSolsticeCache) {
        delete gChineseCalendarWinterSolsticeCache;
        gChineseCalendarWinterSolsticeCache = NULL;
    }
    if (gChineseCalendarNewYearCache) {
        delete gChineseCalendarNewYearCache;
        gChineseCalendarNewYearCache = NULL;
    }
    if (gChineseCalendarZoneAstroCalc) {
        delete gChineseCalendarZoneAstroCalc;
        gChineseCalendarZoneAstroCalc = NULL;
    }
    gChineseCalendarZoneAstroCalcInitOnce.reset();
    return TRUE;
}
U_CDECL_END

U_NAMESPACE_BEGIN










Calendar* ChineseCalendar::clone() const {
    return new ChineseCalendar(*this);
}

ChineseCalendar::ChineseCalendar(const Locale& aLocale, UErrorCode& success)
:   Calendar(TimeZone::createDefault(), aLocale, success),
    isLeapYear(FALSE),
    fEpochYear(CHINESE_EPOCH_YEAR),
    fZoneAstroCalc(getChineseCalZoneAstroCalc())
{
    setTimeInMillis(getNow(), success); 
}

ChineseCalendar::ChineseCalendar(const Locale& aLocale, int32_t epochYear,
                                const TimeZone* zoneAstroCalc, UErrorCode &success)
:   Calendar(TimeZone::createDefault(), aLocale, success),
    isLeapYear(FALSE),
    fEpochYear(epochYear),
    fZoneAstroCalc(zoneAstroCalc)
{
    setTimeInMillis(getNow(), success); 
}

ChineseCalendar::ChineseCalendar(const ChineseCalendar& other) : Calendar(other) {
    isLeapYear = other.isLeapYear;
    fEpochYear = other.fEpochYear;
    fZoneAstroCalc = other.fZoneAstroCalc;
}

ChineseCalendar::~ChineseCalendar()
{
}

const char *ChineseCalendar::getType() const { 
    return "chinese";
}

static void U_CALLCONV initChineseCalZoneAstroCalc() {
    gChineseCalendarZoneAstroCalc = new SimpleTimeZone(CHINA_OFFSET, UNICODE_STRING_SIMPLE("CHINA_ZONE") );
    ucln_i18n_registerCleanup(UCLN_I18N_CHINESE_CALENDAR, calendar_chinese_cleanup);
}

const TimeZone* ChineseCalendar::getChineseCalZoneAstroCalc(void) const {
    umtx_initOnce(gChineseCalendarZoneAstroCalcInitOnce, &initChineseCalZoneAstroCalc);
    return gChineseCalendarZoneAstroCalc;
}






static const int32_t LIMITS[UCAL_FIELD_COUNT][4] = {
    
    
    {        1,        1,    83333,    83333}, 
    {        1,        1,       60,       60}, 
    {        0,        0,       11,       11}, 
    {        1,        1,       50,       55}, 
    {-1,-1,-1,-1}, 
    {        1,        1,       29,       30}, 
    {        1,        1,      353,      385}, 
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
    {        0,        0,        1,        1}, 
};





int32_t ChineseCalendar::handleGetLimit(UCalendarDateFields field, ELimitType limitType) const {
    return LIMITS[field][limitType];
}













int32_t ChineseCalendar::handleGetExtendedYear() {
    int32_t year;
    if (newestStamp(UCAL_ERA, UCAL_YEAR, kUnset) <= fStamp[UCAL_EXTENDED_YEAR]) {
        year = internalGet(UCAL_EXTENDED_YEAR, 1); 
    } else {
        int32_t cycle = internalGet(UCAL_ERA, 1) - 1; 
        
        year = cycle * 60 + internalGet(UCAL_YEAR, 1) - (fEpochYear - CHINESE_EPOCH_YEAR);
    }
    return year;
}









int32_t ChineseCalendar::handleGetMonthLength(int32_t extendedYear, int32_t month) const {
    int32_t thisStart = handleComputeMonthStart(extendedYear, month, TRUE) -
        kEpochStartAsJulianDay + 1; 
    int32_t nextStart = newMoonNear(thisStart + SYNODIC_GAP, TRUE);
    return nextStart - thisStart;
}



















void ChineseCalendar::handleComputeFields(int32_t julianDay, UErrorCode &) {

    computeChineseFields(julianDay - kEpochStartAsJulianDay, 
                         getGregorianYear(), getGregorianMonth(),
                         TRUE); 
}




const UFieldResolutionTable ChineseCalendar::CHINESE_DATE_PRECEDENCE[] =
{
    {
        { UCAL_DAY_OF_MONTH, kResolveSTOP },
        { UCAL_WEEK_OF_YEAR, UCAL_DAY_OF_WEEK, kResolveSTOP },
        { UCAL_WEEK_OF_MONTH, UCAL_DAY_OF_WEEK, kResolveSTOP },
        { UCAL_DAY_OF_WEEK_IN_MONTH, UCAL_DAY_OF_WEEK, kResolveSTOP },
        { UCAL_WEEK_OF_YEAR, UCAL_DOW_LOCAL, kResolveSTOP },
        { UCAL_WEEK_OF_MONTH, UCAL_DOW_LOCAL, kResolveSTOP },
        { UCAL_DAY_OF_WEEK_IN_MONTH, UCAL_DOW_LOCAL, kResolveSTOP },
        { UCAL_DAY_OF_YEAR, kResolveSTOP },
        { kResolveRemap | UCAL_DAY_OF_MONTH, UCAL_IS_LEAP_MONTH, kResolveSTOP },
        { kResolveSTOP }
    },
    {
        { UCAL_WEEK_OF_YEAR, kResolveSTOP },
        { UCAL_WEEK_OF_MONTH, kResolveSTOP },
        { UCAL_DAY_OF_WEEK_IN_MONTH, kResolveSTOP },
        { kResolveRemap | UCAL_DAY_OF_WEEK_IN_MONTH, UCAL_DAY_OF_WEEK, kResolveSTOP },
        { kResolveRemap | UCAL_DAY_OF_WEEK_IN_MONTH, UCAL_DOW_LOCAL, kResolveSTOP },
        { kResolveSTOP }
    },
    {{kResolveSTOP}}
};






const UFieldResolutionTable* ChineseCalendar::getFieldResolutionTable() const {
    return CHINESE_DATE_PRECEDENCE;
}














int32_t ChineseCalendar::handleComputeMonthStart(int32_t eyear, int32_t month, UBool useMonth) const {

    ChineseCalendar *nonConstThis = (ChineseCalendar*)this; 

    
    
    if (month < 0 || month > 11) {
        double m = month;
        eyear += (int32_t)ClockMath::floorDivide(m, 12.0, m);
        month = (int32_t)m;
    }

    int32_t gyear = eyear + fEpochYear - 1; 
    int32_t theNewYear = newYear(gyear);
    int32_t newMoon = newMoonNear(theNewYear + month * 29, TRUE);
    
    int32_t julianDay = newMoon + kEpochStartAsJulianDay;

    
    int32_t saveMonth = internalGet(UCAL_MONTH);
    int32_t saveIsLeapMonth = internalGet(UCAL_IS_LEAP_MONTH);

    
    int32_t isLeapMonth = useMonth ? saveIsLeapMonth : 0;

    UErrorCode status = U_ZERO_ERROR;
    nonConstThis->computeGregorianFields(julianDay, status);
    if (U_FAILURE(status))
        return 0;
    
    
    nonConstThis->computeChineseFields(newMoon, getGregorianYear(),
                         getGregorianMonth(), FALSE);        

    if (month != internalGet(UCAL_MONTH) ||
        isLeapMonth != internalGet(UCAL_IS_LEAP_MONTH)) {
        newMoon = newMoonNear(newMoon + SYNODIC_GAP, TRUE);
        julianDay = newMoon + kEpochStartAsJulianDay;
    }

    nonConstThis->internalSet(UCAL_MONTH, saveMonth);
    nonConstThis->internalSet(UCAL_IS_LEAP_MONTH, saveIsLeapMonth);

    return julianDay - 1;
}






void ChineseCalendar::add(UCalendarDateFields field, int32_t amount, UErrorCode& status) {
    switch (field) {
    case UCAL_MONTH:
        if (amount != 0) {
            int32_t dom = get(UCAL_DAY_OF_MONTH, status);
            if (U_FAILURE(status)) break;
            int32_t day = get(UCAL_JULIAN_DAY, status) - kEpochStartAsJulianDay; 
            if (U_FAILURE(status)) break;
            int32_t moon = day - dom + 1; 
            offsetMonth(moon, dom, amount);
        }
        break;
    default:
        Calendar::add(field, amount, status);
        break;
    }
}





void ChineseCalendar::add(EDateFields field, int32_t amount, UErrorCode& status) {
    add((UCalendarDateFields)field, amount, status);
}





void ChineseCalendar::roll(UCalendarDateFields field, int32_t amount, UErrorCode& status) {
    switch (field) {
    case UCAL_MONTH:
        if (amount != 0) {
            int32_t dom = get(UCAL_DAY_OF_MONTH, status);
            if (U_FAILURE(status)) break;
            int32_t day = get(UCAL_JULIAN_DAY, status) - kEpochStartAsJulianDay; 
            if (U_FAILURE(status)) break;
            int32_t moon = day - dom + 1; 

            
            

            
            
            
            int32_t m = get(UCAL_MONTH, status); 
            if (U_FAILURE(status)) break;
            if (isLeapYear) { 
                if (get(UCAL_IS_LEAP_MONTH, status) == 1) {
                    ++m;
                } else {
                    
                    
                    
                    
                    
                    
                    
                    int moon1 = moon -
                        (int) (CalendarAstronomer::SYNODIC_MONTH * (m - 0.5));
                    moon1 = newMoonNear(moon1, TRUE);
                    if (isLeapMonthBetween(moon1, moon)) {
                        ++m;
                    }
                }
                if (U_FAILURE(status)) break;
            }

            
            
            int32_t n = isLeapYear ? 13 : 12; 
            int32_t newM = (m + amount) % n;
            if (newM < 0) {
                newM += n;
            }

            if (newM != m) {
                offsetMonth(moon, dom, newM - m);
            }
        }
        break;
    default:
        Calendar::roll(field, amount, status);
        break;
    }
}

void ChineseCalendar::roll(EDateFields field, int32_t amount, UErrorCode& status) {
    roll((UCalendarDateFields)field, amount, status);
}


















double ChineseCalendar::daysToMillis(double days) const {
    double millis = days * (double)kOneDay;
    if (fZoneAstroCalc != NULL) {
        int32_t rawOffset, dstOffset;
        UErrorCode status = U_ZERO_ERROR;
        fZoneAstroCalc->getOffset(millis, FALSE, rawOffset, dstOffset, status);
        if (U_SUCCESS(status)) {
        	return millis - (double)(rawOffset + dstOffset);
        }
    }
    return millis - (double)CHINA_OFFSET;
}






double ChineseCalendar::millisToDays(double millis) const {
    if (fZoneAstroCalc != NULL) {
        int32_t rawOffset, dstOffset;
        UErrorCode status = U_ZERO_ERROR;
        fZoneAstroCalc->getOffset(millis, FALSE, rawOffset, dstOffset, status);
        if (U_SUCCESS(status)) {
        	return ClockMath::floorDivide(millis + (double)(rawOffset + dstOffset), kOneDay);
        }
    }
    return ClockMath::floorDivide(millis + (double)CHINA_OFFSET, kOneDay);
}














int32_t ChineseCalendar::winterSolstice(int32_t gyear) const {

    UErrorCode status = U_ZERO_ERROR;
    int32_t cacheValue = CalendarCache::get(&gChineseCalendarWinterSolsticeCache, gyear, status);

    if (cacheValue == 0) {
        
        
        
        
        double ms = daysToMillis(Grego::fieldsToDay(gyear, UCAL_DECEMBER, 1));

        umtx_lock(&astroLock);
        if(gChineseCalendarAstro == NULL) {
            gChineseCalendarAstro = new CalendarAstronomer();
            ucln_i18n_registerCleanup(UCLN_I18N_CHINESE_CALENDAR, calendar_chinese_cleanup);
        }
        gChineseCalendarAstro->setTime(ms);
        UDate solarLong = gChineseCalendarAstro->getSunTime(CalendarAstronomer::WINTER_SOLSTICE(), TRUE);
        umtx_unlock(&astroLock);

        
        cacheValue = (int32_t)millisToDays(solarLong);
        CalendarCache::put(&gChineseCalendarWinterSolsticeCache, gyear, cacheValue, status);
    }
    if(U_FAILURE(status)) {
        cacheValue = 0;
    }
    return cacheValue;
}










int32_t ChineseCalendar::newMoonNear(double days, UBool after) const {
    
    umtx_lock(&astroLock);
    if(gChineseCalendarAstro == NULL) {
        gChineseCalendarAstro = new CalendarAstronomer();
        ucln_i18n_registerCleanup(UCLN_I18N_CHINESE_CALENDAR, calendar_chinese_cleanup);
    }
    gChineseCalendarAstro->setTime(daysToMillis(days));
    UDate newMoon = gChineseCalendarAstro->getMoonTime(CalendarAstronomer::NEW_MOON(), after);
    umtx_unlock(&astroLock);
    
    return (int32_t) millisToDays(newMoon);
}








int32_t ChineseCalendar::synodicMonthsBetween(int32_t day1, int32_t day2) const {
    double roundme = ((day2 - day1) / CalendarAstronomer::SYNODIC_MONTH);
    return (int32_t) (roundme + (roundme >= 0 ? .5 : -.5));
}







int32_t ChineseCalendar::majorSolarTerm(int32_t days) const {
    
    umtx_lock(&astroLock);
    if(gChineseCalendarAstro == NULL) {
        gChineseCalendarAstro = new CalendarAstronomer();
        ucln_i18n_registerCleanup(UCLN_I18N_CHINESE_CALENDAR, calendar_chinese_cleanup);
    }
    gChineseCalendarAstro->setTime(daysToMillis(days));
    UDate solarLongitude = gChineseCalendarAstro->getSunLongitude();
    umtx_unlock(&astroLock);

    
    int32_t term = ( ((int32_t)(6 * solarLongitude / CalendarAstronomer::PI)) + 2 ) % 12;
    if (term < 1) {
        term += 12;
    }
    return term;
}






UBool ChineseCalendar::hasNoMajorSolarTerm(int32_t newMoon) const {
    return majorSolarTerm(newMoon) ==
        majorSolarTerm(newMoonNear(newMoon + SYNODIC_GAP, TRUE));
}














UBool ChineseCalendar::isLeapMonthBetween(int32_t newMoon1, int32_t newMoon2) const {

#ifdef U_DEBUG_CHNSECAL
    
    
    if (synodicMonthsBetween(newMoon1, newMoon2) >= 50) {
        U_DEBUG_CHNSECAL_MSG((
            "isLeapMonthBetween(%d, %d): Invalid parameters", newMoon1, newMoon2
            ));
    }
#endif

    return (newMoon2 >= newMoon1) &&
        (isLeapMonthBetween(newMoon1, newMoonNear(newMoon2 - SYNODIC_GAP, FALSE)) ||
         hasNoMajorSolarTerm(newMoon2));
}

















void ChineseCalendar::computeChineseFields(int32_t days, int32_t gyear, int32_t gmonth,
                                  UBool setAllFields) {

    
    
    
    
    int32_t solsticeBefore;
    int32_t solsticeAfter = winterSolstice(gyear);
    if (days < solsticeAfter) {
        solsticeBefore = winterSolstice(gyear - 1);
    } else {
        solsticeBefore = solsticeAfter;
        solsticeAfter = winterSolstice(gyear + 1);
    }

    
    
    
    int32_t firstMoon = newMoonNear(solsticeBefore + 1, TRUE);
    int32_t lastMoon = newMoonNear(solsticeAfter + 1, FALSE);
    int32_t thisMoon = newMoonNear(days + 1, FALSE); 
    
    isLeapYear = synodicMonthsBetween(firstMoon, lastMoon) == 12;

    int32_t month = synodicMonthsBetween(firstMoon, thisMoon);
    if (isLeapYear && isLeapMonthBetween(firstMoon, thisMoon)) {
        month--;
    }
    if (month < 1) {
        month += 12;
    }

    UBool isLeapMonth = isLeapYear &&
        hasNoMajorSolarTerm(thisMoon) &&
        !isLeapMonthBetween(firstMoon, newMoonNear(thisMoon - SYNODIC_GAP, FALSE));

    internalSet(UCAL_MONTH, month-1); 
    internalSet(UCAL_IS_LEAP_MONTH, isLeapMonth?1:0);

    if (setAllFields) {

        
        
        int32_t extended_year = gyear - fEpochYear;
        int cycle_year = gyear - CHINESE_EPOCH_YEAR;
        if (month < 11 ||
            gmonth >= UCAL_JULY) {
            extended_year++;
            cycle_year++;
        }
        int32_t dayOfMonth = days - thisMoon + 1;

        internalSet(UCAL_EXTENDED_YEAR, extended_year);

        
        int32_t yearOfCycle;
        int32_t cycle = ClockMath::floorDivide(cycle_year - 1, 60, yearOfCycle);
        internalSet(UCAL_ERA, cycle + 1);
        internalSet(UCAL_YEAR, yearOfCycle + 1);

        internalSet(UCAL_DAY_OF_MONTH, dayOfMonth);

        
        
        
        
        int32_t theNewYear = newYear(gyear);
        if (days < theNewYear) {
            theNewYear = newYear(gyear-1);
        }
        internalSet(UCAL_DAY_OF_YEAR, days - theNewYear + 1);
    }
}












int32_t ChineseCalendar::newYear(int32_t gyear) const {
    UErrorCode status = U_ZERO_ERROR;
    int32_t cacheValue = CalendarCache::get(&gChineseCalendarNewYearCache, gyear, status);

    if (cacheValue == 0) {

        int32_t solsticeBefore= winterSolstice(gyear - 1);
        int32_t solsticeAfter = winterSolstice(gyear);
        int32_t newMoon1 = newMoonNear(solsticeBefore + 1, TRUE);
        int32_t newMoon2 = newMoonNear(newMoon1 + SYNODIC_GAP, TRUE);
        int32_t newMoon11 = newMoonNear(solsticeAfter + 1, FALSE);
        
        if (synodicMonthsBetween(newMoon1, newMoon11) == 12 &&
            (hasNoMajorSolarTerm(newMoon1) || hasNoMajorSolarTerm(newMoon2))) {
            cacheValue = newMoonNear(newMoon2 + SYNODIC_GAP, TRUE);
        } else {
            cacheValue = newMoon2;
        }

        CalendarCache::put(&gChineseCalendarNewYearCache, gyear, cacheValue, status);
    }
    if(U_FAILURE(status)) {
        cacheValue = 0;
    }
    return cacheValue;
}












void ChineseCalendar::offsetMonth(int32_t newMoon, int32_t dom, int32_t delta) {
    UErrorCode status = U_ZERO_ERROR;

    
    newMoon += (int32_t) (CalendarAstronomer::SYNODIC_MONTH * (delta - 0.5));

    
    newMoon = newMoonNear(newMoon, TRUE);

    
    int32_t jd = newMoon + kEpochStartAsJulianDay - 1 + dom;

    
    
    if (dom > 29) {
        set(UCAL_JULIAN_DAY, jd-1);
        
        
        
        
        complete(status);
        if (U_FAILURE(status)) return;
        if (getActualMaximum(UCAL_DAY_OF_MONTH, status) >= dom) {
            if (U_FAILURE(status)) return;
            set(UCAL_JULIAN_DAY, jd);
        }
    } else {
        set(UCAL_JULIAN_DAY, jd);
    }
}


UBool
ChineseCalendar::inDaylightTime(UErrorCode& status) const
{
    
    if (U_FAILURE(status) || !getTimeZone().useDaylightTime()) 
        return FALSE;

    
    ((ChineseCalendar*)this)->complete(status); 

    return (UBool)(U_SUCCESS(status) ? (internalGet(UCAL_DST_OFFSET) != 0) : FALSE);
}



static UDate     gSystemDefaultCenturyStart       = DBL_MIN;
static int32_t   gSystemDefaultCenturyStartYear   = -1;
static icu::UInitOnce gSystemDefaultCenturyInitOnce = U_INITONCE_INITIALIZER;


UBool ChineseCalendar::haveDefaultCentury() const
{
    return TRUE;
}

UDate ChineseCalendar::defaultCenturyStart() const
{
    return internalGetDefaultCenturyStart();
}

int32_t ChineseCalendar::defaultCenturyStartYear() const
{
    return internalGetDefaultCenturyStartYear();
}

static void U_CALLCONV initializeSystemDefaultCentury()
{
    
    
    
    UErrorCode status = U_ZERO_ERROR;
    ChineseCalendar calendar(Locale("@calendar=chinese"),status);
    if (U_SUCCESS(status)) {
        calendar.setTime(Calendar::getNow(), status);
        calendar.add(UCAL_YEAR, -80, status);
        gSystemDefaultCenturyStart     = calendar.getTime(status);
        gSystemDefaultCenturyStartYear = calendar.get(UCAL_YEAR, status);
    }
    
    
}

UDate
ChineseCalendar::internalGetDefaultCenturyStart() const
{
    
    umtx_initOnce(gSystemDefaultCenturyInitOnce, &initializeSystemDefaultCentury);
    return gSystemDefaultCenturyStart;
}

int32_t
ChineseCalendar::internalGetDefaultCenturyStartYear() const
{
    
    umtx_initOnce(gSystemDefaultCenturyInitOnce, &initializeSystemDefaultCentury);
    return    gSystemDefaultCenturyStartYear;
}

UOBJECT_DEFINE_RTTI_IMPLEMENTATION(ChineseCalendar)

U_NAMESPACE_END

#endif

