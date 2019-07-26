






















#include "utypeinfo.h"  

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/simpletz.h"
#include "unicode/gregocal.h"
#include "unicode/smpdtfmt.h"

#include "gregoimp.h"

U_NAMESPACE_BEGIN

UOBJECT_DEFINE_RTTI_IMPLEMENTATION(SimpleTimeZone)











const int8_t SimpleTimeZone::STATICMONTHLENGTH[] = {31,29,31,30,31,30,31,31,30,31,30,31};

static const UChar DST_STR[] = {0x0028,0x0044,0x0053,0x0054,0x0029,0}; 
static const UChar STD_STR[] = {0x0028,0x0053,0x0054,0x0044,0x0029,0}; 







SimpleTimeZone::SimpleTimeZone(int32_t rawOffsetGMT, const UnicodeString& ID)
:   BasicTimeZone(ID),
    startMonth(0),
    startDay(0),
    startDayOfWeek(0),
    startTime(0),
    startTimeMode(WALL_TIME),
    endTimeMode(WALL_TIME),
    endMonth(0),
    endDay(0),
    endDayOfWeek(0),
    endTime(0),
    startYear(0),
    rawOffset(rawOffsetGMT),
    useDaylight(FALSE),
    startMode(DOM_MODE),
    endMode(DOM_MODE),
    dstSavings(U_MILLIS_PER_HOUR)
{
    clearTransitionRules();
}



SimpleTimeZone::SimpleTimeZone(int32_t rawOffsetGMT, const UnicodeString& ID,
    int8_t savingsStartMonth, int8_t savingsStartDay,
    int8_t savingsStartDayOfWeek, int32_t savingsStartTime,
    int8_t savingsEndMonth, int8_t savingsEndDay,
    int8_t savingsEndDayOfWeek, int32_t savingsEndTime,
    UErrorCode& status)
:   BasicTimeZone(ID)
{
    clearTransitionRules();
    construct(rawOffsetGMT,
              savingsStartMonth, savingsStartDay, savingsStartDayOfWeek,
              savingsStartTime, WALL_TIME,
              savingsEndMonth, savingsEndDay, savingsEndDayOfWeek,
              savingsEndTime, WALL_TIME,
              U_MILLIS_PER_HOUR, status);
}



SimpleTimeZone::SimpleTimeZone(int32_t rawOffsetGMT, const UnicodeString& ID,
    int8_t savingsStartMonth, int8_t savingsStartDay,
    int8_t savingsStartDayOfWeek, int32_t savingsStartTime,
    int8_t savingsEndMonth, int8_t savingsEndDay,
    int8_t savingsEndDayOfWeek, int32_t savingsEndTime,
    int32_t savingsDST, UErrorCode& status)
:   BasicTimeZone(ID)
{
    clearTransitionRules();
    construct(rawOffsetGMT,
              savingsStartMonth, savingsStartDay, savingsStartDayOfWeek,
              savingsStartTime, WALL_TIME,
              savingsEndMonth, savingsEndDay, savingsEndDayOfWeek,
              savingsEndTime, WALL_TIME,
              savingsDST, status);
}



SimpleTimeZone::SimpleTimeZone(int32_t rawOffsetGMT, const UnicodeString& ID,
    int8_t savingsStartMonth, int8_t savingsStartDay,
    int8_t savingsStartDayOfWeek, int32_t savingsStartTime,
    TimeMode savingsStartTimeMode,
    int8_t savingsEndMonth, int8_t savingsEndDay,
    int8_t savingsEndDayOfWeek, int32_t savingsEndTime,
    TimeMode savingsEndTimeMode,
    int32_t savingsDST, UErrorCode& status)
:   BasicTimeZone(ID)
{
    clearTransitionRules();
    construct(rawOffsetGMT,
              savingsStartMonth, savingsStartDay, savingsStartDayOfWeek,
              savingsStartTime, savingsStartTimeMode,
              savingsEndMonth, savingsEndDay, savingsEndDayOfWeek,
              savingsEndTime, savingsEndTimeMode,
              savingsDST, status);
}




