














#include "islamcal.h"

#if !UCONFIG_NO_FORMATTING

#include "umutex.h"
#include <float.h>
#include "gregoimp.h" 
#include "astro.h" 
#include "uhash.h"
#include "ucln_in.h"
#include "uassert.h"

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






static const int32_t CIVIL_EPOC = 1948440;




static const int32_t ASTRONOMICAL_EPOC = 1948439;


static const int32_t UMALQURA_YEAR_START = 1318;
static const int32_t UMALQURA_YEAR_END = 1480;

static const int UMALQURA_MONTHLENGTH[] = {
    
                            0x0574,           0x0975,           0x06A7,           0x0257,           0x052B,
    
                            0x0695,           0x06CA,           0x0AD5,           0x055B,           0x025B,
    
                            0x092D,           0x0C95,           0x0D4A,           0x0E5B,           0x025B,
    
                            0x0AD5,           0x055A,           0x0AAB,           0x044B,           0x06A5,
    
                            0x0752,           0x0BA9,           0x0374,           0x0AB6,           0x0556,
    
                            0x0AAA,           0x0D52,           0x0DA9,           0x05D4,           0x0AEA,
    
                            0x04DD,           0x026E,           0x092E,           0x0AA6,           0x0D54,
    
                            0x05AA,           0x05B5,           0x02B4,           0x0937,           0x049B,
    
                            0x0A4B,           0x0B25,           0x0B54,           0x0B6A,           0x056D,
    
                            0x04AD,           0x0A55,           0x0D25,           0x0E92,           0x0EC9,
    
                            0x06D4,           0x0ADA,           0x056B,           0x04AB,           0x0685,
    
                            0x0B49,           0x0BA4,           0x0BB2,           0x05B5,           0x02BA,
    
                            0x095B,           0x04AB,           0x0555,           0x06B2,           0x06D9,
    
                            0x02EC,           0x096E,           0x04AE,           0x0A56,           0x0D2A,
    
                            0x0D55,           0x05AA,           0x0AB5,           0x04BB,           0x005B,
    
                            0x092B,           0x0A95,           0x034A,           0x0BA5,           0x05AA,
    
                            0x0AB5,           0x0556,           0x0A96,           0x0B4A,           0x0EA5,
    
                            0x0752,           0x06E9,           0x036A,           0x0AAD,           0x0555,
    
                            0x0AA5,           0x0B52,           0x0BA9,           0x05B4,           0x09BA,
    
                            0x04DB,           0x025D,           0x052D,           0x0AA5,           0x0AD4,
    
                            0x0AEA,           0x056D,           0x04BD,           0x023D,           0x091D,
    
                            0x0A95,           0x0B4A,           0x0B5A,           0x056D,           0x02B6,
    
                            0x093B,           0x049B,           0x0655,           0x06A9,           0x0754,
    
                            0x0B6A,           0x056C,           0x0AAD,           0x0555,           0x0B29,
    
                            0x0B92,           0x0BA9,           0x05D4,           0x0ADA,           0x055A,
    
                            0x0AAB,           0x0595,           0x0749,           0x0764,           0x0BAA,
    
                            0x05B5,           0x02B6,           0x0A56,           0x0E4D,           0x0B25,
    
                            0x0B52,           0x0B6A,           0x05AD,           0x02AE,           0x092F,
    
                            0x0497,           0x064B,           0x06A5,           0x06AC,           0x0AD6,
    
                            0x055D,           0x049D,           0x0A4D,           0x0D16,           0x0D95,
    
                            0x05AA,           0x05B5,           0x029A,           0x095B,           0x04AC,
    
                            0x0595,           0x06CA,           0x06E4,           0x0AEA,           0x04F5,
    
                            0x02B6,           0x0956,           0x0AAA
};

int32_t getUmalqura_MonthLength(int32_t y, int32_t m) {
    int32_t mask = (int32_t) (0x01 << (11 - m));    
    if((UMALQURA_MONTHLENGTH[y] & mask) == 0 )
        return 29;
    else
        return 30;

}





const char *IslamicCalendar::getType() const {
    const char *sType = NULL;

    switch (cType) {
    case CIVIL:
        sType = "islamic-civil";
        break;
    case ASTRONOMICAL:
        sType = "islamic";
        break;
    case TBLA:
        sType = "islamic-tbla";
        break;
    case UMALQURA:
        sType = "islamic-umalqura";
        break;
    default:
        U_ASSERT(false); 
        sType = "islamic";  
        break;
    }
    return sType;
}

Calendar* IslamicCalendar::clone() const {
    return new IslamicCalendar(*this);
}

IslamicCalendar::IslamicCalendar(const Locale& aLocale, UErrorCode& success, ECalculationType type)
:   Calendar(TimeZone::createDefault(), aLocale, success),
cType(type)
{
    setTimeInMillis(getNow(), success); 
}

IslamicCalendar::IslamicCalendar(const IslamicCalendar& other) : Calendar(other), cType(other.cType) {
}

IslamicCalendar::~IslamicCalendar()
{
}

void IslamicCalendar::setCalculationType(ECalculationType type, UErrorCode &status)
{
    if (cType != type) {
        
        
        UDate m = getTimeInMillis(status);
        cType = type;
        clear();
        setTimeInMillis(m, status);
    }
}







