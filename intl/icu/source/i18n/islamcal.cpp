














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


static const int32_t UMALQURA_YEAR_START = 1300;
static const int32_t UMALQURA_YEAR_END = 1600;

static const int UMALQURA_MONTHLENGTH[] = {
    
                            0x0AAA,           0x0D54,           0x0EC9,
    
                            0x06D4,           0x06EA,           0x036C,           0x0AAD,           0x0555,
    
                            0x06A9,           0x0792,           0x0BA9,           0x05D4,           0x0ADA,
    
                            0x055C,           0x0D2D,           0x0695,           0x074A,           0x0B54,
    
                            0x0B6A,           0x05AD,           0x04AE,           0x0A4F,           0x0517,
    
                            0x068B,           0x06A5,           0x0AD5,           0x02D6,           0x095B,
    
                            0x049D,           0x0A4D,           0x0D26,           0x0D95,           0x05AC,
    
                            0x09B6,           0x02BA,           0x0A5B,           0x052B,           0x0A95,
    
                            0x06CA,           0x0AE9,           0x02F4,           0x0976,           0x02B6,
    
                            0x0956,           0x0ACA,           0x0BA4,           0x0BD2,           0x05D9,
    
                            0x02DC,           0x096D,           0x054D,           0x0AA5,           0x0B52,
    
                            0x0BA5,           0x05B4,           0x09B6,           0x0557,           0x0297,
    
                            0x054B,           0x06A3,           0x0752,           0x0B65,           0x056A,
    
                            0x0AAB,           0x052B,           0x0C95,           0x0D4A,           0x0DA5,
    
                            0x05CA,           0x0AD6,           0x0957,           0x04AB,           0x094B,
    
                            0x0AA5,           0x0B52,           0x0B6A,           0x0575,           0x0276,
    
                            0x08B7,           0x045B,           0x0555,           0x05A9,           0x05B4,
    
                            0x09DA,           0x04DD,           0x026E,           0x0936,           0x0AAA,
    
                            0x0D54,           0x0DB2,           0x05D5,           0x02DA,           0x095B,
    
                            0x04AB,           0x0A55,           0x0B49,           0x0B64,           0x0B71,
    
                            0x05B4,           0x0AB5,           0x0A55,           0x0D25,           0x0E92,
    
                            0x0EC9,           0x06D4,           0x0AE9,           0x096B,           0x04AB,
    
                            0x0A93,           0x0D49,         0x0DA4,           0x0DB2,           0x0AB9,
    
                            0x04BA,           0x0A5B,           0x052B,           0x0A95,           0x0B2A,
    
                            0x0B55,           0x055C,           0x04BD,           0x023D,           0x091D,
    
                            0x0A95,           0x0B4A,           0x0B5A,           0x056D,           0x02B6,
    
                            0x093B,           0x049B,           0x0655,           0x06A9,           0x0754,
    
                            0x0B6A,           0x056C,           0x0AAD,           0x0555,           0x0B29,
    
                            0x0B92,           0x0BA9,           0x05D4,           0x0ADA,           0x055A,
    
                            0x0AAB,           0x0595,           0x0749,           0x0764,           0x0BAA,
    
                            0x05B5,           0x02B6,           0x0A56,           0x0E4D,           0x0B25,
    
                            0x0B52,           0x0B6A,           0x05AD,           0x02AE,           0x092F,
    
                            0x0497,           0x064B,           0x06A5,           0x06AC,           0x0AD6,
    
                            0x055D,           0x049D,           0x0A4D,           0x0D16,           0x0D95,
    
                            0x05AA,           0x05B5,           0x02DA,           0x095B,           0x04AD,
    
                            0x0595,           0x06CA,           0x06E4,           0x0AEA,           0x04F5,
    
                            0x02B6,           0x0956,           0x0AAA,           0x0B54,           0x0BD2,
    
                            0x05D9,           0x02EA,           0x096D,           0x04AD,           0x0A95,
    
                            0x0B4A,           0x0BA5,           0x05B2,           0x09B5,           0x04D6,
    
                            0x0A97,           0x0547,           0x0693,           0x0749,           0x0B55,
    
                            0x056A,           0x0A6B,           0x052B,           0x0A8B,           0x0D46,           0x0DA3,           0x05CA,           0x0AD6,           0x04DB,           0x026B,           0x094B,
    
                            0x0AA5,           0x0B52,           0x0B69,           0x0575,           0x0176,           0x08B7,           0x025B,           0x052B,           0x0565,           0x05B4,           0x09DA,
    
                            0x04ED,           0x016D,           0x08B6,           0x0AA6,           0x0D52,           0x0DA9,           0x05D4,           0x0ADA,           0x095B,           0x04AB,           0x0653,
    
                            0x0729,           0x0762,           0x0BA9,           0x05B2,           0x0AB5,           0x0555,           0x0B25,           0x0D92,           0x0EC9,           0x06D2,           0x0AE9,
    
                            0x056B,           0x04AB,           0x0A55,           0x0D29,           0x0D54,           0x0DAA,           0x09B5,           0x04BA,           0x0A3B,           0x049B,           0x0A4D,
    
                            0x0AAA,           0x0AD5,           0x02DA,           0x095D,           0x045E,           0x0A2E,           0x0C9A,           0x0D55,           0x06B2,           0x06B9,           0x04BA,
    
                            0x0A5D,           0x052D,           0x0A95,           0x0B52,           0x0BA8,           0x0BB4,           0x05B9,           0x02DA,           0x095A,           0x0B4A,           0x0DA4,
    
                            0x0ED1,           0x06E8,           0x0B6A,           0x056D,           0x0535,           0x0695,           0x0D4A,           0x0DA8,           0x0DD4,           0x06DA,           0x055B,
    
                            0x029D,           0x062B,           0x0B15,           0x0B4A,           0x0B95,           0x05AA,           0x0AAE,           0x092E,           0x0C8F,           0x0527,           0x0695,
    
                            0x06AA,           0x0AD6,           0x055D,           0x029D
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






static const int8_t umAlQuraYrStartEstimateFix[] = {
     0,  0, -1,  0, -1,  0,  0,  0,  0,  0, 
    -1,  0,  0,  0,  0,  0,  0,  0, -1,  0, 
     1,  0,  1,  1,  0,  0,  0,  0,  1,  0, 
     0,  0,  0,  0,  0,  0,  1,  0,  0,  0, 
     0,  0,  1,  0,  0, -1, -1,  0,  0,  0, 
     1,  0,  0, -1,  0,  0,  0,  1,  1,  0, 
     0,  0,  0,  0,  0,  0,  0, -1,  0,  0, 
     0,  1,  1,  0,  0, -1,  0,  1,  0,  1, 
     1,  0,  0, -1,  0,  1,  0,  0,  0, -1, 
     0,  1,  0,  1,  0,  0,  0, -1,  0,  0, 
     0,  0, -1, -1,  0, -1,  0,  1,  0,  0, 
     0, -1,  0,  0,  0,  1,  0,  0,  0,  0, 
     0,  1,  0,  0, -1, -1,  0,  0,  0,  1, 
     0,  0, -1, -1,  0, -1,  0,  0, -1, -1, 
     0, -1,  0, -1,  0,  0, -1, -1,  0,  0, 
     0,  0,  0,  0, -1,  0,  1,  0,  1,  1, 
     0,  0, -1,  0,  1,  0,  0,  0,  0,  0, 
     1,  0,  1,  0,  0,  0, -1,  0,  1,  0, 
     0, -1, -1,  0,  0,  0,  1,  0,  0,  0, 
     0,  0,  0,  0,  1,  0,  0,  0,  0,  0, 
     1,  0,  0, -1,  0,  0,  0,  1,  1,  0, 
     0, -1,  0,  1,  0,  1,  1,  0,  0,  0, 
     0,  1,  0,  0,  0, -1,  0,  0,  0,  1, 
     0,  0,  0, -1,  0,  0,  0,  0,  0, -1, 
     0, -1,  0,  1,  0,  0,  0, -1,  0,  1, 
     0,  1,  0,  0,  0,  0,  0,  1,  0,  0, 
    -1,  0,  0,  0,  0,  1,  0,  0,  0, -1, 
     0,  0,  0,  0, -1, -1,  0, -1,  0,  1, 
     0,  0, -1, -1,  0,  0,  1,  1,  0,  0, 
    -1,  0,  0,  0,  0,  1,  0,  0,  0,  0, 
     1 
};




UBool IslamicCalendar::civilLeapYear(int32_t year)
{
    return (14 + 11 * year) % 30 < 11;
}





int32_t IslamicCalendar::yearStart(int32_t year) const{
    if (cType == CIVIL || cType == TBLA ||
        (cType == UMALQURA && (year < UMALQURA_YEAR_START || year > UMALQURA_YEAR_END))) 
    {
        return (year-1)*354 + ClockMath::floorDivide((3+11*year),30);
    } else if(cType==ASTRONOMICAL){
        return trueMonthStart(12*(year-1));
    } else {
        year -= UMALQURA_YEAR_START;
        
        int32_t yrStartLinearEstimate = (int32_t)((354.36720 * (double)year) + 460322.05 + 0.5);
        
        return yrStartLinearEstimate + umAlQuraYrStartEstimateFix[year];
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
        for(int i=0; i<12; i++) {
            len += handleGetMonthLength(extendedYear, i);
        }
        return len;
    }
}













int32_t IslamicCalendar::handleComputeMonthStart(int32_t eyear, int32_t month, UBool ) const {
    
    
    if (month > 11) {
        eyear += (month / 12);
        month %= 12;
    } else if (month < 0) {
        month++;
        eyear += (month / 12) - 1;
        month = (month % 12) + 11;
    }
    return monthStart(eyear, month) + ((cType == TBLA)? ASTRONOMICAL_EPOC: CIVIL_EPOC) - 1;
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
    int32_t startDate;
    int32_t days = julianDay - CIVIL_EPOC;

    if (cType == CIVIL || cType == TBLA) {
        if(cType == TBLA) {
            days = julianDay - ASTRONOMICAL_EPOC;
        }
        
        year  = (int)ClockMath::floorDivide( (double)(30 * days + 10646) , 10631.0 );
        month = (int32_t)uprv_ceil((days - 29 - yearStart(year)) / 29.5 );
        month = month<11?month:11;
        startDate = monthStart(year, month);
    } else if(cType == ASTRONOMICAL){
        
        int32_t months = (int32_t)uprv_floor((double)days / CalendarAstronomer::SYNODIC_MONTH);

        startDate = (int32_t)uprv_floor(months * CalendarAstronomer::SYNODIC_MONTH);

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

    
    dayOfYear = (days - monthStart(year, 0)) + 1;


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
    
    if (U_FAILURE(status) || !getTimeZone().useDaylightTime()) 
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