void SimpleTimeZone::construct(int32_t rawOffsetGMT,
                               int8_t savingsStartMonth,
                               int8_t savingsStartDay,
                               int8_t savingsStartDayOfWeek,
                               int32_t savingsStartTime,
                               TimeMode savingsStartTimeMode,
                               int8_t savingsEndMonth,
                               int8_t savingsEndDay,
                               int8_t savingsEndDayOfWeek,
                               int32_t savingsEndTime,
                               TimeMode savingsEndTimeMode,
                               int32_t savingsDST,
                               UErrorCode& status)
{
    this->rawOffset      = rawOffsetGMT;
    this->startMonth     = savingsStartMonth;
    this->startDay       = savingsStartDay;
    this->startDayOfWeek = savingsStartDayOfWeek;
    this->startTime      = savingsStartTime;
    this->startTimeMode  = savingsStartTimeMode;
    this->endMonth       = savingsEndMonth;
    this->endDay         = savingsEndDay;
    this->endDayOfWeek   = savingsEndDayOfWeek;
    this->endTime        = savingsEndTime;
    this->endTimeMode    = savingsEndTimeMode;
    this->dstSavings     = savingsDST;
    this->startYear      = 0;
    this->startMode      = DOM_MODE;
    this->endMode        = DOM_MODE;

    decodeRules(status);

    if (savingsDST <= 0) {
        status = U_ILLEGAL_ARGUMENT_ERROR;
    }
}



SimpleTimeZone::~SimpleTimeZone()
{
    deleteTransitionRules();
}




SimpleTimeZone::SimpleTimeZone(const SimpleTimeZone &source)
:   BasicTimeZone(source)
{
    *this = source;
}




SimpleTimeZone &
SimpleTimeZone::operator=(const SimpleTimeZone &right)
{
    if (this != &right)
    {
        TimeZone::operator=(right);
        rawOffset      = right.rawOffset;
        startMonth     = right.startMonth;
        startDay       = right.startDay;
        startDayOfWeek = right.startDayOfWeek;
        startTime      = right.startTime;
        startTimeMode  = right.startTimeMode;
        startMode      = right.startMode;
        endMonth       = right.endMonth;
        endDay         = right.endDay;
        endDayOfWeek   = right.endDayOfWeek;
        endTime        = right.endTime;
        endTimeMode    = right.endTimeMode;
        endMode        = right.endMode;
        startYear      = right.startYear;
        dstSavings     = right.dstSavings;
        useDaylight    = right.useDaylight;
        clearTransitionRules();
    }
    return *this;
}



UBool
SimpleTimeZone::operator==(const TimeZone& that) const
{
    return ((this == &that) ||
            (typeid(*this) == typeid(that) &&
            TimeZone::operator==(that) &&
            hasSameRules(that)));
}




TimeZone*
SimpleTimeZone::clone() const
{
    return new SimpleTimeZone(*this);
}










void
SimpleTimeZone::setStartYear(int32_t year)
{
    startYear = year;
    transitionRulesInitialized = FALSE;
}









































 
void
SimpleTimeZone::setStartRule(int32_t month, int32_t dayOfWeekInMonth, int32_t dayOfWeek,
                             int32_t time, TimeMode mode, UErrorCode& status)
{
    startMonth     = (int8_t)month;
    startDay       = (int8_t)dayOfWeekInMonth;
    startDayOfWeek = (int8_t)dayOfWeek;
    startTime      = time;
    startTimeMode  = mode;
    decodeStartRule(status);
    transitionRulesInitialized = FALSE;
}



void 
SimpleTimeZone::setStartRule(int32_t month, int32_t dayOfMonth, 
                             int32_t time, TimeMode mode, UErrorCode& status) 
{
    setStartRule(month, dayOfMonth, 0, time, mode, status);
}



void 
SimpleTimeZone::setStartRule(int32_t month, int32_t dayOfMonth, int32_t dayOfWeek, 
                             int32_t time, TimeMode mode, UBool after, UErrorCode& status)
{
    setStartRule(month, after ? dayOfMonth : -dayOfMonth,
                 -dayOfWeek, time, mode, status);
}





















