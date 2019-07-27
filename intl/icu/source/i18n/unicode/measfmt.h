









#ifndef MEASUREFORMAT_H
#define MEASUREFORMAT_H

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/format.h"
#include "unicode/udat.h"














enum UMeasureFormatWidth {

    
    



    UMEASFMT_WIDTH_WIDE,
 
    



    UMEASFMT_WIDTH_SHORT,

    



    UMEASFMT_WIDTH_NARROW,

    




    UMEASFMT_WIDTH_NUMERIC,

    



    UMEASFMT_WIDTH_COUNT = 4
};

typedef enum UMeasureFormatWidth UMeasureFormatWidth; 

U_NAMESPACE_BEGIN

class Measure;
class MeasureUnit;
class NumberFormat;
class PluralRules;
class MeasureFormatCacheData;
class SharedNumberFormat;
class SharedPluralRules;
class QuantityFormatter;
class SimplePatternFormatter;
class ListFormatter;
class DateFormat;









class U_I18N_API MeasureFormat : public Format {
 public:
    using Format::parseObject;
    using Format::format;

    



    MeasureFormat(
            const Locale &locale, UMeasureFormatWidth width, UErrorCode &status);

    



    MeasureFormat(
            const Locale &locale,
            UMeasureFormatWidth width,
            NumberFormat *nfToAdopt,
            UErrorCode &status);

    



    MeasureFormat(const MeasureFormat &other);

    



    MeasureFormat &operator=(const MeasureFormat &rhs);

    



    virtual ~MeasureFormat();

    



    virtual UBool operator==(const Format &other) const;

    



    virtual Format *clone() const;

    



    virtual UnicodeString &format(
            const Formattable &obj,
            UnicodeString &appendTo,
            FieldPosition &pos,
            UErrorCode &status) const;

    





    virtual void parseObject(
            const UnicodeString &source,
            Formattable &reslt,
            ParsePosition &pos) const;

    















    UnicodeString &formatMeasures(
            const Measure *measures,
            int32_t measureCount,
            UnicodeString &appendTo,
            FieldPosition &pos,
            UErrorCode &status) const;

#ifndef U_HIDE_DRAFT_API
    












    UnicodeString &formatMeasurePerUnit(
            const Measure &measure,
            const MeasureUnit &perUnit,
            UnicodeString &appendTo,
            FieldPosition &pos,
            UErrorCode &status) const;

#endif  

    







    static MeasureFormat* U_EXPORT2 createCurrencyFormat(const Locale& locale,
                                               UErrorCode& ec);

    






    static MeasureFormat* U_EXPORT2 createCurrencyFormat(UErrorCode& ec);

    










    static UClassID U_EXPORT2 getStaticClassID(void);

    










    virtual UClassID getDynamicClassID(void) const;

 protected:
    



    MeasureFormat();

#ifndef U_HIDE_INTERNAL_API 

    




    void initMeasureFormat(
            const Locale &locale,
            UMeasureFormatWidth width,
            NumberFormat *nfToAdopt,
            UErrorCode &status);
    






    UBool setMeasureFormatLocale(const Locale &locale, UErrorCode &status);

    




    void adoptNumberFormat(NumberFormat *nfToAdopt, UErrorCode &status);

    



    const NumberFormat &getNumberFormat() const;

    



    const PluralRules &getPluralRules() const;

    



    Locale getLocale(UErrorCode &status) const;

    



    const char *getLocaleID(UErrorCode &status) const;

#endif 

 private:
    const MeasureFormatCacheData *cache;
    const SharedNumberFormat *numberFormat;
    const SharedPluralRules *pluralRules;
    UMeasureFormatWidth width;    

    
    
    
    ListFormatter *listFormatter;

    const QuantityFormatter *getQuantityFormatter(
            int32_t index,
            int32_t widthIndex,
            UErrorCode &status) const;

    const SimplePatternFormatter *getPerUnitFormatter(
            int32_t index,
            int32_t widthIndex) const;

    const SimplePatternFormatter *getPerFormatter(
            int32_t widthIndex,
            UErrorCode &status) const;

    int32_t withPerUnitAndAppend(
        const UnicodeString &formatted,
        const MeasureUnit &perUnit,
        UnicodeString &appendTo,
        UErrorCode &status) const;

    UnicodeString &formatMeasure(
        const Measure &measure,
        const NumberFormat &nf,
        UnicodeString &appendTo,
        FieldPosition &pos,
        UErrorCode &status) const;

    UnicodeString &formatMeasuresSlowTrack(
        const Measure *measures,
        int32_t measureCount,
        UnicodeString& appendTo,
        FieldPosition& pos,
        UErrorCode& status) const;

    UnicodeString &formatNumeric(
        const Formattable *hms,  
                                 
        int32_t bitMap,   
        UnicodeString &appendTo,
        UErrorCode &status) const;

    UnicodeString &formatNumeric(
        UDate date,
        const DateFormat &dateFmt,
        UDateFormatField smallestField,
        const Formattable &smallestAmount,
        UnicodeString &appendTo,
        UErrorCode &status) const;
};

U_NAMESPACE_END

#endif 
#endif 
