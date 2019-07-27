










#ifndef _UNUM
#define _UNUM

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/localpointer.h"
#include "unicode/uloc.h"
#include "unicode/ucurr.h"
#include "unicode/umisc.h"
#include "unicode/parseerr.h"
#include "unicode/uformattable.h"
#include "unicode/udisplaycontext.h"












































































































typedef void* UNumberFormat;




typedef enum UNumberFormatStyle {
    



    UNUM_PATTERN_DECIMAL=0,
    



    UNUM_DECIMAL=1,
    



    UNUM_CURRENCY=2,
    



    UNUM_PERCENT=3,
    



    UNUM_SCIENTIFIC=4,
    





    UNUM_SPELLOUT=5,
    





    UNUM_ORDINAL=6,
    



    UNUM_DURATION=7,
    



    UNUM_NUMBERING_SYSTEM=8,
    



    UNUM_PATTERN_RULEBASED=9,
    



    UNUM_CURRENCY_ISO=10,
    




    UNUM_CURRENCY_PLURAL=11,
    




    UNUM_CURRENCY_ACCOUNTING=12,
#ifndef U_HIDE_DRAFT_API
    




    UNUM_CASH_CURRENCY=13,
#endif 

    



    UNUM_FORMAT_STYLE_COUNT=14,

    



    UNUM_DEFAULT = UNUM_DECIMAL,
    



    UNUM_IGNORE = UNUM_PATTERN_DECIMAL
} UNumberFormatStyle;




typedef enum UNumberFormatRoundingMode {
    UNUM_ROUND_CEILING,
    UNUM_ROUND_FLOOR,
    UNUM_ROUND_DOWN,
    UNUM_ROUND_UP,
    



    UNUM_ROUND_HALFEVEN,
#ifndef U_HIDE_DEPRECATED_API
    



    UNUM_FOUND_HALFEVEN = UNUM_ROUND_HALFEVEN,
#endif  
    UNUM_ROUND_HALFDOWN = UNUM_ROUND_HALFEVEN + 1,
    UNUM_ROUND_HALFUP,
    



    UNUM_ROUND_UNNECESSARY
} UNumberFormatRoundingMode;




typedef enum UNumberFormatPadPosition {
    UNUM_PAD_BEFORE_PREFIX,
    UNUM_PAD_AFTER_PREFIX,
    UNUM_PAD_BEFORE_SUFFIX,
    UNUM_PAD_AFTER_SUFFIX
} UNumberFormatPadPosition;





typedef enum UNumberCompactStyle {
  
  UNUM_SHORT,
  
  UNUM_LONG
  
} UNumberCompactStyle;





enum UCurrencySpacing {
    
    UNUM_CURRENCY_MATCH,
    
    UNUM_CURRENCY_SURROUNDING_MATCH,
    
    UNUM_CURRENCY_INSERT,
    
    UNUM_CURRENCY_SPACING_COUNT
};
typedef enum UCurrencySpacing UCurrencySpacing; 







typedef enum UNumberFormatFields {
    
    UNUM_INTEGER_FIELD,
    
    UNUM_FRACTION_FIELD,
    
    UNUM_DECIMAL_SEPARATOR_FIELD,
    
    UNUM_EXPONENT_SYMBOL_FIELD,
    
    UNUM_EXPONENT_SIGN_FIELD,
    
    UNUM_EXPONENT_FIELD,
    
    UNUM_GROUPING_SEPARATOR_FIELD,
    
    UNUM_CURRENCY_FIELD,
    
    UNUM_PERCENT_FIELD,
    
    UNUM_PERMILL_FIELD,
    
    UNUM_SIGN_FIELD,
    
    UNUM_FIELD_COUNT
} UNumberFormatFields;



































U_STABLE UNumberFormat* U_EXPORT2 
unum_open(  UNumberFormatStyle    style,
            const    UChar*    pattern,
            int32_t            patternLength,
            const    char*     locale,
            UParseError*       parseErr,
            UErrorCode*        status);