void
SimpleTimeZone::setEndRule(int32_t month, int32_t dayOfWeekInMonth, int32_t dayOfWeek,
                           int32_t time, TimeMode mode, UErrorCode& status)
{
    endMonth     = (int8_t)month;
    endDay       = (int8_t)dayOfWeekInMonth;
    endDayOfWeek = (int8_t)dayOfWeek;
    endTime      = time;
    endTimeMode  = mode;
    decodeEndRule(status);
    transitionRulesInitialized = FALSE;
}



void 
SimpleTimeZone::setEndRule(int32_t month, int32_t dayOfMonth, 
                           int32_t time, TimeMode mode, UErrorCode& status)
{
    setEndRule(month, dayOfMonth, 0, time, mode, status);
}



void 
SimpleTimeZone::setEndRule(int32_t month, int32_t dayOfMonth, int32_t dayOfWeek, 
                           int32_t time, TimeMode mode, UBool after, UErrorCode& status)
{
    setEndRule(month, after ? dayOfMonth : -dayOfMonth,
               -dayOfWeek, time, mode, status);
}



int32_t
SimpleTimeZone::getOffset(uint8_t era, int32_t year, int32_t month, int32_t day,
                          uint8_t dayOfWeek, int32_t millis, UErrorCode& status) const
{
    
    
    
    
    
    
    
    if(month < UCAL_JANUARY || month > UCAL_DECEMBER) {
        status = U_ILLEGAL_ARGUMENT_ERROR;
        return 0;
    }

    return getOffset(era, year, month, day, dayOfWeek, millis, Grego::monthLength(year, month), status);
}

int32_t 
SimpleTimeZone::getOffset(uint8_t era, int32_t year, int32_t month, int32_t day,
                          uint8_t dayOfWeek, int32_t millis, 
                          int32_t , UErrorCode& status) const
{
    
    
    
    
    
    
    
    if (month < UCAL_JANUARY
        || month > UCAL_DECEMBER) {
        status = U_ILLEGAL_ARGUMENT_ERROR;
        return -1;
    }

    
    
    
    return getOffset(era, year, month, day, dayOfWeek, millis,
                     Grego::monthLength(year, month),
                     Grego::previousMonthLength(year, month),
                     status);
}

int32_t 
SimpleTimeZone::getOffset(uint8_t era, int32_t year, int32_t month, int32_t day,
                          uint8_t dayOfWeek, int32_t millis, 
                          int32_t monthLength, int32_t prevMonthLength,
                          UErrorCode& status) const
{
    if(U_FAILURE(status)) return 0;

    if ((era != GregorianCalendar::AD && era != GregorianCalendar::BC)
        || month < UCAL_JANUARY
        || month > UCAL_DECEMBER
        || day < 1
        || day > monthLength
        || dayOfWeek < UCAL_SUNDAY
        || dayOfWeek > UCAL_SATURDAY
        || millis < 0
        || millis >= U_MILLIS_PER_DAY
        || monthLength < 28
        || monthLength > 31
        || prevMonthLength < 28
        || prevMonthLength > 31) {
        status = U_ILLEGAL_ARGUMENT_ERROR;
        return -1;
    }

    int32_t result = rawOffset;

    
    if(!useDaylight || year < startYear || era != GregorianCalendar::AD) 
        return result;

    
    
    UBool southern = (startMonth > endMonth);

    
    
    int32_t startCompare = compareToRule((int8_t)month, (int8_t)monthLength, (int8_t)prevMonthLength,
                                         (int8_t)day, (int8_t)dayOfWeek, millis,
                                         startTimeMode == UTC_TIME ? -rawOffset : 0,
                                         startMode, (int8_t)startMonth, (int8_t)startDayOfWeek,
                                         (int8_t)startDay, startTime);
    int32_t endCompare = 0;

    





    if(southern != (startCompare >= 0)) {
        endCompare = compareToRule((int8_t)month, (int8_t)monthLength, (int8_t)prevMonthLength,
                                   (int8_t)day, (int8_t)dayOfWeek, millis,
                                   endTimeMode == WALL_TIME ? dstSavings :
                                    (endTimeMode == UTC_TIME ? -rawOffset : 0),
                                   endMode, (int8_t)endMonth, (int8_t)endDayOfWeek,
                                   (int8_t)endDay, endTime);
    }

    
    
    
    
    if ((!southern && (startCompare >= 0 && endCompare < 0)) ||
        (southern && (startCompare >= 0 || endCompare < 0)))
        result += dstSavings;

    return result;
}

