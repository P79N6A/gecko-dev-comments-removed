














#include "hebrwcal.h"

#if !UCONFIG_NO_FORMATTING

#include "umutex.h"
#include <float.h>
#include "gregoimp.h" 
#include "astro.h" 
#include "uhash.h"
#include "ucln_in.h"











static const int32_t LIMITS[UCAL_FIELD_COUNT][4] = {
    
    
    {        0,        0,        0,        0}, 
    { -5000000, -5000000,  5000000,  5000000}, 
    {        0,        0,       12,       12}, 
    {        1,        1,       51,       56}, 
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
    {-1,-1,-1,-1}, 
};








static const int8_t MONTH_LENGTH[][3] = {
    
    {   30,         30,         30     },           
    {   29,         29,         30     },           
    {   29,         30,         30     },           
    {   29,         29,         29     },           
    {   30,         30,         30     },           
    {   30,         30,         30     },           
    {   29,         29,         29     },           
    {   30,         30,         30     },           
    {   29,         29,         29     },           
    {   30,         30,         30     },           
    {   29,         29,         29     },           
    {   30,         30,         30     },           
    {   29,         29,         29     },           
};







static const int16_t MONTH_START[][3] = {
    
    {    0,          0,          0  },          
    {   30,         30,         30  },          
    {   59,         59,         60  },          
    {   88,         89,         90  },          
    {  117,        118,        119  },          
    {  147,        148,        149  },          
    {  147,        148,        149  },          
    {  176,        177,        178  },          
    {  206,        207,        208  },          
    {  235,        236,        237  },          
    {  265,        266,        267  },          
    {  294,        295,        296  },          
    {  324,        325,        326  },          
    {  353,        354,        355  },          
};




static const int16_t  LEAP_MONTH_START[][3] = {
    
    {    0,          0,          0  },          
    {   30,         30,         30  },          
    {   59,         59,         60  },          
    {   88,         89,         90  },          
    {  117,        118,        119  },          
    {  147,        148,        149  },          
    {  177,        178,        179  },          
    {  206,        207,        208  },          
    {  236,        237,        238  },          
    {  265,        266,        267  },          
    {  295,        296,        297  },          
    {  324,        325,        326  },          
    {  354,        355,        356  },          
    {  383,        384,        385  },          
};

static icu::CalendarCache *gCache =  NULL;

U_CDECL_BEGIN
static UBool calendar_hebrew_cleanup(void) {
    delete gCache;
    gCache = NULL;
    return TRUE;
}
U_CDECL_END

U_NAMESPACE_BEGIN









HebrewCalendar::HebrewCalendar(const Locale& aLocale, UErrorCode& success)
:   Calendar(TimeZone::createDefault(), aLocale, success)

{
    setTimeInMillis(getNow(), success); 
}


HebrewCalendar::~HebrewCalendar() {
}

const char *HebrewCalendar::getType() const {
    return "hebrew";
}

Calendar* HebrewCalendar::clone() const {
    return new HebrewCalendar(*this);
}

HebrewCalendar::HebrewCalendar(const HebrewCalendar& other) : Calendar(other) {
}




































void HebrewCalendar::add(UCalendarDateFields field, int32_t amount, UErrorCode& status)
{
    if(U_FAILURE(status)) {
        return;
    }
    switch (field) {
  case UCAL_MONTH: 
      {
          
          
          
          
          
          int32_t month = get(UCAL_MONTH, status);
          int32_t year = get(UCAL_YEAR, status);
          UBool acrossAdar1;
          if (amount > 0) {
              acrossAdar1 = (month < ADAR_1); 
              month += amount;
              for (;;) {
                  if (acrossAdar1 && month>=ADAR_1 && !isLeapYear(year)) {
                      ++month;
                  }
                  if (month <= ELUL) {
                      break;
                  }
                  month -= ELUL+1;
                  ++year;
                  acrossAdar1 = TRUE;
              }
          } else {
              acrossAdar1 = (month > ADAR_1); 
              month += amount;
              for (;;) {
                  if (acrossAdar1 && month<=ADAR_1 && !isLeapYear(year)) {
                      --month;
                  }
                  if (month >= 0) {
                      break;
                  }
                  month += ELUL+1;
                  --year;
                  acrossAdar1 = TRUE;
              }
          }
          set(UCAL_MONTH, month);
          set(UCAL_YEAR, year);
          pinField(UCAL_DAY_OF_MONTH, status);
          break;
      }

  default:
      Calendar::add(field, amount, status);
      break;
    }
}




void HebrewCalendar::add(EDateFields field, int32_t amount, UErrorCode& status)
{
    add((UCalendarDateFields)field, amount, status);
}

































