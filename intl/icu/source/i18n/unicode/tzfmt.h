





#ifndef __TZFMT_H
#define __TZFMT_H






#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/format.h"
#include "unicode/timezone.h"
#include "unicode/tznames.h"

U_CDECL_BEGIN





typedef enum UTimeZoneFormatStyle {
    



    UTZFMT_STYLE_GENERIC_LOCATION,
    



    UTZFMT_STYLE_GENERIC_LONG,
    



    UTZFMT_STYLE_GENERIC_SHORT,
    



    UTZFMT_STYLE_SPECIFIC_LONG,
    



    UTZFMT_STYLE_SPECIFIC_SHORT,
    



    UTZFMT_STYLE_LOCALIZED_GMT,
    




    UTZFMT_STYLE_LOCALIZED_GMT_SHORT,
    





    UTZFMT_STYLE_ISO_BASIC_SHORT,
    





    UTZFMT_STYLE_ISO_BASIC_LOCAL_SHORT,
    





    UTZFMT_STYLE_ISO_BASIC_FIXED,
    





    UTZFMT_STYLE_ISO_BASIC_LOCAL_FIXED,
    





    UTZFMT_STYLE_ISO_BASIC_FULL,
    





    UTZFMT_STYLE_ISO_BASIC_LOCAL_FULL,
    





    UTZFMT_STYLE_ISO_EXTENDED_FIXED,
    





    UTZFMT_STYLE_ISO_EXTENDED_LOCAL_FIXED,
    





    UTZFMT_STYLE_ISO_EXTENDED_FULL,
    





    UTZFMT_STYLE_ISO_EXTENDED_LOCAL_FULL,
    



    UTZFMT_STYLE_ZONE_ID,
    



    UTZFMT_STYLE_ZONE_ID_SHORT,
    



    UTZFMT_STYLE_EXEMPLAR_LOCATION
} UTimeZoneFormatStyle;





typedef enum UTimeZoneFormatGMTOffsetPatternType {
    



    UTZFMT_PAT_POSITIVE_HM,
    



    UTZFMT_PAT_POSITIVE_HMS,
    



    UTZFMT_PAT_NEGATIVE_HM,
    



    UTZFMT_PAT_NEGATIVE_HMS,
    



    UTZFMT_PAT_POSITIVE_H,
    



    UTZFMT_PAT_NEGATIVE_H,

    
    



    UTZFMT_PAT_COUNT = 6
} UTimeZoneFormatGMTOffsetPatternType;






typedef enum UTimeZoneFormatTimeType {
    



    UTZFMT_TIME_TYPE_UNKNOWN,
    



    UTZFMT_TIME_TYPE_STANDARD,
    



    UTZFMT_TIME_TYPE_DAYLIGHT
} UTimeZoneFormatTimeType;





typedef enum UTimeZoneFormatParseOption {
    



    UTZFMT_PARSE_OPTION_NONE        = 0x00,
    





    UTZFMT_PARSE_OPTION_ALL_STYLES  = 0x01
#ifndef U_HIDE_DRAFT_API
    ,
     






    UTZFMT_PARSE_OPTION_TZ_DATABASE_ABBREVIATIONS = 0x02
#endif  
} UTimeZoneFormatParseOption;

U_CDECL_END

U_NAMESPACE_BEGIN

class TimeZoneGenericNames;
class TZDBTimeZoneNames;
class UVector;
















class U_I18N_API TimeZoneFormat : public Format {
public:
    



    TimeZoneFormat(const TimeZoneFormat& other);

    



    virtual ~TimeZoneFormat();

    



    TimeZoneFormat& operator=(const TimeZoneFormat& other);

    







    virtual UBool operator==(const Format& other) const;

    





    virtual Format* clone() const;

    







    static TimeZoneFormat* U_EXPORT2 createInstance(const Locale& locale, UErrorCode& status);

    




    const TimeZoneNames* getTimeZoneNames() const;

    






    void adoptTimeZoneNames(TimeZoneNames *tznames);

    




    void setTimeZoneNames(const TimeZoneNames &tznames);

    






    UnicodeString& getGMTPattern(UnicodeString& pattern) const;

    







    void setGMTPattern(const UnicodeString& pattern, UErrorCode& status);

    







    UnicodeString& getGMTOffsetPattern(UTimeZoneFormatGMTOffsetPatternType type, UnicodeString& pattern) const;

    