void
SimpleTimeZone::getOffsetFromLocal(UDate date, int32_t nonExistingTimeOpt, int32_t duplicatedTimeOpt,
                                   int32_t& rawOffsetGMT, int32_t& savingsDST, UErrorCode& status)  {
    if (U_FAILURE(status)) {
        return;
    }

    rawOffsetGMT = getRawOffset();
    int32_t year, month, dom, dow;
    double day = uprv_floor(date / U_MILLIS_PER_DAY);
    int32_t millis = (int32_t) (date - day * U_MILLIS_PER_DAY);

    Grego::dayToFields(day, year, month, dom, dow);

    savingsDST = getOffset(GregorianCalendar::AD, year, month, dom,
                          (uint8_t) dow, millis,
                          Grego::monthLength(year, month),
                          status) - rawOffsetGMT;
    if (U_FAILURE(status)) {
        return;
    }

    UBool recalc = FALSE;

    
    if (savingsDST > 0) {
        if ((nonExistingTimeOpt & kStdDstMask) == kStandard
            || ((nonExistingTimeOpt & kStdDstMask) != kDaylight && (nonExistingTimeOpt & kFormerLatterMask) != kLatter)) {
            date -= getDSTSavings();
            recalc = TRUE;
        }
    } else {
        if ((duplicatedTimeOpt & kStdDstMask) == kDaylight
                || ((duplicatedTimeOpt & kStdDstMask) != kStandard && (duplicatedTimeOpt & kFormerLatterMask) == kFormer)) {
            date -= getDSTSavings();
            recalc = TRUE;
        }
    }
    if (recalc) {
        day = uprv_floor(date / U_MILLIS_PER_DAY);
        millis = (int32_t) (date - day * U_MILLIS_PER_DAY);
        Grego::dayToFields(day, year, month, dom, dow);
        savingsDST = getOffset(GregorianCalendar::AD, year, month, dom,
                          (uint8_t) dow, millis,
                          Grego::monthLength(year, month),
                          status) - rawOffsetGMT;
    }
}












int32_t 
SimpleTimeZone::compareToRule(int8_t month, int8_t monthLen, int8_t prevMonthLen,
                              int8_t dayOfMonth,
                              int8_t dayOfWeek, int32_t millis, int32_t millisDelta,
                              EMode ruleMode, int8_t ruleMonth, int8_t ruleDayOfWeek,
                              int8_t ruleDay, int32_t ruleMillis)
{
    
    millis += millisDelta;
    while (millis >= U_MILLIS_PER_DAY) {
        millis -= U_MILLIS_PER_DAY;
        ++dayOfMonth;
        dayOfWeek = (int8_t)(1 + (dayOfWeek % 7)); 
        if (dayOfMonth > monthLen) {
            dayOfMonth = 1;
            



            ++month;
        }
    }
    while (millis < 0) {
        millis += U_MILLIS_PER_DAY;
        --dayOfMonth;
        dayOfWeek = (int8_t)(1 + ((dayOfWeek+5) % 7)); 
        if (dayOfMonth < 1) {
            dayOfMonth = prevMonthLen;
            --month;
        }
    }

    
    
    if (month < ruleMonth) return -1;
    else if (month > ruleMonth) return 1;

    
    int32_t ruleDayOfMonth = 0;

    
    if (ruleDay > monthLen) {
        ruleDay = monthLen;
    }

    switch (ruleMode)
    {
    
    case DOM_MODE:
        ruleDayOfMonth = ruleDay;
        break;

    
    case DOW_IN_MONTH_MODE:
        
        
        
        
        if (ruleDay > 0)
            ruleDayOfMonth = 1 + (ruleDay - 1) * 7 +
                (7 + ruleDayOfWeek - (dayOfWeek - dayOfMonth + 1)) % 7;
        
        
        
        else
        {
            
            
            
            ruleDayOfMonth = monthLen + (ruleDay + 1) * 7 -
                (7 + (dayOfWeek + monthLen - dayOfMonth) - ruleDayOfWeek) % 7;
        }
        break;

    case DOW_GE_DOM_MODE:
        ruleDayOfMonth = ruleDay +
            (49 + ruleDayOfWeek - ruleDay - dayOfWeek + dayOfMonth) % 7;
        break;

    case DOW_LE_DOM_MODE:
        ruleDayOfMonth = ruleDay -
            (49 - ruleDayOfWeek + ruleDay + dayOfWeek - dayOfMonth) % 7;
        
        
        break;
    }

    
    if (dayOfMonth < ruleDayOfMonth) return -1;
    else if (dayOfMonth > ruleDayOfMonth) return 1;

    
    if (millis < ruleMillis) return -1;
    else if (millis > ruleMillis) return 1;
    else return 0;
}