void HebrewCalendar::roll(UCalendarDateFields field, int32_t amount, UErrorCode& status)
{
    if(U_FAILURE(status)) {
        return;
    }
    switch (field) {
  case UCAL_MONTH:
      {
          int32_t month = get(UCAL_MONTH, status);
          int32_t year = get(UCAL_YEAR, status);

          UBool leapYear = isLeapYear(year);
          int32_t yearLength = monthsInYear(year);
          int32_t newMonth = month + (amount % yearLength);
          
          
          
          
          if (!leapYear) {
              if (amount > 0 && month < ADAR_1 && newMonth >= ADAR_1) {
                  newMonth++;
              } else if (amount < 0 && month > ADAR_1 && newMonth <= ADAR_1) {
                  newMonth--;
              }
          }
          set(UCAL_MONTH, (newMonth + 13) % 13);
          pinField(UCAL_DAY_OF_MONTH, status);
          return;
      }
  default:
      Calendar::roll(field, amount, status);
    }
}

void HebrewCalendar::roll(EDateFields field, int32_t amount, UErrorCode& status) {
    roll((UCalendarDateFields)field, amount, status);
}







static const int32_t HOUR_PARTS = 1080;
static const int32_t DAY_PARTS  = 24*HOUR_PARTS;




static const int32_t  MONTH_DAYS = 29;
static const int32_t MONTH_FRACT = 12*HOUR_PARTS + 793;
static const int32_t MONTH_PARTS = MONTH_DAYS*DAY_PARTS + MONTH_FRACT;




static const int32_t BAHARAD = 11*HOUR_PARTS + 204;





















int32_t HebrewCalendar::startOfYear(int32_t year, UErrorCode &status)
{
    ucln_i18n_registerCleanup(UCLN_I18N_HEBREW_CALENDAR, calendar_hebrew_cleanup);
    int32_t day = CalendarCache::get(&gCache, year, status);

    if (day == 0) {
        int32_t months = (235 * year - 234) / 19;           

        int64_t frac = (int64_t)months * MONTH_FRACT + BAHARAD;  
        day  = months * 29 + (int32_t)(frac / DAY_PARTS);        
        frac = frac % DAY_PARTS;                        

        int32_t wd = (day % 7);                        

        if (wd == 2 || wd == 4 || wd == 6) {
            
            day += 1;
            wd = (day % 7);
        }
        if (wd == 1 && frac > 15*HOUR_PARTS+204 && !isLeapYear(year) ) {
            
            
            
            day += 2;
        }
        else if (wd == 0 && frac > 21*HOUR_PARTS+589 && isLeapYear(year-1) ) {
            
            
            
            day += 1;
        }
        CalendarCache::put(&gCache, year, day, status);
    }
    return day;
}







int32_t HebrewCalendar::absoluteDayToDayOfWeek(int32_t day)
{
    
    return (day % 7) + 1;
}







int32_t HebrewCalendar::yearType(int32_t year) const
{
    int32_t yearLength = handleGetYearLength(year);

    if (yearLength > 380) {
        yearLength -= 30;        
    }

    int type = 0;

    switch (yearLength) {
  case 353:
      type = 0; break;
  case 354:
      type = 1; break;
  case 355:
      type = 2; break;
  default:
      
      type = 1;
    }
    return type;
}







UBool HebrewCalendar::isLeapYear(int32_t year) {
    
    int32_t x = (year*12 + 17) % 19;
    return x >= ((x < 0) ? -7 : 12);
}

int32_t HebrewCalendar::monthsInYear(int32_t year) {
    return isLeapYear(year) ? 13 : 12;
}








int32_t HebrewCalendar::handleGetLimit(UCalendarDateFields field, ELimitType limitType) const {
    return LIMITS[field][limitType];
}





int32_t HebrewCalendar::handleGetMonthLength(int32_t extendedYear, int32_t month) const {
    
    
    
    
    
    
    while (month < 0) {
        month += monthsInYear(--extendedYear);
    }
    
    while (month > 12) {
        month -= monthsInYear(extendedYear++);
    }

    switch (month) {
    case HESHVAN:
    case KISLEV:
      
      return MONTH_LENGTH[month][yearType(extendedYear)];

    default:
      
      return MONTH_LENGTH[month][0];
    }
}





int32_t HebrewCalendar::handleGetYearLength(int32_t eyear) const {
    UErrorCode status = U_ZERO_ERROR;
    return startOfYear(eyear+1, status) - startOfYear(eyear, status);
}


























