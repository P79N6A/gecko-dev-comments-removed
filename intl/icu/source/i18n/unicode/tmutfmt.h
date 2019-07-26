






#ifndef __TMUTFMT_H__
#define __TMUTFMT_H__

#include "unicode/utypes.h"







#if !UCONFIG_NO_FORMATTING

#include "unicode/unistr.h"
#include "unicode/tmunit.h"
#include "unicode/tmutamt.h"
#include "unicode/measfmt.h"
#include "unicode/numfmt.h"
#include "unicode/plurrule.h"








enum UTimeUnitFormatStyle {
    
    UTMUTFMT_FULL_STYLE,
    
    UTMUTFMT_ABBREVIATED_STYLE,
    
    UTMUTFMT_FORMAT_STYLE_COUNT
};
typedef enum UTimeUnitFormatStyle UTimeUnitFormatStyle; 

U_NAMESPACE_BEGIN

class Hashtable;
class UVector;































class U_I18N_API TimeUnitFormat: public MeasureFormat {
public:

    




    TimeUnitFormat(UErrorCode& status);

    



    TimeUnitFormat(const Locale& locale, UErrorCode& status);

    



    TimeUnitFormat(const Locale& locale, UTimeUnitFormatStyle style, UErrorCode& status);

    



    TimeUnitFormat(const TimeUnitFormat&);

    



    virtual ~TimeUnitFormat();

    





    virtual Format* clone(void) const;

    



    TimeUnitFormat& operator=(const TimeUnitFormat& other);


    






    virtual UBool operator==(const Format& other) const;

    






    UBool operator!=(const Format& other) const;

    





    void setLocale(const Locale& locale, UErrorCode& status);


    





    void setNumberFormat(const NumberFormat& format, UErrorCode& status);


    using MeasureFormat::format;

    







    virtual UnicodeString& format(const Formattable& obj,
                                  UnicodeString& toAppendTo,
                                  FieldPosition& pos,
                                  UErrorCode& status) const;

    




    virtual void parseObject(const UnicodeString& source,
                             Formattable& result,
                             ParsePosition& pos) const;

    










    static UClassID U_EXPORT2 getStaticClassID(void);

    










    virtual UClassID getDynamicClassID(void) const;

private:
    NumberFormat* fNumberFormat;
    Locale        fLocale;
    Hashtable*    fTimeUnitToCountToPatterns[TimeUnit::UTIMEUNIT_FIELD_COUNT];
    PluralRules*  fPluralRules;
    UTimeUnitFormatStyle fStyle;

    void create(const Locale& locale, UTimeUnitFormatStyle style, UErrorCode& status);

    
    
    void setup(UErrorCode& status);

    
    void initDataMembers(UErrorCode& status);

    
    void readFromCurrentLocale(UTimeUnitFormatStyle style, const char* key, const UVector& pluralCounts,
                               UErrorCode& status);

    
    
    void checkConsistency(UTimeUnitFormatStyle style, const char* key, UErrorCode& status);

    
    void searchInLocaleChain(UTimeUnitFormatStyle style, const char* key, const char* localeName,
                             TimeUnit::UTimeUnitFields field, const UnicodeString&,
                             const char*, Hashtable*, UErrorCode&);

    
    Hashtable* initHash(UErrorCode& status);

    
    void deleteHash(Hashtable* htable);

    
    void copyHash(const Hashtable* source, Hashtable* target, UErrorCode& status);
    
    
    static const char* getTimeUnitName(TimeUnit::UTimeUnitFields field, UErrorCode& status);
};



inline UBool
TimeUnitFormat::operator!=(const Format& other) const  {
    return !operator==(other);
}



U_NAMESPACE_END

#endif 

#endif 

