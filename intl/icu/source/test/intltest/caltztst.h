






#ifndef _CALTZTST
#define _CALTZTST

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/unistr.h"
#include "unicode/calendar.h"
#include "unicode/datefmt.h"
#include "intltest.h"




class CalendarTimeZoneTest : public IntlTest
{
public:
    static void cleanup();
protected:
    
    
    UBool failure(UErrorCode status, const char* msg, UBool possibleDataError=FALSE);

    
    
    UnicodeString  dateToString(UDate d);
    UnicodeString& dateToString(UDate d, UnicodeString& str);
    UnicodeString& dateToString(UDate d, UnicodeString& str, const TimeZone& z);

    
    
    UDate date(int32_t y, int32_t m, int32_t d, int32_t hr=0, int32_t min=0, int32_t sec=0);

    
    


    
    void dateToFields(UDate date, int32_t& y, int32_t& m, int32_t& d, int32_t& hr, int32_t& min, int32_t& sec);

protected:
    static DateFormat*         fgDateFormat;
    static Calendar*           fgCalendar;

    
    
     DateFormat*               getDateFormat(void);
    static void                releaseDateFormat(DateFormat *f);

     Calendar*                 getCalendar(void);
    static void                releaseCalendar(Calendar *c);
};

#endif 

#endif 