int32_t
SimpleTimeZone::getRawOffset() const
{
    return rawOffset;
}



void
SimpleTimeZone::setRawOffset(int32_t offsetMillis)
{
    rawOffset = offsetMillis;
    transitionRulesInitialized = FALSE;
}



void 
SimpleTimeZone::setDSTSavings(int32_t millisSavedDuringDST, UErrorCode& status) 
{
    if (millisSavedDuringDST <= 0) {
        status = U_ILLEGAL_ARGUMENT_ERROR;
    }
    else {
        dstSavings = millisSavedDuringDST;
    }
    transitionRulesInitialized = FALSE;
}



int32_t 
SimpleTimeZone::getDSTSavings() const
{
    return dstSavings;
}



UBool
SimpleTimeZone::useDaylightTime() const
{
    return useDaylight;
}







UBool SimpleTimeZone::inDaylightTime(UDate date, UErrorCode& status) const
{
    
    
    
    if (U_FAILURE(status)) return FALSE;
    GregorianCalendar *gc = new GregorianCalendar(*this, status);
    
    if (gc == 0) {
        status = U_MEMORY_ALLOCATION_ERROR;
        return FALSE;
    }
    gc->setTime(date, status);
    UBool result = gc->inDaylightTime(status);
    delete gc;
    return result;
}








UBool 
SimpleTimeZone::hasSameRules(const TimeZone& other) const
{
    if (this == &other) return TRUE;
    if (typeid(*this) != typeid(other)) return FALSE;
    SimpleTimeZone *that = (SimpleTimeZone*)&other;
    return rawOffset     == that->rawOffset &&
        useDaylight     == that->useDaylight &&
        (!useDaylight
         
         || (dstSavings     == that->dstSavings &&
             startMode      == that->startMode &&
             startMonth     == that->startMonth &&
             startDay       == that->startDay &&
             startDayOfWeek == that->startDayOfWeek &&
             startTime      == that->startTime &&
             startTimeMode  == that->startTimeMode &&
             endMode        == that->endMode &&
             endMonth       == that->endMonth &&
             endDay         == that->endDay &&
             endDayOfWeek   == that->endDayOfWeek &&
             endTime        == that->endTime &&
             endTimeMode    == that->endTimeMode &&
             startYear      == that->startYear));
}


































































void 
SimpleTimeZone::decodeRules(UErrorCode& status)
{
    decodeStartRule(status);
    decodeEndRule(status);
}

























