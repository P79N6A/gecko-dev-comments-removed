






















#ifndef SMPDTFMT_H
#define SMPDTFMT_H

#include "unicode/utypes.h"






#if !UCONFIG_NO_FORMATTING

#include "unicode/datefmt.h"
#include "unicode/udisplaycontext.h"

U_NAMESPACE_BEGIN

class DateFormatSymbols;
class DateFormat;
class MessageFormat;
class FieldPositionHandler;
class TimeZoneFormat;





















































































































































































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

    


















    virtual UnicodeString& format(  const Formattable& obj,
                                    UnicodeString& appendTo,
                                    FieldPosition& pos,
                                    UErrorCode& status) const;

    



















    virtual UnicodeString& format(  const Formattable& obj,
                                    UnicodeString& appendTo,
                                    FieldPositionIterator* posIter,
                                    UErrorCode& status) const;

    









    UnicodeString& format(UDate date,
                          UnicodeString& appendTo,
                          FieldPosition& fieldPosition) const;

    











    UnicodeString& format(UDate date,
                          UnicodeString& appendTo,
                          FieldPositionIterator* posIter,
                          UErrorCode& status) const;

    








    UnicodeString& format(const Formattable& obj,
                          UnicodeString& appendTo,
                          UErrorCode& status) const;

    







    UnicodeString& format(UDate date, UnicodeString& appendTo) const;

    

























    virtual void parse( const UnicodeString& text,
                        Calendar& cal,
                        ParsePosition& pos) const;

    





























    UDate parse( const UnicodeString& text,
                 ParsePosition& pos) const;


    
































    virtual UDate parse( const UnicodeString& text,
                        UErrorCode& status) const;

    















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

    
    








    virtual void setContext(UDisplayContext value, UErrorCode& status);

    
    









    virtual UDisplayContext getContext(UDisplayContextType type, UErrorCode& status) const;

    
    






    virtual void adoptTimeZoneFormat(TimeZoneFormat* timeZoneFormatToAdopt);

    




    virtual void setTimeZoneFormat(const TimeZoneFormat& newTimeZoneFormat);

    




    virtual const TimeZoneFormat* getTimeZoneFormat(void) const;

#ifndef U_HIDE_INTERNAL_API
    









    UBool isFieldUnitIgnored(UCalendarDateFields field) const;


    










    static UBool isFieldUnitIgnored(const UnicodeString& pattern,
                                    UCalendarDateFields field);

    







    const Locale& getSmpFmtLocale(void) const;
#endif  

private:
    friend class DateFormat;

    void initializeDefaultCentury(void);

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

    





    void initializeSymbols(const Locale& locale, Calendar* calendar, UErrorCode& status);

    








    void construct(EStyle timeStyle, EStyle dateStyle, const Locale& locale, UErrorCode& status);

    





    void initialize(const Locale& locale, UErrorCode& status);

    












    int32_t matchString(const UnicodeString& text, int32_t start, UCalendarDateFields field,
                        const UnicodeString* stringArray, int32_t stringArrayCount,
                        const UnicodeString* monthPattern, Calendar& cal) const;

    











    int32_t matchQuarterString(const UnicodeString& text, int32_t start, UCalendarDateFields field,
                               const UnicodeString* stringArray, int32_t stringArrayCount, Calendar& cal) const;
    
    












    static UBool matchLiterals(const UnicodeString &pattern, int32_t &patternOffset,
                               const UnicodeString &text, int32_t &textOffset, UBool lenient);
    
    

















    int32_t subParse(const UnicodeString& text, int32_t& start, UChar ch, int32_t count,
                     UBool obeyCount, UBool allowNegative, UBool ambiguousYear[], int32_t& saveHebrewMonth, Calendar& cal,
                     int32_t patLoc, MessageFormat * numericLeapMonthFormatter) const;

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

    


     NumberFormat * getNumberFormatByIndex(UDateFormatField index) const;

    


    void processOverrideString(const Locale &locale, const UnicodeString &str, int8_t type, UErrorCode &status);

    


    static const UCalendarDateFields fgPatternIndexToCalendarField[];

    


    static const UDateFormatField fgPatternIndexToDateFormatField[];

    


    TimeZoneFormat *tzFormat() const;

    





    static const int32_t fgCalendarFieldToLevel[];
    static const int32_t fgPatternCharToLevel[];

    


    UnicodeString       fPattern;

    


    UnicodeString       fDateOverride;

    


    UnicodeString       fTimeOverride;


    


    Locale              fLocale;

    



    DateFormatSymbols*  fSymbols;   

    


    TimeZoneFormat* fTimeZoneFormat;

    







    UDate                fDefaultCenturyStart;

    


     int32_t   fDefaultCenturyStartYear;

    int32_t tztype; 

    typedef struct NSOverride {
        NumberFormat *nf;
        int32_t hash;
        NSOverride *next;
    } NSOverride;

    NumberFormat    **fNumberFormatters;

    NSOverride      *fOverrideList;

    UBool fHaveDefaultCentury;

    UDisplayContext fCapitalizationContext;
};

inline UDate
SimpleDateFormat::get2DigitYearStart(UErrorCode& ) const
{
    return fDefaultCenturyStart;
}

inline UnicodeString&
SimpleDateFormat::format(const Formattable& obj,
                         UnicodeString& appendTo,
                         UErrorCode& status) const {
    
    
    return DateFormat::format(obj, appendTo, status);
}

inline UnicodeString&
SimpleDateFormat::format(const Formattable& obj,
                         UnicodeString& appendTo,
                         FieldPosition& pos,
                         UErrorCode& status) const
{
    
    
    return DateFormat::format(obj, appendTo, pos, status);
}

inline UnicodeString&
SimpleDateFormat::format(const Formattable& obj,
                         UnicodeString& appendTo,
                         FieldPositionIterator* posIter,
                         UErrorCode& status) const
{
    
    
    return DateFormat::format(obj, appendTo, posIter, status);
}

inline UnicodeString&
SimpleDateFormat::format(UDate date,
                         UnicodeString& appendTo,
                         FieldPosition& fieldPosition) const {
    
    
    return DateFormat::format(date, appendTo, fieldPosition);
}

inline UnicodeString&
SimpleDateFormat::format(UDate date,
                         UnicodeString& appendTo,
                         FieldPositionIterator* posIter,
                         UErrorCode& status) const {
    
    
    return DateFormat::format(date, appendTo, posIter, status);
}

inline UnicodeString&
SimpleDateFormat::format(UDate date, UnicodeString& appendTo) const {
    return DateFormat::format(date, appendTo);
}

U_NAMESPACE_END

#endif 

#endif 

