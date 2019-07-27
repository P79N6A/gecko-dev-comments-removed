






















#ifndef SMPDTFMT_H
#define SMPDTFMT_H

#include "unicode/utypes.h"






#if !UCONFIG_NO_FORMATTING

#include "unicode/datefmt.h"
#include "unicode/udisplaycontext.h"
#include "unicode/tzfmt.h"  
#include "unicode/brkiter.h"

U_NAMESPACE_BEGIN

class DateFormatSymbols;
class DateFormat;
class MessageFormat;
class FieldPositionHandler;
class TimeZoneFormat;
class SharedNumberFormat;
class SimpleDateFormatMutableNFs;



























































































































































































































































































































































































































































































































































































































































































class U_I18N_API SimpleDateFormat: public DateFormat {
public:
    








    SimpleDateFormat(UErrorCode& status);

    










    SimpleDateFormat(const UnicodeString& pattern,
                     UErrorCode& status);

    




















    SimpleDateFormat(const UnicodeString& pattern,
                     const UnicodeString& override,
                     UErrorCode& status);

    











    SimpleDateFormat(const UnicodeString& pattern,
                     const Locale& locale,
                     UErrorCode& status);

    




















    SimpleDateFormat(const UnicodeString& pattern,
                     const UnicodeString& override,
                     const Locale& locale,
                     UErrorCode& status);

    








    SimpleDateFormat(const UnicodeString& pattern,
                     DateFormatSymbols* formatDataToAdopt,
                     UErrorCode& status);

    








    SimpleDateFormat(const UnicodeString& pattern,
                     const DateFormatSymbols& formatData,
                     UErrorCode& status);

    



    SimpleDateFormat(const SimpleDateFormat&);

    



    SimpleDateFormat& operator=(const SimpleDateFormat&);

    



    virtual ~SimpleDateFormat();

    





    virtual Format* clone(void) const;

    






    virtual UBool operator==(const Format& other) const;


    using DateFormat::format;

    















    virtual UnicodeString& format(  Calendar& cal,
                                    UnicodeString& appendTo,
                                    FieldPosition& pos) const;

    

















    virtual UnicodeString& format(  Calendar& cal,
                                    UnicodeString& appendTo,
                                    FieldPositionIterator* posIter,
                                    UErrorCode& status) const;

    using DateFormat::parse;

    

























    virtual void parse( const UnicodeString& text,
                        Calendar& cal,
                        ParsePosition& pos) const;


    















    virtual void set2DigitYearStart(UDate d, UErrorCode& status);

    














    UDate get2DigitYearStart(UErrorCode& status) const;

    





    virtual UnicodeString& toPattern(UnicodeString& result) const;

    















    virtual UnicodeString& toLocalizedPattern(UnicodeString& result,
                                              UErrorCode& status) const;

    







    virtual void applyPattern(const UnicodeString& pattern);

    









    virtual void applyLocalizedPattern(const UnicodeString& pattern,
                                       UErrorCode& status);

    







    virtual const DateFormatSymbols* getDateFormatSymbols(void) const;

    





    virtual void adoptDateFormatSymbols(DateFormatSymbols* newFormatSymbols);

    




    virtual void setDateFormatSymbols(const DateFormatSymbols& newFormatSymbols);

    










    static UClassID U_EXPORT2 getStaticClassID(void);

    










    virtual UClassID getDynamicClassID(void) const;

    








    virtual void adoptCalendar(Calendar* calendarToAdopt);

    
    






    virtual void adoptTimeZoneFormat(TimeZoneFormat* timeZoneFormatToAdopt);

    




    virtual void setTimeZoneFormat(const TimeZoneFormat& newTimeZoneFormat);

    




    virtual const TimeZoneFormat* getTimeZoneFormat(void) const;

    









    virtual void setContext(UDisplayContext value, UErrorCode& status);
    
#ifndef U_HIDE_DRAFT_API
    






    void adoptNumberFormat(NumberFormat *formatToAdopt);

    














    void adoptNumberFormat(const UnicodeString& fields, NumberFormat *formatToAdopt, UErrorCode &status);

    




    const NumberFormat * getNumberFormatForField(UChar field) const;
#endif  

#ifndef U_HIDE_INTERNAL_API
    









    UBool isFieldUnitIgnored(UCalendarDateFields field) const;


    










    static UBool isFieldUnitIgnored(const UnicodeString& pattern,
                                    UCalendarDateFields field);

    







    const Locale& getSmpFmtLocale(void) const;
#endif  

private:
    friend class DateFormat;

    void initializeDefaultCentury(void);

    void initializeBooleanAttributes(void);

    SimpleDateFormat(); 

    







    SimpleDateFormat(EStyle timeStyle, EStyle dateStyle, const Locale& locale, UErrorCode& status);

    







    SimpleDateFormat(const Locale& locale, UErrorCode& status); 

    


    UnicodeString& _format(Calendar& cal, UnicodeString& appendTo, FieldPositionHandler& handler, UErrorCode& status) const;

    