void 
SimpleTimeZone::decodeStartRule(UErrorCode& status) 
{
    if(U_FAILURE(status)) return;

    useDaylight = (UBool)((startDay != 0) && (endDay != 0) ? TRUE : FALSE);
    if (useDaylight && dstSavings == 0) {
        dstSavings = U_MILLIS_PER_HOUR;
    }
    if (startDay != 0) {
        if (startMonth < UCAL_JANUARY || startMonth > UCAL_DECEMBER) {
            status = U_ILLEGAL_ARGUMENT_ERROR;
            return;
        }
        if (startTime < 0 || startTime > U_MILLIS_PER_DAY ||
            startTimeMode < WALL_TIME || startTimeMode > UTC_TIME) {
            status = U_ILLEGAL_ARGUMENT_ERROR;
            return;
        }
        if (startDayOfWeek == 0) {
            startMode = DOM_MODE;
        } else {
            if (startDayOfWeek > 0) {
                startMode = DOW_IN_MONTH_MODE;
            } else {
                startDayOfWeek = (int8_t)-startDayOfWeek;
                if (startDay > 0) {
                    startMode = DOW_GE_DOM_MODE;
                } else {
                    startDay = (int8_t)-startDay;
                    startMode = DOW_LE_DOM_MODE;
                }
            }
            if (startDayOfWeek > UCAL_SATURDAY) {
                status = U_ILLEGAL_ARGUMENT_ERROR;
                return;
            }
        }
        if (startMode == DOW_IN_MONTH_MODE) {
            if (startDay < -5 || startDay > 5) {
                status = U_ILLEGAL_ARGUMENT_ERROR;
                return;
            }
        } else if (startDay<1 || startDay > STATICMONTHLENGTH[startMonth]) {
            status = U_ILLEGAL_ARGUMENT_ERROR;
            return;
        }
    }
}






void 
SimpleTimeZone::decodeEndRule(UErrorCode& status) 
{
    if(U_FAILURE(status)) return;

    useDaylight = (UBool)((startDay != 0) && (endDay != 0) ? TRUE : FALSE);
    if (useDaylight && dstSavings == 0) {
        dstSavings = U_MILLIS_PER_HOUR;
    }
    if (endDay != 0) {
        if (endMonth < UCAL_JANUARY || endMonth > UCAL_DECEMBER) {
            status = U_ILLEGAL_ARGUMENT_ERROR;
            return;
        }
        if (endTime < 0 || endTime > U_MILLIS_PER_DAY ||
            endTimeMode < WALL_TIME || endTimeMode > UTC_TIME) {
            status = U_ILLEGAL_ARGUMENT_ERROR;
            return;
        }
        if (endDayOfWeek == 0) {
            endMode = DOM_MODE;
        } else {
            if (endDayOfWeek > 0) {
                endMode = DOW_IN_MONTH_MODE;
            } else {
                endDayOfWeek = (int8_t)-endDayOfWeek;
                if (endDay > 0) {
                    endMode = DOW_GE_DOM_MODE;
                } else {
                    endDay = (int8_t)-endDay;
                    endMode = DOW_LE_DOM_MODE;
                }
            }
            if (endDayOfWeek > UCAL_SATURDAY) {
                status = U_ILLEGAL_ARGUMENT_ERROR;
                return;
            }
        }
        if (endMode == DOW_IN_MONTH_MODE) {
            if (endDay < -5 || endDay > 5) {
                status = U_ILLEGAL_ARGUMENT_ERROR;
                return;
            }
        } else if (endDay<1 || endDay > STATICMONTHLENGTH[endMonth]) {
            status = U_ILLEGAL_ARGUMENT_ERROR;
            return;
        }
    }
}

UBool
SimpleTimeZone::getNextTransition(UDate base, UBool inclusive, TimeZoneTransition& result)  {
    if (!useDaylight) {
        return FALSE;
    }

    UErrorCode status = U_ZERO_ERROR;
    initTransitionRules(status);
    if (U_FAILURE(status)) {
        return FALSE;
    }

    UDate firstTransitionTime = firstTransition->getTime();
    if (base < firstTransitionTime || (inclusive && base == firstTransitionTime)) {
        result = *firstTransition;
    }
    UDate stdDate, dstDate;
    UBool stdAvail = stdRule->getNextStart(base, dstRule->getRawOffset(), dstRule->getDSTSavings(), inclusive, stdDate);
    UBool dstAvail = dstRule->getNextStart(base, stdRule->getRawOffset(), stdRule->getDSTSavings(), inclusive, dstDate);
    if (stdAvail && (!dstAvail || stdDate < dstDate)) {
        result.setTime(stdDate);
        result.setFrom((const TimeZoneRule&)*dstRule);
        result.setTo((const TimeZoneRule&)*stdRule);
        return TRUE;
    }
    if (dstAvail && (!stdAvail || dstDate < stdDate)) {
        result.setTime(dstDate);
        result.setFrom((const TimeZoneRule&)*stdRule);
        result.setTo((const TimeZoneRule&)*dstRule);
        return TRUE;
    }
    return FALSE;
}