UBool IslamicCalendar::isCivil() {
    return (cType == CIVIL);
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





int32_t IslamicCalendar::yearStart(int32_t year) const{
    if (cType == CIVIL || cType == TBLA ||
        (cType == UMALQURA && year < UMALQURA_YEAR_START)) 
    {
        return (year-1)*354 + ClockMath::floorDivide((3+11*year),30);
    } else if(cType==ASTRONOMICAL){
        return trueMonthStart(12*(year-1));
    } else {
        int32_t ys = yearStart(UMALQURA_YEAR_START-1);
        ys+= handleGetYearLength(UMALQURA_YEAR_START-1);
        for(int i=UMALQURA_YEAR_START; i< year; i++){  
            ys+= handleGetYearLength(i);
        }
        return ys;
    }
}








int32_t IslamicCalendar::monthStart(int32_t year, int32_t month) const {
    if (cType == CIVIL || cType == TBLA) {
        return (int32_t)uprv_ceil(29.5*month)
            + (year-1)*354 + (int32_t)ClockMath::floorDivide((3+11*year),30);
    } else if(cType==ASTRONOMICAL){
        return trueMonthStart(12*(year-1) + month);
    } else {
        int32_t ms = yearStart(year);
        for(int i=0; i< month; i++){
            ms+= handleGetMonthLength(year, i);
        }
        return ms;
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

    if (cType == CIVIL || cType == TBLA ||
        (cType == UMALQURA && (extendedYear<UMALQURA_YEAR_START || extendedYear>UMALQURA_YEAR_END)) ) {
        length = 29 + (month+1) % 2;
        if (month == DHU_AL_HIJJAH && civilLeapYear(extendedYear)) {
            length++;
        }
    } else if(cType == ASTRONOMICAL){
        month = 12*(extendedYear-1) + month;
        length =  trueMonthStart(month+1) - trueMonthStart(month) ;
    } else {
        length = getUmalqura_MonthLength(extendedYear - UMALQURA_YEAR_START, month);
    }
    return length;
}





int32_t IslamicCalendar::handleGetYearLength(int32_t extendedYear) const {
    if (cType == CIVIL || cType == TBLA ||
        (cType == UMALQURA && (extendedYear<UMALQURA_YEAR_START || extendedYear>UMALQURA_YEAR_END)) ) {
        return 354 + (civilLeapYear(extendedYear) ? 1 : 0);
    } else if(cType == ASTRONOMICAL){
        int32_t month = 12*(extendedYear-1);
        return (trueMonthStart(month + 12) - trueMonthStart(month));
    } else {
        int len = 0;
        for(int i=0; i<12; i++)
            len += handleGetMonthLength(extendedYear, i);
        return len;
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
    int32_t days = julianDay - CIVIL_EPOC;

    if (cType == CIVIL || cType == TBLA) {
        if(cType == TBLA)
            days = julianDay - ASTRONOMICAL_EPOC;
        
        year  = (int)ClockMath::floorDivide( (double)(30 * days + 10646) , 10631.0 );
        month = (int32_t)uprv_ceil((days - 29 - yearStart(year)) / 29.5 );
        month = month<11?month:11;
        startDate = monthStart(year, month);
    } else if(cType == ASTRONOMICAL){
        
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
    } else if(cType == UMALQURA) {
        int32_t umalquraStartdays = yearStart(UMALQURA_YEAR_START) ;
        if( days < umalquraStartdays){
                
                year  = (int)ClockMath::floorDivide( (double)(30 * days + 10646) , 10631.0 );
                month = (int32_t)uprv_ceil((days - 29 - yearStart(year)) / 29.5 );
                month = month<11?month:11;
                startDate = monthStart(year, month);
            }else{
                int y =UMALQURA_YEAR_START-1, m =0;
                long d = 1;
                while(d > 0){ 
                    y++; 
                    d = days - yearStart(y) +1;
                    if(d == handleGetYearLength(y)){
                        m=11;
                        break;
                    }else if(d < handleGetYearLength(y) ){
                        int monthLen = handleGetMonthLength(y, m); 
                        m=0;
                        while(d > monthLen){
                            d -= monthLen;
                            m++;
                            monthLen = handleGetMonthLength(y, m);
                        }
                        break;
                    }
                }
                year = y;
                month = m;
            }
    } else { 
      U_ASSERT(false); 
      year=month=0;
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






static UDate           gSystemDefaultCenturyStart       = DBL_MIN;
static int32_t         gSystemDefaultCenturyStartYear   = -1;
static icu::UInitOnce  gSystemDefaultCenturyInit        = U_INITONCE_INITIALIZER;


UBool IslamicCalendar::haveDefaultCentury() const
{
    return TRUE;
}

UDate IslamicCalendar::defaultCenturyStart() const
{
    
    umtx_initOnce(gSystemDefaultCenturyInit, &initializeSystemDefaultCentury);
    return gSystemDefaultCenturyStart;
}

int32_t IslamicCalendar::defaultCenturyStartYear() const
{
    
    umtx_initOnce(gSystemDefaultCenturyInit, &initializeSystemDefaultCentury);
    return gSystemDefaultCenturyStartYear;
}


void U_CALLCONV
IslamicCalendar::initializeSystemDefaultCentury()
{
    
    
    
    UErrorCode status = U_ZERO_ERROR;
    IslamicCalendar calendar(Locale("@calendar=islamic-civil"),status);
    if (U_SUCCESS(status)) {
        calendar.setTime(Calendar::getNow(), status);
        calendar.add(UCAL_YEAR, -80, status);

        gSystemDefaultCenturyStart = calendar.getTime(status);
        gSystemDefaultCenturyStartYear = calendar.get(UCAL_YEAR, status);
    }
    
    
}



UOBJECT_DEFINE_RTTI_IMPLEMENTATION(IslamicCalendar)

U_NAMESPACE_END

#endif

