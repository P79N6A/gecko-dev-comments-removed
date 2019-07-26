

















#ifndef DATEFMT_H
#define DATEFMT_H

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/udat.h"
#include "unicode/calendar.h"
#include "unicode/numfmt.h"
#include "unicode/format.h"
#include "unicode/locid.h"






U_NAMESPACE_BEGIN

class TimeZone;
class DateTimePatternGenerator;

































































































class U_I18N_API DateFormat : public Format {
public:

    






    enum EStyle
    {
        kNone   = -1,

        kFull   = 0,
        kLong   = 1,
        kMedium = 2,
        kShort  = 3,

        kDateOffset   = kShort + 1,
     
     
     
     

        kDateTime             = 8,
     

        kDateTimeOffset = kDateTime + 1,
     
     
     
     

        
        kRelative = (1 << 7),

        kFullRelative = (kFull | kRelative),

        kLongRelative = kLong | kRelative,

        kMediumRelative = kMedium | kRelative,

        kShortRelative = kShort | kRelative,


        kDefault      = kMedium,



    



        FULL        = kFull,
        LONG        = kLong,
        MEDIUM        = kMedium,
        SHORT        = kShort,
        DEFAULT        = kDefault,
        DATE_OFFSET    = kDateOffset,
        NONE        = kNone,
        DATE_TIME    = kDateTime
    };

    



    virtual ~DateFormat();

    



    virtual UBool operator==(const Format&) const;


    using Format::format;

    













    virtual UnicodeString& format(const Formattable& obj,
                                  UnicodeString& appendTo,
                                  FieldPosition& pos,
                                  UErrorCode& status) const;

    














    virtual UnicodeString& format(const Formattable& obj,
                                  UnicodeString& appendTo,
                                  FieldPositionIterator* posIter,
                                  UErrorCode& status) const;
    
































    virtual UnicodeString& format(  Calendar& cal,
                                    UnicodeString& appendTo,
                                    FieldPosition& fieldPosition) const = 0;

    

















    virtual UnicodeString& format(Calendar& cal,
                                  UnicodeString& appendTo,
                                  FieldPositionIterator* posIter,
                                  UErrorCode& status) const;
    


























    UnicodeString& format(  UDate date,
                            UnicodeString& appendTo,
                            FieldPosition& fieldPosition) const;

    












    UnicodeString& format(UDate date,
                          UnicodeString& appendTo,
                          FieldPositionIterator* posIter,
                          UErrorCode& status) const;
    










    UnicodeString& format(UDate date, UnicodeString& appendTo) const;

    









    UnicodeString& format(const Formattable& obj,
                          UnicodeString& appendTo,
                          UErrorCode& status) const;

    

































    virtual UDate parse( const UnicodeString& text,
                        UErrorCode& status) const;

    

























    virtual void parse( const UnicodeString& text,
                        Calendar& cal,
                        ParsePosition& pos) const = 0;

    





























    UDate parse( const UnicodeString& text,
                 ParsePosition& pos) const;

    






















    virtual void parseObject(const UnicodeString& source,
                             Formattable& result,
                             ParsePosition& parse_pos) const;

    






    static DateFormat* U_EXPORT2 createInstance(void);

    










    static DateFormat* U_EXPORT2 createTimeInstance(EStyle style = kDefault,
                                          const Locale& aLocale = Locale::getDefault());

    














    static DateFormat* U_EXPORT2 createDateInstance(EStyle style = kDefault,
                                          const Locale& aLocale = Locale::getDefault());

    

















    static DateFormat* U_EXPORT2 createDateTimeInstance(EStyle dateStyle = kDefault,
                                              EStyle timeStyle = kDefault,
                                              const Locale& aLocale = Locale::getDefault());

    






    static const Locale* U_EXPORT2 getAvailableLocales(int32_t& count);

    



    virtual UBool isLenient(void) const;

    









    virtual void setLenient(UBool lenient);

    




    virtual const Calendar* getCalendar(void) const;

    








    virtual void adoptCalendar(Calendar* calendarToAdopt);

    






    virtual void setCalendar(const Calendar& newCalendar);


    





    virtual const NumberFormat* getNumberFormat(void) const;

    





    virtual void adoptNumberFormat(NumberFormat* formatToAdopt);

    