UBool
SimpleTimeZone::getPreviousTransition(UDate base, UBool inclusive, TimeZoneTransition& result)  {
    if (!useDaylight) {
        return FALSE;
    }

    UErrorCode status = U_ZERO_ERROR;
    initTransitionRules(status);
    if (U_FAILURE(status)) {
        return FALSE;
    }

    UDate firstTransitionTime = firstTransition->getTime();
    if (base < firstTransitionTime || (!inclusive && base == firstTransitionTime)) {
        return FALSE;
    }
    UDate stdDate, dstDate;
    UBool stdAvail = stdRule->getPreviousStart(base, dstRule->getRawOffset(), dstRule->getDSTSavings(), inclusive, stdDate);
    UBool dstAvail = dstRule->getPreviousStart(base, stdRule->getRawOffset(), stdRule->getDSTSavings(), inclusive, dstDate);
    if (stdAvail && (!dstAvail || stdDate > dstDate)) {
        result.setTime(stdDate);
        result.setFrom((const TimeZoneRule&)*dstRule);
        result.setTo((const TimeZoneRule&)*stdRule);
        return TRUE;
    }
    if (dstAvail && (!stdAvail || dstDate > stdDate)) {
        result.setTime(dstDate);
        result.setFrom((const TimeZoneRule&)*stdRule);
        result.setTo((const TimeZoneRule&)*dstRule);
        return TRUE;
    }
    return FALSE;
}

void
SimpleTimeZone::clearTransitionRules(void) {
    initialRule = NULL;
    firstTransition = NULL;
    stdRule = NULL;
    dstRule = NULL;
    transitionRulesInitialized = FALSE;
}

void
SimpleTimeZone::deleteTransitionRules(void) {
    if (initialRule != NULL) {
        delete initialRule;
    }
    if (firstTransition != NULL) {
        delete firstTransition;
    }
    if (stdRule != NULL) {
        delete stdRule;
    }
    if (dstRule != NULL) {
        delete dstRule;
    }
    clearTransitionRules();
 }