void HebrewCalendar::handleComputeFields(int32_t julianDay, UErrorCode &status) {
    int32_t d = julianDay - 347997;
    double m = ((d * (double)DAY_PARTS)/ (double) MONTH_PARTS);         
    int32_t year = (int32_t)( ((19. * m + 234.) / 235.) + 1.);     
    int32_t ys  = startOfYear(year, status);                   
    int32_t dayOfYear = (d - ys);

    
    while (dayOfYear < 1) {
        year--;
        ys  = startOfYear(year, status);
        dayOfYear = (d - ys);
    }

    
    int32_t type = yearType(year);
    UBool isLeap = isLeapYear(year);

    int32_t month = 0;
    int32_t momax = sizeof(MONTH_START) / (3 * sizeof(MONTH_START[0][0]));
    while (month < momax && dayOfYear > (  isLeap ? LEAP_MONTH_START[month][type] : MONTH_START[month][type] ) ) {
        month++;
    }
    if (month >= momax || month<=0) {
        
        
        
        
        
        
        
        
        
        status = U_ILLEGAL_ARGUMENT_ERROR;
        return;
    }
    month--;
    int dayOfMonth = dayOfYear - (isLeap ? LEAP_MONTH_START[month][type] : MONTH_START[month][type]);

    internalSet(UCAL_ERA, 0);
    internalSet(UCAL_YEAR, year);
    internalSet(UCAL_EXTENDED_YEAR, year);
    internalSet(UCAL_MONTH, month);
    internalSet(UCAL_DAY_OF_MONTH, dayOfMonth);
    internalSet(UCAL_DAY_OF_YEAR, dayOfYear);       
}








int32_t HebrewCalendar::handleGetExtendedYear() {
    int32_t year;
    if (newerField(UCAL_EXTENDED_YEAR, UCAL_YEAR) == UCAL_EXTENDED_YEAR) {
        year = internalGet(UCAL_EXTENDED_YEAR, 1); 
    } else {
        year = internalGet(UCAL_YEAR, 1); 
    }
    return year;
}





int32_t HebrewCalendar::handleComputeMonthStart(int32_t eyear, int32_t month, UBool ) const {
    UErrorCode status = U_ZERO_ERROR;
    
    
    
    
    
    
    while (month < 0) {
        month += monthsInYear(--eyear);
    }
    
    while (month > 12) {
        month -= monthsInYear(eyear++);
    }

    int32_t day = startOfYear(eyear, status);

    if(U_FAILURE(status)) {
        return 0;
    }

    if (month != 0) {
        if (isLeapYear(eyear)) {
            day += LEAP_MONTH_START[month][yearType(eyear)];
        } else {
            day += MONTH_START[month][yearType(eyear)];
        }
    }

    return (int) (day + 347997);
}

UBool
HebrewCalendar::inDaylightTime(UErrorCode& status) const
{
    
    if (U_FAILURE(status) || !getTimeZone().useDaylightTime()) 
        return FALSE;

    
    ((HebrewCalendar*)this)->complete(status); 

    return (UBool)(U_SUCCESS(status) ? (internalGet(UCAL_DST_OFFSET) != 0) : FALSE);
}


const UDate     HebrewCalendar::fgSystemDefaultCentury        = DBL_MIN;
const int32_t   HebrewCalendar::fgSystemDefaultCenturyYear    = -1;

UDate           HebrewCalendar::fgSystemDefaultCenturyStart       = DBL_MIN;
int32_t         HebrewCalendar::fgSystemDefaultCenturyStartYear   = -1;


UBool HebrewCalendar::haveDefaultCentury() const
{
    return TRUE;
}

UDate HebrewCalendar::defaultCenturyStart() const
{
    return internalGetDefaultCenturyStart();
}

int32_t HebrewCalendar::defaultCenturyStartYear() const
{
    return internalGetDefaultCenturyStartYear();
}

UDate
HebrewCalendar::internalGetDefaultCenturyStart() const
{
    
    UBool needsUpdate;
    UMTX_CHECK(NULL, (fgSystemDefaultCenturyStart == fgSystemDefaultCentury), needsUpdate);

    if (needsUpdate) {
        initializeSystemDefaultCentury();
    }

    
    

    return fgSystemDefaultCenturyStart;
}

int32_t
HebrewCalendar::internalGetDefaultCenturyStartYear() const
{
    
    UBool needsUpdate;
    UMTX_CHECK(NULL, (fgSystemDefaultCenturyStart == fgSystemDefaultCentury), needsUpdate);

    if (needsUpdate) {
        initializeSystemDefaultCentury();
    }

    
    

    return fgSystemDefaultCenturyStartYear;
}

void
HebrewCalendar::initializeSystemDefaultCentury()
{
    
    
    
    UErrorCode status = U_ZERO_ERROR;
    HebrewCalendar calendar(Locale("@calendar=hebrew"),status);
    if (U_SUCCESS(status))
    {
        calendar.setTime(Calendar::getNow(), status);
        calendar.add(UCAL_YEAR, -80, status);
        UDate    newStart =  calendar.getTime(status);
        int32_t  newYear  =  calendar.get(UCAL_YEAR, status);
        umtx_lock(NULL);
        if (fgSystemDefaultCenturyStart == fgSystemDefaultCentury) {
            fgSystemDefaultCenturyStartYear = newYear;
            fgSystemDefaultCenturyStart = newStart;
        }
        umtx_unlock(NULL);
    }
    
    
}

UOBJECT_DEFINE_RTTI_IMPLEMENTATION(HebrewCalendar)

U_NAMESPACE_END

#endif 