    void setGMTOffsetPattern(UTimeZoneFormatGMTOffsetPatternType type, const UnicodeString& pattern, UErrorCode& status);

    








    UnicodeString& getGMTOffsetDigits(UnicodeString& digits) const;

    











    void setGMTOffsetDigits(const UnicodeString& digits, UErrorCode& status);

    






    UnicodeString& getGMTZeroFormat(UnicodeString& gmtZeroFormat) const;

    






    void setGMTZeroFormat(const UnicodeString& gmtZeroFormat, UErrorCode& status);

    






    uint32_t getDefaultParseOptions(void) const;

    







    void setDefaultParseOptions(uint32_t flags);

    














    UnicodeString& formatOffsetISO8601Basic(int32_t offset, UBool useUtcIndicator, UBool isShort, UBool ignoreSeconds,
        UnicodeString& result, UErrorCode& status) const;

    














    UnicodeString& formatOffsetISO8601Extended(int32_t offset, UBool useUtcIndicator, UBool isShort, UBool ignoreSeconds,
        UnicodeString& result, UErrorCode& status) const;

    


















    UnicodeString& formatOffsetLocalizedGMT(int32_t offset, UnicodeString& result, UErrorCode& status) const;

    


















    UnicodeString& formatOffsetShortLocalizedGMT(int32_t offset, UnicodeString& result, UErrorCode& status) const;

    using Format::format;

    












    virtual UnicodeString& format(UTimeZoneFormatStyle style, const TimeZone& tz, UDate date,
        UnicodeString& name, UTimeZoneFormatTimeType* timeType = NULL) const;

    













    int32_t parseOffsetISO8601(const UnicodeString& text, ParsePosition& pos) const;

    











    int32_t parseOffsetLocalizedGMT(const UnicodeString& text, ParsePosition& pos) const;

    











    int32_t parseOffsetShortLocalizedGMT(const UnicodeString& text, ParsePosition& pos) const;

    















    virtual TimeZone* parse(UTimeZoneFormatStyle style, const UnicodeString& text, ParsePosition& pos,
        int32_t parseOptions, UTimeZoneFormatTimeType* timeType = NULL) const;

    














    TimeZone* parse(UTimeZoneFormatStyle style, const UnicodeString& text, ParsePosition& pos,
        UTimeZoneFormatTimeType* timeType = NULL) const;

    



    










    virtual UnicodeString& format(const Formattable& obj, UnicodeString& appendTo,
        FieldPosition& pos, UErrorCode& status) const;

    











    virtual void parseObject(const UnicodeString& source, Formattable& result, ParsePosition& parse_pos) const;

    



    static UClassID U_EXPORT2 getStaticClassID(void);

    



    virtual UClassID getDynamicClassID() const;

protected:
    





    TimeZoneFormat(const Locale& locale, UErrorCode& status);

private:
    
    Locale fLocale;

    
    char fTargetRegion[ULOC_COUNTRY_CAPACITY];

    
    TimeZoneNames* fTimeZoneNames;

    
    TimeZoneGenericNames* fTimeZoneGenericNames;

    
    UnicodeString fGMTPattern;

    
    UnicodeString fGMTOffsetPatterns[UTZFMT_PAT_COUNT];

    
    UChar32 fGMTOffsetDigits[10];

    
    UnicodeString fGMTZeroFormat;

    
    uint32_t fDefParseOptionFlags;

    
    UnicodeString fGMTPatternPrefix;    
    UnicodeString fGMTPatternSuffix;    

    
    UVector* fGMTOffsetPatternItems[UTZFMT_PAT_COUNT];

    UBool fAbuttingOffsetHoursAndMinutes;

    
    TZDBTimeZoneNames* fTZDBTimeZoneNames;

    









    UnicodeString& formatSpecific(const TimeZone& tz, UTimeZoneNameType stdType, UTimeZoneNameType dstType,
        UDate date, UnicodeString& name, UTimeZoneFormatTimeType *timeType) const;

    







    UnicodeString& formatGeneric(const TimeZone& tz, int32_t genType, UDate date, UnicodeString& name) const;

    




    const TimeZoneGenericNames* getTimeZoneGenericNames(UErrorCode& status) const;

    




    const TZDBTimeZoneNames* getTZDBTimeZoneNames(UErrorCode& status) const;

    






    UnicodeString& formatExemplarLocation(const TimeZone& tz, UnicodeString& name) const;

    