void
SimpleTimeZone::initTransitionRules(UErrorCode& status) {
    if (U_FAILURE(status)) {
        return;
    }
    if (transitionRulesInitialized) {
        return;
    }
    deleteTransitionRules();
    UnicodeString tzid;
    getID(tzid);

    if (useDaylight) {
        DateTimeRule* dtRule;
        DateTimeRule::TimeRuleType timeRuleType;
        UDate firstStdStart, firstDstStart;

        
        timeRuleType = (startTimeMode == STANDARD_TIME) ? DateTimeRule::STANDARD_TIME :
            ((startTimeMode == UTC_TIME) ? DateTimeRule::UTC_TIME : DateTimeRule::WALL_TIME);
        switch (startMode) {
        case DOM_MODE:
            dtRule = new DateTimeRule(startMonth, startDay, startTime, timeRuleType);
            break;
        case DOW_IN_MONTH_MODE:
            dtRule = new DateTimeRule(startMonth, startDay, startDayOfWeek, startTime, timeRuleType);
            break;
        case DOW_GE_DOM_MODE:
            dtRule = new DateTimeRule(startMonth, startDay, startDayOfWeek, true, startTime, timeRuleType);
            break;
        case DOW_LE_DOM_MODE:
            dtRule = new DateTimeRule(startMonth, startDay, startDayOfWeek, false, startTime, timeRuleType);
            break;
        default:
            status = U_INVALID_STATE_ERROR;
            return;
        }
        
        if (dtRule == NULL) {
        	status = U_MEMORY_ALLOCATION_ERROR;
        	return;
        }
        
        dstRule = new AnnualTimeZoneRule(tzid+UnicodeString(DST_STR), getRawOffset(), getDSTSavings(),
            dtRule, startYear, AnnualTimeZoneRule::MAX_YEAR);
        
        
        if (dstRule == NULL) {
        	status = U_MEMORY_ALLOCATION_ERROR;
        	deleteTransitionRules();
        	return;
        }
 
        
        dstRule->getFirstStart(getRawOffset(), 0, firstDstStart);

        
        timeRuleType = (endTimeMode == STANDARD_TIME) ? DateTimeRule::STANDARD_TIME :
            ((endTimeMode == UTC_TIME) ? DateTimeRule::UTC_TIME : DateTimeRule::WALL_TIME);
        switch (endMode) {
        case DOM_MODE:
            dtRule = new DateTimeRule(endMonth, endDay, endTime, timeRuleType);
            break;
        case DOW_IN_MONTH_MODE:
            dtRule = new DateTimeRule(endMonth, endDay, endDayOfWeek, endTime, timeRuleType);
            break;
        case DOW_GE_DOM_MODE:
            dtRule = new DateTimeRule(endMonth, endDay, endDayOfWeek, true, endTime, timeRuleType);
            break;
        case DOW_LE_DOM_MODE:
            dtRule = new DateTimeRule(endMonth, endDay, endDayOfWeek, false, endTime, timeRuleType);
            break;
        }
        
        
        if (dtRule == NULL) {
        	status = U_MEMORY_ALLOCATION_ERROR;
        	deleteTransitionRules();
        	return;
        }
        
        stdRule = new AnnualTimeZoneRule(tzid+UnicodeString(STD_STR), getRawOffset(), 0,
            dtRule, startYear, AnnualTimeZoneRule::MAX_YEAR);
        
        
        if (stdRule == NULL) {
        	status = U_MEMORY_ALLOCATION_ERROR;
        	deleteTransitionRules();
        	return;
        }

        
        stdRule->getFirstStart(getRawOffset(), dstRule->getDSTSavings(), firstStdStart);

        
        if (firstStdStart < firstDstStart) {
            initialRule = new InitialTimeZoneRule(tzid+UnicodeString(DST_STR), getRawOffset(), dstRule->getDSTSavings());
            firstTransition = new TimeZoneTransition(firstStdStart, *initialRule, *stdRule);
        } else {
            initialRule = new InitialTimeZoneRule(tzid+UnicodeString(STD_STR), getRawOffset(), 0);
            firstTransition = new TimeZoneTransition(firstDstStart, *initialRule, *dstRule);
        }
        
        if (initialRule == NULL || firstTransition == NULL) {
        	status = U_MEMORY_ALLOCATION_ERROR;
        	deleteTransitionRules();
        	return;
        }
        
    } else {
        
        initialRule = new InitialTimeZoneRule(tzid, getRawOffset(), 0);
        
        if (initialRule == NULL) {
        	status = U_MEMORY_ALLOCATION_ERROR;
        	deleteTransitionRules();
        	return;
        }
    }

    transitionRulesInitialized = true;
}

int32_t
SimpleTimeZone::countTransitionRules(UErrorCode& )  {
    return (useDaylight) ? 2 : 0;
}

void
SimpleTimeZone::getTimeZoneRules(const InitialTimeZoneRule*& initial,
                                 const TimeZoneRule* trsrules[],
                                 int32_t& trscount,
                                 UErrorCode& status)  {
    if (U_FAILURE(status)) {
        return;
    }
    initTransitionRules(status);
    if (U_FAILURE(status)) {
        return;
    }
    initial = initialRule;
    int32_t cnt = 0;
    if (stdRule != NULL) {
        if (cnt < trscount) {
            trsrules[cnt++] = stdRule;
        }
        if (cnt < trscount) {
            trsrules[cnt++] = dstRule;
        }
    }
    trscount = cnt;
}


U_NAMESPACE_END

#endif 


