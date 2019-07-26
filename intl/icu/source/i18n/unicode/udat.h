






#ifndef UDAT_H
#define UDAT_H

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/localpointer.h"
#include "unicode/ucal.h"
#include "unicode/unum.h"
#include "unicode/udisplaycontext.h"

























































































































typedef void* UDateFormat;




typedef enum UDateFormatStyle {
    
    UDAT_FULL,
    
    UDAT_LONG,
    
    UDAT_MEDIUM,
    
    UDAT_SHORT,
    
    UDAT_DEFAULT = UDAT_MEDIUM,

    
    UDAT_RELATIVE = (1 << 7),
    
    UDAT_FULL_RELATIVE = UDAT_FULL | UDAT_RELATIVE,
        
    UDAT_LONG_RELATIVE = UDAT_LONG | UDAT_RELATIVE,
    
    UDAT_MEDIUM_RELATIVE = UDAT_MEDIUM | UDAT_RELATIVE,
    
    UDAT_SHORT_RELATIVE = UDAT_SHORT | UDAT_RELATIVE,
    
    
    
    UDAT_NONE = -1,

    




    UDAT_PATTERN = -2,

    
    UDAT_IGNORE = UDAT_PATTERN
} UDateFormatStyle;







#define UDAT_YEAR                       "y"




#define UDAT_QUARTER                    "QQQQ"




#define UDAT_ABBR_QUARTER               "QQQ"




#define UDAT_YEAR_QUARTER               "yQQQQ"




#define UDAT_YEAR_ABBR_QUARTER          "yQQQ"




#define UDAT_MONTH                      "MMMM"




#define UDAT_ABBR_MONTH                 "MMM"




#define UDAT_NUM_MONTH                  "M"




#define UDAT_YEAR_MONTH                 "yMMMM"




#define UDAT_YEAR_ABBR_MONTH            "yMMM"




#define UDAT_YEAR_NUM_MONTH             "yM"




#define UDAT_DAY                        "d"





#define UDAT_YEAR_MONTH_DAY             "yMMMMd"





#define UDAT_YEAR_ABBR_MONTH_DAY        "yMMMd"





#define UDAT_YEAR_NUM_MONTH_DAY         "yMd"




#define UDAT_WEEKDAY                    "EEEE"




#define UDAT_ABBR_WEEKDAY               "E"





#define UDAT_YEAR_MONTH_WEEKDAY_DAY     "yMMMMEEEEd"





#define UDAT_YEAR_ABBR_MONTH_WEEKDAY_DAY "yMMMEd" 





#define UDAT_YEAR_NUM_MONTH_WEEKDAY_DAY "yMEd"





#define UDAT_MONTH_DAY                  "MMMMd"





#define UDAT_ABBR_MONTH_DAY             "MMMd"





#define UDAT_NUM_MONTH_DAY              "Md"





#define UDAT_MONTH_WEEKDAY_DAY          "MMMMEEEEd"





#define UDAT_ABBR_MONTH_WEEKDAY_DAY     "MMMEd"





#define UDAT_NUM_MONTH_WEEKDAY_DAY      "MEd"







#define UDAT_HOUR                       "j"




#define UDAT_HOUR24                     "H"




#define UDAT_MINUTE                     "m"





#define UDAT_HOUR_MINUTE                "jm"





#define UDAT_HOUR24_MINUTE              "Hm"




#define UDAT_SECOND                     "s"






#define UDAT_HOUR_MINUTE_SECOND         "jms"






#define UDAT_HOUR24_MINUTE_SECOND       "Hms"





#define UDAT_MINUTE_SECOND              "ms"










#define UDAT_LOCATION_TZ "VVVV"







#define UDAT_GENERIC_TZ "vvvv"







#define UDAT_ABBR_GENERIC_TZ "v"







#define UDAT_SPECIFIC_TZ "zzzz"







#define UDAT_ABBR_SPECIFIC_TZ "z"







#define UDAT_ABBR_UTC_TZ "ZZZZ"







#define UDAT_STANDALONE_MONTH           "LLLL"




#define UDAT_ABBR_STANDALONE_MONTH      "LLL"





#define UDAT_HOUR_MINUTE_GENERIC_TZ     "jmv"




#define UDAT_HOUR_MINUTE_TZ             "jmz"




#define UDAT_HOUR_GENERIC_TZ            "jv"




#define UDAT_HOUR_TZ                    "jz"