U_STABLE void U_EXPORT2 
unum_close(UNumberFormat* fmt);

#if U_SHOW_CPLUSPLUS_API

U_NAMESPACE_BEGIN










U_DEFINE_LOCAL_OPEN_POINTER(LocalUNumberFormatPointer, UNumberFormat, unum_close);

U_NAMESPACE_END

#endif









U_STABLE UNumberFormat* U_EXPORT2 
unum_clone(const UNumberFormat *fmt,
       UErrorCode *status);

























U_STABLE int32_t U_EXPORT2 
unum_format(    const    UNumberFormat*    fmt,
        int32_t            number,
        UChar*            result,
        int32_t            resultLength,
        UFieldPosition    *pos,
        UErrorCode*        status);

























U_STABLE int32_t U_EXPORT2 
unum_formatInt64(const UNumberFormat *fmt,
        int64_t         number,
        UChar*          result,
        int32_t         resultLength,
        UFieldPosition *pos,
        UErrorCode*     status);

























U_STABLE int32_t U_EXPORT2 
unum_formatDouble(    const    UNumberFormat*  fmt,
            double          number,
            UChar*          result,
            int32_t         resultLength,
            UFieldPosition  *pos, 
            UErrorCode*     status);





























U_STABLE int32_t U_EXPORT2 
unum_formatDecimal(    const    UNumberFormat*  fmt,
            const char *    number,
            int32_t         length,
            UChar*          result,
            int32_t         resultLength,
            UFieldPosition  *pos, 
            UErrorCode*     status);

























U_STABLE int32_t U_EXPORT2
unum_formatDoubleCurrency(const UNumberFormat* fmt,
                          double number,
                          UChar* currency,
                          UChar* result,
                          int32_t resultLength,
                          UFieldPosition* pos,
                          UErrorCode* status);





















U_STABLE int32_t U_EXPORT2
unum_formatUFormattable(const UNumberFormat* fmt,
                        const UFormattable *number,
                        UChar *result,
                        int32_t resultLength,
                        UFieldPosition *pos,
                        UErrorCode *status);


















U_STABLE int32_t U_EXPORT2 
unum_parse(    const   UNumberFormat*  fmt,
        const   UChar*          text,
        int32_t         textLength,
        int32_t         *parsePos ,
        UErrorCode      *status);


















U_STABLE int64_t U_EXPORT2 
unum_parseInt64(const UNumberFormat*  fmt,
        const UChar*  text,
        int32_t       textLength,
        int32_t       *parsePos ,
        UErrorCode    *status);


















U_STABLE double U_EXPORT2 
unum_parseDouble(    const   UNumberFormat*  fmt,
            const   UChar*          text,
            int32_t         textLength,
            int32_t         *parsePos ,
            UErrorCode      *status);



























U_STABLE int32_t U_EXPORT2 
unum_parseDecimal(const   UNumberFormat*  fmt,
                 const   UChar*          text,
                         int32_t         textLength,
                         int32_t         *parsePos ,
                         char            *outBuf,
                         int32_t         outBufLength,
                         UErrorCode      *status);




















U_STABLE double U_EXPORT2
unum_parseDoubleCurrency(const UNumberFormat* fmt,
                         const UChar* text,
                         int32_t textLength,
                         int32_t* parsePos, 
                         UChar* currency,
                         UErrorCode* status);



















U_STABLE UFormattable* U_EXPORT2
unum_parseToUFormattable(const UNumberFormat* fmt,
                         UFormattable *result,
                         const UChar* text,
                         int32_t textLength,
                         int32_t* parsePos, 
                         UErrorCode* status);

















U_STABLE void U_EXPORT2 
unum_applyPattern(          UNumberFormat  *format,
                            UBool          localized,
                    const   UChar          *pattern,
                            int32_t         patternLength,
                            UParseError    *parseError,
                            UErrorCode     *status
                                    );











U_STABLE const char* U_EXPORT2 
unum_getAvailable(int32_t localeIndex);










U_STABLE int32_t U_EXPORT2 
unum_countAvailable(void);

