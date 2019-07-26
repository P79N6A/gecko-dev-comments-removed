










#ifndef __WINDTFMT
#define __WINDTFMT

#include "unicode/utypes.h"

#if U_PLATFORM_HAS_WIN32_API

#if !UCONFIG_NO_FORMATTING

#include "unicode/format.h"
#include "unicode/datefmt.h"
#include "unicode/calendar.h"
#include "unicode/ustring.h"
#include "unicode/locid.h"






U_CDECL_BEGIN

typedef struct _SYSTEMTIME SYSTEMTIME;
typedef struct _TIME_ZONE_INFORMATION TIME_ZONE_INFORMATION;
U_CDECL_END

U_NAMESPACE_BEGIN

class Win32DateFormat : public DateFormat
{
public:
    Win32DateFormat(DateFormat::EStyle timeStyle, DateFormat::EStyle dateStyle, const Locale &locale, UErrorCode &status);

    Win32DateFormat(const Win32DateFormat &other);

    virtual ~Win32DateFormat();

    virtual Format *clone(void) const;

    Win32DateFormat &operator=(const Win32DateFormat &other);

    UnicodeString &format(Calendar &cal, UnicodeString &appendTo, FieldPosition &pos) const;

    UnicodeString& format(UDate date, UnicodeString& appendTo) const;

    void parse(const UnicodeString& text, Calendar& cal, ParsePosition& pos) const;

    






    virtual void adoptCalendar(Calendar* calendarToAdopt);

    





    virtual void setCalendar(const Calendar& newCalendar);

    





    virtual void adoptTimeZone(TimeZone* zoneToAdopt);

    



    virtual void setTimeZone(const TimeZone& zone);

    









    U_I18N_API static UClassID U_EXPORT2 getStaticClassID(void);

    









    virtual UClassID getDynamicClassID(void) const;

private:
    void formatDate(const SYSTEMTIME *st, UnicodeString &appendTo) const;
    void formatTime(const SYSTEMTIME *st, UnicodeString &appendTo) const;

    UnicodeString setTimeZoneInfo(TIME_ZONE_INFORMATION *tzi, const TimeZone &zone) const;
    UnicodeString* getTimeDateFormat(const Calendar *cal, const Locale *locale, UErrorCode &status) const;

    UnicodeString *fDateTimeMsg;
    DateFormat::EStyle fTimeStyle;
    DateFormat::EStyle fDateStyle;
    const Locale *fLocale;
    int32_t fLCID;
    UnicodeString fZoneID;
    TIME_ZONE_INFORMATION *fTZI;
};

inline UnicodeString &Win32DateFormat::format(UDate date, UnicodeString& appendTo) const {
    return DateFormat::format(date, appendTo);
}

U_NAMESPACE_END

#endif 

#endif 

#endif 
