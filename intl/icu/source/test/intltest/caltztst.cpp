














#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "caltztst.h"
#include "unicode/smpdtfmt.h"
#include "mutex.h"

DateFormat*         CalendarTimeZoneTest::fgDateFormat = 0;
Calendar*           CalendarTimeZoneTest::fgCalendar   = 0;

UBool CalendarTimeZoneTest::failure(UErrorCode status, const char* msg, UBool possibleDataError)
{
    if (U_FAILURE(status))
    {
        if (possibleDataError) {
            dataerrln(UnicodeString("FAIL: ") + msg + " failed, error " + u_errorName(status));
        } else {
            errcheckln(status, UnicodeString("FAIL: ") + msg + " failed, error " + u_errorName(status));
        }
        return TRUE;
    }
    return FALSE;
}

DateFormat*   CalendarTimeZoneTest::getDateFormat()
{
    DateFormat *theFormat = 0;

    if (fgDateFormat != 0) 
    {
        Mutex lock;

        if (fgDateFormat != 0) 
        {
            theFormat = fgDateFormat;
            fgDateFormat = 0; 
        }
    }

    if(theFormat == 0) 
    {
        UErrorCode status = U_ZERO_ERROR;
        theFormat = new SimpleDateFormat(UnicodeString("EEE MMM dd HH:mm:ss zzz yyyy"), status);
        if (U_FAILURE(status))
        {
            delete theFormat;
            theFormat = 0;
            dataerrln("FAIL: Could not create SimpleDateFormat - %s", u_errorName(status));
        }
    }

    return theFormat;
}

void CalendarTimeZoneTest::releaseDateFormat(DateFormat *adopt)
{
    if(fgDateFormat == 0) 
    {
        Mutex lock;

        if(fgDateFormat == 0)
        {
            fgDateFormat = adopt;
            adopt = 0;
        }
    }
    else {
        delete adopt;
    }
}

Calendar*  CalendarTimeZoneTest::getCalendar()
{
    Calendar *theCalendar = 0;

    if (fgCalendar != 0) 
    {
        Mutex lock;

        if (fgCalendar != 0) 
        {
            theCalendar = fgCalendar;
            fgCalendar = 0; 
        }
    }

    if(theCalendar == 0) 
    {
        UErrorCode status = U_ZERO_ERROR;
        theCalendar = Calendar::createInstance(status);
        if (U_FAILURE(status))
        {
            delete theCalendar;
            theCalendar = 0;
            dataerrln("FAIL: Calendar::createInstance failed: %s", u_errorName(status));
        }
    }
    return theCalendar;
}

void CalendarTimeZoneTest::releaseCalendar(Calendar* adopt)
{
    if(fgCalendar == 0) 
    {
        Mutex lock;

        if(fgCalendar == 0)
        {
            fgCalendar = adopt;
            adopt = 0;
        }
    }
    else
    {
        delete adopt;
    }
}



UnicodeString
CalendarTimeZoneTest::dateToString(UDate d)
{
    UnicodeString str;
    return dateToString(d, str);
}

UnicodeString&
CalendarTimeZoneTest::dateToString(UDate d, UnicodeString& str)
{
    str.remove();
    DateFormat* format = getDateFormat();
    if (format == 0)
    {
        str += "DATE_FORMAT_FAILURE";
        return str;
    }
    format->format(d, str);
    releaseDateFormat(format);
    return str;
}

UnicodeString&
CalendarTimeZoneTest::dateToString(UDate d, UnicodeString& str,
                                   const TimeZone& tz)
{
    str.remove();
    DateFormat* format = getDateFormat();
    if (format == 0)
    {
        str += "DATE_FORMAT_FAILURE";
        return str;
    }
    TimeZone* save = format->getTimeZone().clone();
    format->setTimeZone(tz);
    format->format(d, str);
    format->adoptTimeZone(save);
    releaseDateFormat(format);
    return str;
}



UDate
CalendarTimeZoneTest::date(int32_t y, int32_t m, int32_t d, int32_t hr, int32_t min, int32_t sec)
{
    Calendar* cal = getCalendar();
    if (cal == 0) return 0.0;
    cal->clear();
    cal->set(1900 + y, m, d, hr, min, sec); 
    UErrorCode status = U_ZERO_ERROR;
    UDate dt = cal->getTime(status);
    releaseCalendar(cal);
    if (U_FAILURE(status))
    {
        errln("FAIL: Calendar::getTime failed: %s", u_errorName(status));
        return 0.0;
    }
    return dt;
}





















void
CalendarTimeZoneTest::dateToFields(UDate date, int32_t& y, int32_t& m, int32_t& d, int32_t& hr, int32_t& min, int32_t& sec)
{
    Calendar* cal = getCalendar();
    if (cal == 0) return;
    UErrorCode status = U_ZERO_ERROR;
    cal->setTime(date, status);
    y = cal->get(UCAL_YEAR, status) - 1900;
    m = cal->get(UCAL_MONTH, status);
    d = cal->get(UCAL_DATE, status);
    hr = cal->get(UCAL_HOUR_OF_DAY, status);
    min = cal->get(UCAL_MINUTE, status);
    sec = cal->get(UCAL_SECOND, status);
    releaseCalendar(cal);
}

void CalendarTimeZoneTest::cleanup()
{
    delete fgDateFormat;
    fgDateFormat = 0;
    delete fgCalendar;
    fgCalendar   = 0;
}

#endif 