    void subFormat(UnicodeString &appendTo,
                   UChar ch,
                   int32_t count,
                   UDisplayContext capitalizationContext,
                   int32_t fieldNum,
                   FieldPositionHandler& handler,
                   Calendar& cal,
                   SimpleDateFormatMutableNFs &mutableNFs,
                   UErrorCode& status) const; 

    












    void zeroPaddingNumber(NumberFormat *currentNumberFormat,
                           UnicodeString &appendTo,
                           int32_t value,
                           int32_t minDigits,
                           int32_t maxDigits) const;

    



    static UBool isNumeric(UChar formatChar, int32_t count);

    


    static UBool isAtNumericField(const UnicodeString &pattern, int32_t patternOffset);

    


    static UBool isAfterNonNumericField(const UnicodeString &pattern, int32_t patternOffset);

    






    Calendar *initializeCalendar(TimeZone* adoptZone, const Locale& locale, UErrorCode& status);

    








    void construct(EStyle timeStyle, EStyle dateStyle, const Locale& locale, UErrorCode& status);

    





    void initialize(const Locale& locale, UErrorCode& status);

    












    int32_t matchString(const UnicodeString& text, int32_t start, UCalendarDateFields field,
                        const UnicodeString* stringArray, int32_t stringArrayCount,
                        const UnicodeString* monthPattern, Calendar& cal) const;

    











    int32_t matchQuarterString(const UnicodeString& text, int32_t start, UCalendarDateFields field,
                               const UnicodeString* stringArray, int32_t stringArrayCount, Calendar& cal) const;
    
    














    static UBool matchLiterals(const UnicodeString &pattern, int32_t &patternOffset,
                               const UnicodeString &text, int32_t &textOffset, 
                               UBool whitespaceLenient, UBool partialMatchLenient, UBool oldLeniency);
    
    



















    int32_t subParse(const UnicodeString& text, int32_t& start, UChar ch, int32_t count,
                     UBool obeyCount, UBool allowNegative, UBool ambiguousYear[], int32_t& saveHebrewMonth, Calendar& cal,
                     int32_t patLoc, MessageFormat * numericLeapMonthFormatter, UTimeZoneFormatTimeType *tzTimeType, SimpleDateFormatMutableNFs &mutableNFs) const;

    void parseInt(const UnicodeString& text,
                  Formattable& number,
                  ParsePosition& pos,
                  UBool allowNegative,
                  NumberFormat *fmt) const;

    void parseInt(const UnicodeString& text,
                  Formattable& number,
                  int32_t maxDigits,
                  ParsePosition& pos,
                  UBool allowNegative,
                  NumberFormat *fmt) const;

    int32_t checkIntSuffix(const UnicodeString& text, int32_t start,
                           int32_t patLoc, UBool isNegative) const;

    











    static void translatePattern(const UnicodeString& originalPattern,
                                UnicodeString& translatedPattern,
                                const UnicodeString& from,
                                const UnicodeString& to,
                                UErrorCode& status);

    






    void         parseAmbiguousDatesAsAfter(UDate startDate, UErrorCode& status);

    








    int32_t compareSimpleAffix(const UnicodeString& affix,
                   const UnicodeString& input,
                   int32_t pos) const;

    



    int32_t skipPatternWhiteSpace(const UnicodeString& text, int32_t pos) const;

    



    int32_t skipUWhiteSpace(const UnicodeString& text, int32_t pos) const;

    


    void initNumberFormatters(const Locale &locale,UErrorCode &status);

    


    void processOverrideString(const Locale &locale, const UnicodeString &str, int8_t type, UErrorCode &status);

    


    static const UCalendarDateFields fgPatternIndexToCalendarField[];

    


    static const UDateFormatField fgPatternIndexToDateFormatField[];

    


    TimeZoneFormat *tzFormat() const;

    const NumberFormat* getNumberFormatByIndex(UDateFormatField index) const;

    





    static const int32_t fgCalendarFieldToLevel[];

    


    static int32_t getLevelFromChar(UChar ch);

    


    static UBool isSyntaxChar(UChar ch);

    


    UnicodeString       fPattern;

    


    UnicodeString       fDateOverride;

    


    UnicodeString       fTimeOverride;


    


    Locale              fLocale;

    



    DateFormatSymbols*  fSymbols;   

    


    TimeZoneFormat* fTimeZoneFormat;

    







    UDate                fDefaultCenturyStart;

    


     int32_t   fDefaultCenturyStartYear;

    struct NSOverride : public UMemory {
        const SharedNumberFormat *snf;
        int32_t hash;
        NSOverride *next;
        void free();
        NSOverride() : snf(NULL), hash(0), next(NULL) {
        }
        ~NSOverride();
    };

    



    const SharedNumberFormat    **fSharedNumberFormatters;

    UBool fHaveDefaultCentury;

    BreakIterator* fCapitalizationBrkIter;
};

inline UDate
SimpleDateFormat::get2DigitYearStart(UErrorCode& ) const
{
    return fDefaultCenturyStart;
}

U_NAMESPACE_END

#endif 

#endif 