#if UCONFIG_HAVE_PARSEALLINPUT




typedef enum UNumberFormatAttributeValue {
#ifndef U_HIDE_INTERNAL_API
  
  UNUM_NO = 0,
  
  UNUM_YES = 1,
  
  UNUM_MAYBE = 2
#endif 
} UNumberFormatAttributeValue;
#endif


typedef enum UNumberFormatAttribute {
  
  UNUM_PARSE_INT_ONLY,
  
  UNUM_GROUPING_USED,
  
  UNUM_DECIMAL_ALWAYS_SHOWN,
  
  UNUM_MAX_INTEGER_DIGITS,
  
  UNUM_MIN_INTEGER_DIGITS,
  
  UNUM_INTEGER_DIGITS,
  
  UNUM_MAX_FRACTION_DIGITS,
  
  UNUM_MIN_FRACTION_DIGITS,
  
  UNUM_FRACTION_DIGITS,
  
  UNUM_MULTIPLIER,
  
  UNUM_GROUPING_SIZE,
  
  UNUM_ROUNDING_MODE,
  
  UNUM_ROUNDING_INCREMENT,
  
  UNUM_FORMAT_WIDTH,
  
  UNUM_PADDING_POSITION,
  
  UNUM_SECONDARY_GROUPING_SIZE,
  

  UNUM_SIGNIFICANT_DIGITS_USED,
  

  UNUM_MIN_SIGNIFICANT_DIGITS,
  

  UNUM_MAX_SIGNIFICANT_DIGITS,
  


  UNUM_LENIENT_PARSE,
#if UCONFIG_HAVE_PARSEALLINPUT
  



  UNUM_PARSE_ALL_INPUT = UNUM_LENIENT_PARSE + 1,
#endif
  








  UNUM_SCALE = UNUM_LENIENT_PARSE + 2,

#ifndef U_HIDE_INTERNAL_API
  

  UNUM_NUMERIC_ATTRIBUTE_COUNT = UNUM_LENIENT_PARSE + 3,
#endif  

#ifndef U_HIDE_DRAFT_API
  





  UNUM_CURRENCY_USAGE = UNUM_LENIENT_PARSE + 4,
#endif  

  
  


  UNUM_MAX_NONBOOLEAN_ATTRIBUTE = 0x0FFF,

  




  UNUM_FORMAT_FAIL_IF_MORE_THAN_MAX_DIGITS = 0x1000,
  





  UNUM_PARSE_NO_EXPONENT,

#ifndef U_HIDE_DRAFT_API
  







  UNUM_PARSE_DECIMAL_MARK_REQUIRED = UNUM_PARSE_NO_EXPONENT+1,
#endif  

  
  

  UNUM_LIMIT_BOOLEAN_ATTRIBUTE = UNUM_PARSE_NO_EXPONENT+2
} UNumberFormatAttribute;


















U_STABLE int32_t U_EXPORT2 
unum_getAttribute(const UNumberFormat*          fmt,
          UNumberFormatAttribute  attr);




















U_STABLE void U_EXPORT2 
unum_setAttribute(    UNumberFormat*          fmt,
            UNumberFormatAttribute  attr,
            int32_t                 newValue);
















U_STABLE double U_EXPORT2 
unum_getDoubleAttribute(const UNumberFormat*          fmt,
          UNumberFormatAttribute  attr);















U_STABLE void U_EXPORT2 
unum_setDoubleAttribute(    UNumberFormat*          fmt,
            UNumberFormatAttribute  attr,
            double                 newValue);


typedef enum UNumberFormatTextAttribute {
  
  UNUM_POSITIVE_PREFIX,
  
  UNUM_POSITIVE_SUFFIX,
  
  UNUM_NEGATIVE_PREFIX,
  
  UNUM_NEGATIVE_SUFFIX,
  
  UNUM_PADDING_CHARACTER,
  
  UNUM_CURRENCY_CODE,
  







  UNUM_DEFAULT_RULESET,
  







  UNUM_PUBLIC_RULESETS
} UNumberFormatTextAttribute;



