    enum OffsetFields {
        FIELDS_H,
        FIELDS_HM,
        FIELDS_HMS
    };

    






    void initGMTPattern(const UnicodeString& gmtPattern, UErrorCode& status);

    







    static UVector* parseOffsetPattern(const UnicodeString& pattern, OffsetFields required, UErrorCode& status);

    







    static UnicodeString& expandOffsetPattern(const UnicodeString& offsetHM, UnicodeString& result, UErrorCode& status);

    







    static UnicodeString& truncateOffsetPattern(const UnicodeString& offsetHM, UnicodeString& result, UErrorCode& status);

    









    static UBool toCodePoints(const UnicodeString& str, UChar32* codeArray, int32_t capacity);

    









    UnicodeString& formatOffsetISO8601(int32_t offset, UBool isBasic, UBool useUtcIndicator,
        UBool isShort, UBool ignoreSeconds, UnicodeString& result, UErrorCode& status) const;

    






    UnicodeString& formatOffsetLocalizedGMT(int32_t offset, UBool isShort, UnicodeString& result, UErrorCode& status) const;

    













    int32_t parseOffsetISO8601(const UnicodeString& text, ParsePosition& pos, UBool extendedOnly,
        UBool* hasDigitOffset = NULL) const;

    






    void appendOffsetDigits(UnicodeString& buf, int32_t n, uint8_t minDigits) const;

    











    int32_t parseOffsetLocalizedGMT(const UnicodeString& text, ParsePosition& pos,
        UBool isShort, UBool* hasDigitOffset) const;

    








    int32_t parseOffsetLocalizedGMTPattern(const UnicodeString& text, int32_t start,
        UBool isShort, int32_t& parsedLen) const;

    







    int32_t parseOffsetFields(const UnicodeString& text, int32_t start, UBool isShort, int32_t& parsedLen) const;

    










    int32_t parseOffsetFieldsWithPattern(const UnicodeString& text, int32_t start,
        UVector* patternItems, UBool forceSingleHourDigit, int32_t& hour, int32_t& min, int32_t& sec) const;

    






    int32_t parseAbuttingOffsetFields(const UnicodeString& text, int32_t start, int32_t& parsedLen) const;

    






    int32_t parseOffsetDefaultLocalizedGMT(const UnicodeString& text, int start, int32_t& parsedLen) const;

    







    int32_t parseDefaultOffsetFields(const UnicodeString& text, int32_t start, UChar separator,
        int32_t& parsedLen) const;

    













    int32_t parseOffsetFieldWithLocalizedDigits(const UnicodeString& text, int32_t start,
        uint8_t minDigits, uint8_t maxDigits, uint16_t minVal, uint16_t maxVal, int32_t& parsedLen) const;

    








    int32_t parseSingleLocalizedDigit(const UnicodeString& text, int32_t start, int32_t& len) const;

    








    static UnicodeString& formatOffsetWithAsciiDigits(int32_t offset, UChar sep,
        OffsetFields minFields, OffsetFields maxFields, UnicodeString& result);

    











    static int32_t parseAbuttingAsciiOffsetFields(const UnicodeString& text, ParsePosition& pos,
        OffsetFields minFields, OffsetFields maxFields, UBool fixedHourWidth);

    











    static int32_t parseAsciiOffsetFields(const UnicodeString& text, ParsePosition& pos, UChar sep,
        OffsetFields minFields, OffsetFields maxFields);

    





    static UnicodeString& unquote(const UnicodeString& pattern, UnicodeString& result);

    




    void initGMTOffsetPatterns(UErrorCode& status);

    





    void checkAbuttingHoursAndMinutes();

    




    TimeZone* createTimeZoneForOffset(int32_t offset) const;

    




    static UTimeZoneFormatTimeType getTimeType(UTimeZoneNameType nameType);

    







    UnicodeString& getTimeZoneID(const TimeZoneNames::MatchInfoCollection* matches, int32_t idx, UnicodeString& tzID) const;


    






    UnicodeString& parseZoneID(const UnicodeString& text, ParsePosition& pos, UnicodeString& tzID) const;

    






    UnicodeString& parseShortZoneID(const UnicodeString& text, ParsePosition& pos, UnicodeString& tzID) const;

    






    UnicodeString& parseExemplarLocation(const UnicodeString& text, ParsePosition& pos, UnicodeString& tzID) const;
};

U_NAMESPACE_END

#endif 
#endif
