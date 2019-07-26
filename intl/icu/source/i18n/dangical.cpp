









#include "chnsecal.h"
#include "dangical.h"

#if !UCONFIG_NO_FORMATTING

#include "gregoimp.h" 
#include "uassert.h"
#include "ucln_in.h"
#include "umutex.h"
#include "unicode/rbtz.h"
#include "unicode/tzrule.h"


static icu::TimeZone *gDangiCalendarZoneAstroCalc = NULL;
static icu::UInitOnce gDangiCalendarInitOnce = U_INITONCE_INITIALIZER;





static const int32_t DANGI_EPOCH_YEAR = -2332; 

U_CDECL_BEGIN
static UBool calendar_dangi_cleanup(void) {
    if (gDangiCalendarZoneAstroCalc) {
        delete gDangiCalendarZoneAstroCalc;
        gDangiCalendarZoneAstroCalc = NULL;
    }
    gDangiCalendarInitOnce.reset();
    return TRUE;
}
U_CDECL_END

U_NAMESPACE_BEGIN







DangiCalendar::DangiCalendar(const Locale& aLocale, UErrorCode& success)
:   ChineseCalendar(aLocale, DANGI_EPOCH_YEAR, getDangiCalZoneAstroCalc(), success)
{
}

DangiCalendar::DangiCalendar (const DangiCalendar& other) 
: ChineseCalendar(other)
{
}

DangiCalendar::~DangiCalendar()
{
}

Calendar*
DangiCalendar::clone() const
{
    return new DangiCalendar(*this);
}

const char *DangiCalendar::getType() const { 
    return "dangi";
}





























static void U_CALLCONV initDangiCalZoneAstroCalc(void) {
    U_ASSERT(gDangiCalendarZoneAstroCalc == NULL);
    const UDate millis1897[] = { (UDate)((1897 - 1970) * 365 * kOneDay) }; 
    const UDate millis1898[] = { (UDate)((1898 - 1970) * 365 * kOneDay) }; 
    const UDate millis1912[] = { (UDate)((1912 - 1970) * 365 * kOneDay) }; 
    InitialTimeZoneRule* initialTimeZone = new InitialTimeZoneRule(UNICODE_STRING_SIMPLE("GMT+8"), 8*kOneHour, 0);
    TimeZoneRule* rule1897 = new TimeArrayTimeZoneRule(UNICODE_STRING_SIMPLE("Korean 1897"), 7*kOneHour, 0, millis1897, 1, DateTimeRule::STANDARD_TIME);
    TimeZoneRule* rule1898to1911 = new TimeArrayTimeZoneRule(UNICODE_STRING_SIMPLE("Korean 1898-1911"), 8*kOneHour, 0, millis1898, 1, DateTimeRule::STANDARD_TIME);
    TimeZoneRule* ruleFrom1912 = new TimeArrayTimeZoneRule(UNICODE_STRING_SIMPLE("Korean 1912-"), 9*kOneHour, 0, millis1912, 1, DateTimeRule::STANDARD_TIME);
    UErrorCode status = U_ZERO_ERROR;
    RuleBasedTimeZone* dangiCalZoneAstroCalc = new RuleBasedTimeZone(UNICODE_STRING_SIMPLE("KOREA_ZONE"), initialTimeZone); 
    dangiCalZoneAstroCalc->addTransitionRule(rule1897, status); 
    dangiCalZoneAstroCalc->addTransitionRule(rule1898to1911, status);
    dangiCalZoneAstroCalc->addTransitionRule(ruleFrom1912, status);
    dangiCalZoneAstroCalc->complete(status);
    if (U_SUCCESS(status)) {
        gDangiCalendarZoneAstroCalc = dangiCalZoneAstroCalc;
    } else {
        delete dangiCalZoneAstroCalc;
        gDangiCalendarZoneAstroCalc = NULL;
    }
    ucln_i18n_registerCleanup(UCLN_I18N_DANGI_CALENDAR, calendar_dangi_cleanup);
}

const TimeZone* DangiCalendar::getDangiCalZoneAstroCalc(void) const {
    umtx_initOnce(gDangiCalendarInitOnce, &initDangiCalZoneAstroCalc);
    return gDangiCalendarZoneAstroCalc;
}


UOBJECT_DEFINE_RTTI_IMPLEMENTATION(DangiCalendar)

U_NAMESPACE_END

#endif