U_STABLE int32_t U_EXPORT2 
unum_getTextAttribute(    const    UNumberFormat*                    fmt,
            UNumberFormatTextAttribute      tag,
            UChar*                            result,
            int32_t                            resultLength,
            UErrorCode*                        status);

















U_STABLE void U_EXPORT2 
unum_setTextAttribute(    UNumberFormat*                    fmt,
            UNumberFormatTextAttribute      tag,
            const    UChar*                            newValue,
            int32_t                            newValueLength,
            UErrorCode                        *status);

















U_STABLE int32_t U_EXPORT2 
unum_toPattern(    const    UNumberFormat*          fmt,
        UBool                  isPatternLocalized,
        UChar*                  result,
        int32_t                 resultLength,
        UErrorCode*             status);






typedef enum UNumberFormatSymbol {
  
  UNUM_DECIMAL_SEPARATOR_SYMBOL = 0,
  
  UNUM_GROUPING_SEPARATOR_SYMBOL = 1,
  
  UNUM_PATTERN_SEPARATOR_SYMBOL = 2,
  
  UNUM_PERCENT_SYMBOL = 3,
  
  UNUM_ZERO_DIGIT_SYMBOL = 4,
  
  UNUM_DIGIT_SYMBOL = 5,
  
  UNUM_MINUS_SIGN_SYMBOL = 6,
  
  UNUM_PLUS_SIGN_SYMBOL = 7,
  
  UNUM_CURRENCY_SYMBOL = 8,
  
  UNUM_INTL_CURRENCY_SYMBOL = 9,
  
  UNUM_MONETARY_SEPARATOR_SYMBOL = 10,
  
  UNUM_EXPONENTIAL_SYMBOL = 11,
  
  UNUM_PERMILL_SYMBOL = 12,
  
  UNUM_PAD_ESCAPE_SYMBOL = 13,
  
  UNUM_INFINITY_SYMBOL = 14,
  
  UNUM_NAN_SYMBOL = 15,
  

  UNUM_SIGNIFICANT_DIGIT_SYMBOL = 16,
  


  UNUM_MONETARY_GROUPING_SEPARATOR_SYMBOL = 17,
  


  UNUM_ONE_DIGIT_SYMBOL = 18,
  


  UNUM_TWO_DIGIT_SYMBOL = 19,
  


  UNUM_THREE_DIGIT_SYMBOL = 20,
  


  UNUM_FOUR_DIGIT_SYMBOL = 21,
  


  UNUM_FIVE_DIGIT_SYMBOL = 22,
  


  UNUM_SIX_DIGIT_SYMBOL = 23,
  


  UNUM_SEVEN_DIGIT_SYMBOL = 24,
  


  UNUM_EIGHT_DIGIT_SYMBOL = 25,
  


  UNUM_NINE_DIGIT_SYMBOL = 26,

#ifndef U_HIDE_DRAFT_API
  


  UNUM_EXPONENT_MULTIPLICATION_SYMBOL = 27,
#endif  

  
  UNUM_FORMAT_SYMBOL_COUNT = 28
} UNumberFormatSymbol;

















U_STABLE int32_t U_EXPORT2
unum_getSymbol(const UNumberFormat *fmt,
               UNumberFormatSymbol symbol,
               UChar *buffer,
               int32_t size,
               UErrorCode *status);














U_STABLE void U_EXPORT2
unum_setSymbol(UNumberFormat *fmt,
               UNumberFormatSymbol symbol,
               const UChar *value,
               int32_t length,
               UErrorCode *status);











U_STABLE const char* U_EXPORT2
unum_getLocaleByType(const UNumberFormat *fmt,
                     ULocDataLocaleType type,
                     UErrorCode* status); 









U_STABLE void U_EXPORT2
unum_setContext(UNumberFormat* fmt, UDisplayContext value, UErrorCode* status);










U_STABLE UDisplayContext U_EXPORT2
unum_getContext(const UNumberFormat *fmt, UDisplayContextType type, UErrorCode* status);

#endif 

#endif
