






#ifndef RELDTFMT_H
#define RELDTFMT_H

#include "unicode/utypes.h"






#if !UCONFIG_NO_FORMATTING

#include "unicode/datefmt.h"
#include "unicode/smpdtfmt.h"

U_NAMESPACE_BEGIN


class DateFormatSymbols;
class MessageFormat;


struct URelativeString;











class RelativeDateFormat : public DateFormat {
public:
    RelativeDateFormat( UDateFormatStyle timeStyle, UDateFormatStyle dateStyle, const Locale& locale, UErrorCode& status);

    
    



    RelativeDateFormat(const RelativeDateFormat&);

    



    RelativeDateFormat& operator=(const RelativeDateFormat&);

    



    virtual ~RelativeDateFormat();

    





    virtual Format* clone(void) const;

    






    virtual UBool operator==(const Format& other) const;


    using DateFormat::format;

    















    virtual UnicodeString& format(  Calendar& cal,
                                    UnicodeString& appendTo,
                                    FieldPosition& pos) const;

    













    virtual UnicodeString& format(const Formattable& obj,
                                  UnicodeString& appendTo,
                                  FieldPosition& pos,
                                  UErrorCode& status) const;


    


















    virtual void parse( const UnicodeString& text,
                        Calendar& cal,
                        ParsePosition& pos) const;

    


















    UDate parse( const UnicodeString& text,
                 ParsePosition& pos) const;


    














    virtual UDate parse( const UnicodeString& text,
                        UErrorCode& status) const;

    






    virtual UnicodeString& toPattern(UnicodeString& result, UErrorCode& status) const;

    





    virtual UnicodeString& toPatternDate(UnicodeString& result, UErrorCode& status) const;

    





    virtual UnicodeString& toPatternTime(UnicodeString& result, UErrorCode& status) const;

    








    virtual void applyPatterns(const UnicodeString& datePattern, const UnicodeString& timePattern, UErrorCode &status);

    







    virtual const DateFormatSymbols* getDateFormatSymbols(void) const;


private:
    SimpleDateFormat *fDateTimeFormatter;
    UnicodeString fDatePattern;
    UnicodeString fTimePattern;
    MessageFormat *fCombinedFormat; 

    UDateFormatStyle fDateStyle;
    Locale  fLocale;

    int32_t fDayMin;    
    int32_t fDayMax;    
    int32_t fDatesLen;    
    URelativeString *fDates; 


    





    const UChar *getStringForDay(int32_t day, int32_t &len, UErrorCode &status) const;

    


    void loadDates(UErrorCode &status);

    


    static int32_t dayDifference(Calendar &until, UErrorCode &status);

    







    Calendar* initializeCalendar(TimeZone* adoptZone, const Locale& locale, UErrorCode& status);

public:
    










    U_I18N_API static UClassID U_EXPORT2 getStaticClassID(void);

    










    virtual UClassID getDynamicClassID(void) const;
};


U_NAMESPACE_END

#endif 

#endif 

