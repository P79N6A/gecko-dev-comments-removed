












#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "buddhcal.h"
#include "unicode/gregocal.h"
#include "umutex.h"
#include <float.h>

U_NAMESPACE_BEGIN

UOBJECT_DEFINE_RTTI_IMPLEMENTATION(BuddhistCalendar)



static const int32_t kBuddhistEraStart = -543;  

static const int32_t kGregorianEpoch = 1970;    

BuddhistCalendar::BuddhistCalendar(const Locale& aLocale, UErrorCode& success)
:   GregorianCalendar(aLocale, success)
{
    setTimeInMillis(getNow(), success); 
}

BuddhistCalendar::~BuddhistCalendar()
{
}

BuddhistCalendar::BuddhistCalendar(const BuddhistCalendar& source)
: GregorianCalendar(source)
{
}

BuddhistCalendar& BuddhistCalendar::operator= ( const BuddhistCalendar& right)
{
    GregorianCalendar::operator=(right);
    return *this;
}

Calendar* BuddhistCalendar::clone(void) const
{
    return new BuddhistCalendar(*this);
}

const char *BuddhistCalendar::getType() const
{
    return "buddhist";
}

int32_t BuddhistCalendar::handleGetExtendedYear()
{
    
    
    int32_t year;
    if (newerField(UCAL_EXTENDED_YEAR, UCAL_YEAR) == UCAL_EXTENDED_YEAR) {
        year = internalGet(UCAL_EXTENDED_YEAR, kGregorianEpoch);
    } else {
        
        year = internalGet(UCAL_YEAR, kGregorianEpoch - kBuddhistEraStart)
                + kBuddhistEraStart;
    }
    return year;
}

int32_t BuddhistCalendar::handleComputeMonthStart(int32_t eyear, int32_t month,

                                                  UBool useMonth) const
{
    return GregorianCalendar::handleComputeMonthStart(eyear, month, useMonth);
}

void BuddhistCalendar::handleComputeFields(int32_t julianDay, UErrorCode& status)
{
    GregorianCalendar::handleComputeFields(julianDay, status);
    int32_t y = internalGet(UCAL_EXTENDED_YEAR) - kBuddhistEraStart;
    internalSet(UCAL_ERA, 0);
    internalSet(UCAL_YEAR, y);
}

int32_t BuddhistCalendar::handleGetLimit(UCalendarDateFields field, ELimitType limitType) const
{
    if(field == UCAL_ERA) {
        return BE;
    } else {
        return GregorianCalendar::handleGetLimit(field,limitType);
    }
}

#if 0
void BuddhistCalendar::timeToFields(UDate theTime, UBool quick, UErrorCode& status)
{
    

    int32_t era = internalGet(UCAL_ERA);
    int32_t year = internalGet(UCAL_YEAR);

    if(era == GregorianCalendar::BC) {
        year = 1-year;
        era = BuddhistCalendar::BE;
    } else if(era == GregorianCalendar::AD) {
        era = BuddhistCalendar::BE;
    } else {
        status = U_INTERNAL_PROGRAM_ERROR;
    }

    year = year - kBuddhistEraStart;

    internalSet(UCAL_ERA, era);
    internalSet(UCAL_YEAR, year);
}
#endif






static UDate     gSystemDefaultCenturyStart       = DBL_MIN;
static int32_t   gSystemDefaultCenturyStartYear   = -1;
static icu::UInitOnce gBCInitOnce;


UBool BuddhistCalendar::haveDefaultCentury() const
{
    return TRUE;
}

static void U_CALLCONV
initializeSystemDefaultCentury()
{
    
    
    
    UErrorCode status = U_ZERO_ERROR;
    BuddhistCalendar calendar(Locale("@calendar=buddhist"),status);
    if (U_SUCCESS(status)) {
        calendar.setTime(Calendar::getNow(), status);
        calendar.add(UCAL_YEAR, -80, status);
        UDate    newStart =  calendar.getTime(status);
        int32_t  newYear  =  calendar.get(UCAL_YEAR, status);
        gSystemDefaultCenturyStartYear = newYear;
        gSystemDefaultCenturyStart = newStart;
    }
    
    
}

UDate BuddhistCalendar::defaultCenturyStart() const
{
    
    umtx_initOnce(gBCInitOnce, &initializeSystemDefaultCentury);
    return gSystemDefaultCenturyStart;
}

int32_t BuddhistCalendar::defaultCenturyStartYear() const
{
    
    umtx_initOnce(gBCInitOnce, &initializeSystemDefaultCentury);
    return gSystemDefaultCenturyStartYear;
}


U_NAMESPACE_END

#endif