typedef enum UDateFormatField {
    




    UDAT_ERA_FIELD = 0,

    




    UDAT_YEAR_FIELD = 1,

    




    UDAT_MONTH_FIELD = 2,

    




    UDAT_DATE_FIELD = 3,

    






    UDAT_HOUR_OF_DAY1_FIELD = 4,

    






    UDAT_HOUR_OF_DAY0_FIELD = 5,

    




    UDAT_MINUTE_FIELD = 6,

    




    UDAT_SECOND_FIELD = 7,

    












    UDAT_FRACTIONAL_SECOND_FIELD = 8,

    




    UDAT_DAY_OF_WEEK_FIELD = 9,

    




    UDAT_DAY_OF_YEAR_FIELD = 10,

    




    UDAT_DAY_OF_WEEK_IN_MONTH_FIELD = 11,

    




    UDAT_WEEK_OF_YEAR_FIELD = 12,

    




    UDAT_WEEK_OF_MONTH_FIELD = 13,

    




    UDAT_AM_PM_FIELD = 14,

    






    UDAT_HOUR1_FIELD = 15,

    






    UDAT_HOUR0_FIELD = 16,

    





    UDAT_TIMEZONE_FIELD = 17,

    




    UDAT_YEAR_WOY_FIELD = 18,

    




    UDAT_DOW_LOCAL_FIELD = 19,

    




    UDAT_EXTENDED_YEAR_FIELD = 20,

    




    UDAT_JULIAN_DAY_FIELD = 21,

    




    UDAT_MILLISECONDS_IN_DAY_FIELD = 22,

    





    UDAT_TIMEZONE_RFC_FIELD = 23,

    




    UDAT_TIMEZONE_GENERIC_FIELD = 24,
    





    UDAT_STANDALONE_DAY_FIELD = 25,

    





    UDAT_STANDALONE_MONTH_FIELD = 26,

    






    UDAT_QUARTER_FIELD = 27,

    






    UDAT_STANDALONE_QUARTER_FIELD = 28,

    




    UDAT_TIMEZONE_SPECIAL_FIELD = 29,

    






    UDAT_YEAR_NAME_FIELD = 30,

   







    UDAT_FIELD_COUNT = 31

} UDateFormatField;










U_STABLE UCalendarDateFields U_EXPORT2
udat_toCalendarDateField(UDateFormatField field);






























U_STABLE UDateFormat* U_EXPORT2 
udat_open(UDateFormatStyle  timeStyle,
          UDateFormatStyle  dateStyle,
          const char        *locale,
          const UChar       *tzID,
          int32_t           tzIDLength,
          const UChar       *pattern,
          int32_t           patternLength,
          UErrorCode        *status);








U_STABLE void U_EXPORT2 
udat_close(UDateFormat* format);

#if U_SHOW_CPLUSPLUS_API

U_NAMESPACE_BEGIN










U_DEFINE_LOCAL_OPEN_POINTER(LocalUDateFormatPointer, UDateFormat, udat_close);

U_NAMESPACE_END

#endif









U_STABLE UDateFormat* U_EXPORT2 
udat_clone(const UDateFormat *fmt,
       UErrorCode *status);



















U_STABLE int32_t U_EXPORT2 
udat_format(    const    UDateFormat*    format,
                        UDate           dateToFormat,
                        UChar*          result,
                        int32_t         resultLength,
                        UFieldPosition* position,
                        UErrorCode*     status);


























U_STABLE UDate U_EXPORT2 
udat_parse(const    UDateFormat*    format,
           const    UChar*          text,
                    int32_t         textLength,
                    int32_t         *parsePos,
                    UErrorCode      *status);






















U_STABLE void U_EXPORT2 
udat_parseCalendar(const    UDateFormat*    format,
                            UCalendar*      calendar,
                   const    UChar*          text,
                            int32_t         textLength,
                            int32_t         *parsePos,
                            UErrorCode      *status);










U_STABLE UBool U_EXPORT2 
udat_isLenient(const UDateFormat* fmt);










U_STABLE void U_EXPORT2 
udat_setLenient(    UDateFormat*    fmt,
                    UBool          isLenient);










U_STABLE const UCalendar* U_EXPORT2 
udat_getCalendar(const UDateFormat* fmt);










U_STABLE void U_EXPORT2 
udat_setCalendar(            UDateFormat*    fmt,
                    const   UCalendar*      calendarToSet);










U_STABLE const UNumberFormat* U_EXPORT2 
udat_getNumberFormat(const UDateFormat* fmt);










U_STABLE void U_EXPORT2 
udat_setNumberFormat(            UDateFormat*    fmt,
                        const   UNumberFormat*  numberFormatToSet);










U_STABLE const char* U_EXPORT2 
udat_getAvailable(int32_t localeIndex);









U_STABLE int32_t U_EXPORT2 
udat_countAvailable(void);