    virtual void setNumberFormat(const NumberFormat& newNumberFormat);

    




    virtual const TimeZone& getTimeZone(void) const;

    





    virtual void adoptTimeZone(TimeZone* zoneToAdopt);

    




    virtual void setTimeZone(const TimeZone& zone);

protected:
    





    DateFormat();

    



    DateFormat(const DateFormat&);

    



    DateFormat& operator=(const DateFormat&);

    





    Calendar* fCalendar;

    





    NumberFormat* fNumberFormat;

private:
    







    static DateFormat* U_EXPORT2 create(EStyle timeStyle, EStyle dateStyle, const Locale& inLocale);

public:
#ifndef U_HIDE_OBSOLETE_API
    




    enum EField
    {
        
        kEraField = UDAT_ERA_FIELD,
        kYearField = UDAT_YEAR_FIELD,
        kMonthField = UDAT_MONTH_FIELD,
        kDateField = UDAT_DATE_FIELD,
        kHourOfDay1Field = UDAT_HOUR_OF_DAY1_FIELD,
        kHourOfDay0Field = UDAT_HOUR_OF_DAY0_FIELD,
        kMinuteField = UDAT_MINUTE_FIELD,
        kSecondField = UDAT_SECOND_FIELD,
        kMillisecondField = UDAT_FRACTIONAL_SECOND_FIELD,
        kDayOfWeekField = UDAT_DAY_OF_WEEK_FIELD,
        kDayOfYearField = UDAT_DAY_OF_YEAR_FIELD,
        kDayOfWeekInMonthField = UDAT_DAY_OF_WEEK_IN_MONTH_FIELD,
        kWeekOfYearField = UDAT_WEEK_OF_YEAR_FIELD,
        kWeekOfMonthField = UDAT_WEEK_OF_MONTH_FIELD,
        kAmPmField = UDAT_AM_PM_FIELD,
        kHour1Field = UDAT_HOUR1_FIELD,
        kHour0Field = UDAT_HOUR0_FIELD,
        kTimezoneField = UDAT_TIMEZONE_FIELD,
        kYearWOYField = UDAT_YEAR_WOY_FIELD,
        kDOWLocalField = UDAT_DOW_LOCAL_FIELD,
        kExtendedYearField = UDAT_EXTENDED_YEAR_FIELD,
        kJulianDayField = UDAT_JULIAN_DAY_FIELD,
        kMillisecondsInDayField = UDAT_MILLISECONDS_IN_DAY_FIELD,

        
        ERA_FIELD = UDAT_ERA_FIELD,
        YEAR_FIELD = UDAT_YEAR_FIELD,
        MONTH_FIELD = UDAT_MONTH_FIELD,
        DATE_FIELD = UDAT_DATE_FIELD,
        HOUR_OF_DAY1_FIELD = UDAT_HOUR_OF_DAY1_FIELD,
        HOUR_OF_DAY0_FIELD = UDAT_HOUR_OF_DAY0_FIELD,
        MINUTE_FIELD = UDAT_MINUTE_FIELD,
        SECOND_FIELD = UDAT_SECOND_FIELD,
        MILLISECOND_FIELD = UDAT_FRACTIONAL_SECOND_FIELD,
        DAY_OF_WEEK_FIELD = UDAT_DAY_OF_WEEK_FIELD,
        DAY_OF_YEAR_FIELD = UDAT_DAY_OF_YEAR_FIELD,
        DAY_OF_WEEK_IN_MONTH_FIELD = UDAT_DAY_OF_WEEK_IN_MONTH_FIELD,
        WEEK_OF_YEAR_FIELD = UDAT_WEEK_OF_YEAR_FIELD,
        WEEK_OF_MONTH_FIELD = UDAT_WEEK_OF_MONTH_FIELD,
        AM_PM_FIELD = UDAT_AM_PM_FIELD,
        HOUR1_FIELD = UDAT_HOUR1_FIELD,
        HOUR0_FIELD = UDAT_HOUR0_FIELD,
        TIMEZONE_FIELD = UDAT_TIMEZONE_FIELD
    };
#endif  
};

inline UnicodeString&
DateFormat::format(const Formattable& obj,
                   UnicodeString& appendTo,
                   UErrorCode& status) const {
    return Format::format(obj, appendTo, status);
}
U_NAMESPACE_END

#endif

#endif