U_STABLE UDate U_EXPORT2 
udat_get2DigitYearStart(    const   UDateFormat     *fmt,
                                    UErrorCode      *status);











U_STABLE void U_EXPORT2 
udat_set2DigitYearStart(    UDateFormat     *fmt,
                            UDate           d,
                            UErrorCode      *status);













U_STABLE int32_t U_EXPORT2 
udat_toPattern(    const   UDateFormat     *fmt,
                        UBool          localized,
                        UChar           *result,
                        int32_t         resultLength,
                        UErrorCode      *status);











U_STABLE void U_EXPORT2 
udat_applyPattern(            UDateFormat     *format,
                            UBool          localized,
                    const   UChar           *pattern,
                            int32_t         patternLength);





typedef enum UDateFormatSymbolType {
    
    UDAT_ERAS,
    
    UDAT_MONTHS,
    
    UDAT_SHORT_MONTHS,
    
    UDAT_WEEKDAYS,
    
    UDAT_SHORT_WEEKDAYS,
    
    UDAT_AM_PMS,
    
    UDAT_LOCALIZED_CHARS,
    
    UDAT_ERA_NAMES,
    
    UDAT_NARROW_MONTHS,
    
    UDAT_NARROW_WEEKDAYS,
    
    UDAT_STANDALONE_MONTHS,
    UDAT_STANDALONE_SHORT_MONTHS,
    UDAT_STANDALONE_NARROW_MONTHS,
    
    UDAT_STANDALONE_WEEKDAYS,
    UDAT_STANDALONE_SHORT_WEEKDAYS,
    UDAT_STANDALONE_NARROW_WEEKDAYS,
    
    UDAT_QUARTERS,
    
    UDAT_SHORT_QUARTERS,
    
    UDAT_STANDALONE_QUARTERS,
    UDAT_STANDALONE_SHORT_QUARTERS

} UDateFormatSymbolType;

struct UDateFormatSymbols;




typedef struct UDateFormatSymbols UDateFormatSymbols;

















U_STABLE int32_t U_EXPORT2 
udat_getSymbols(const   UDateFormat             *fmt,
                        UDateFormatSymbolType   type,
                        int32_t                 symbolIndex,
                        UChar                   *result,
                        int32_t                 resultLength,
                        UErrorCode              *status);













U_STABLE int32_t U_EXPORT2 
udat_countSymbols(    const    UDateFormat                *fmt,
                            UDateFormatSymbolType    type);
















U_STABLE void U_EXPORT2 
udat_setSymbols(    UDateFormat             *format,
                    UDateFormatSymbolType   type,
                    int32_t                 symbolIndex,
                    UChar                   *value,
                    int32_t                 valueLength,
                    UErrorCode              *status);










U_STABLE const char* U_EXPORT2
udat_getLocaleByType(const UDateFormat *fmt,
                     ULocDataLocaleType type,
                     UErrorCode* status); 

#ifndef U_HIDE_INTERNAL_API








U_INTERNAL void U_EXPORT2
udat_setContext(UDateFormat* fmt, UDisplayContext value, UErrorCode* status);










U_INTERNAL UDisplayContext U_EXPORT2
udat_getContext(UDateFormat* fmt, UDisplayContextType type, UErrorCode* status);

#endif  

#ifndef U_HIDE_INTERNAL_API











U_INTERNAL int32_t U_EXPORT2 
udat_toPatternRelativeDate(const UDateFormat *fmt,
                           UChar             *result,
                           int32_t           resultLength,
                           UErrorCode        *status);












U_INTERNAL int32_t U_EXPORT2 
udat_toPatternRelativeTime(const UDateFormat *fmt,
                           UChar             *result,
                           int32_t           resultLength,
                           UErrorCode        *status);













U_INTERNAL void U_EXPORT2 
udat_applyPatternRelative(UDateFormat *format,
                          const UChar *datePattern,
                          int32_t     datePatternLength,
                          const UChar *timePattern,
                          int32_t     timePatternLength,
                          UErrorCode  *status);
#endif  





typedef UDateFormat* (U_EXPORT2 *UDateFormatOpener) (UDateFormatStyle  timeStyle,
                                                    UDateFormatStyle  dateStyle,
                                                    const char        *locale,
                                                    const UChar       *tzID,
                                                    int32_t           tzIDLength,
                                                    const UChar       *pattern,
                                                    int32_t           patternLength,
                                                    UErrorCode        *status);





U_INTERNAL void U_EXPORT2
udat_registerOpener(UDateFormatOpener opener, UErrorCode *status);





U_INTERNAL UDateFormatOpener U_EXPORT2
udat_unregisterOpener(UDateFormatOpener opener, UErrorCode *status);


#endif 

#endif
