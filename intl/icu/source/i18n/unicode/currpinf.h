





#ifndef CURRPINF_H
#define CURRPINF_H

#include "unicode/utypes.h"






#if !UCONFIG_NO_FORMATTING

#include "unicode/unistr.h"

U_NAMESPACE_BEGIN

class Locale;
class PluralRules;
class Hashtable;


















class  U_I18N_API CurrencyPluralInfo : public UObject {
public:

    




    CurrencyPluralInfo(UErrorCode& status);

    





    CurrencyPluralInfo(const Locale& locale, UErrorCode& status); 

    




    CurrencyPluralInfo(const CurrencyPluralInfo& info);


    




    CurrencyPluralInfo& operator=(const CurrencyPluralInfo& info);


    




    virtual ~CurrencyPluralInfo();


    




    UBool operator==(const CurrencyPluralInfo& info) const;


    




    UBool operator!=(const CurrencyPluralInfo& info) const;


    




    CurrencyPluralInfo* clone() const;


    





    const PluralRules* getPluralRules() const;

    








    UnicodeString& getCurrencyPluralPattern(const UnicodeString& pluralCount,
                                            UnicodeString& result) const; 

    





    const Locale& getLocale() const;

    










    void setPluralRules(const UnicodeString& ruleDescription,
                        UErrorCode& status);

    












    void setCurrencyPluralPattern(const UnicodeString& pluralCount, 
                                  const UnicodeString& pattern,
                                  UErrorCode& status);

    






    void setLocale(const Locale& loc, UErrorCode& status);

    




    virtual UClassID getDynamicClassID() const;

    




    static UClassID U_EXPORT2 getStaticClassID();

private:
    friend class DecimalFormat;

    void initialize(const Locale& loc, UErrorCode& status);
   
    void setupCurrencyPluralPattern(const Locale& loc, UErrorCode& status);

    




    void deleteHash(Hashtable* hTable);


    





    Hashtable* initHash(UErrorCode& status);



    






    void copyHash(const Hashtable* source, Hashtable* target, UErrorCode& status);

    
    
    
    
    
    
    
    
    
    Hashtable* fPluralCountToCurrencyUnitPattern;

    





    PluralRules* fPluralRules;

    
    Locale* fLocale;
};


inline UBool
CurrencyPluralInfo::operator!=(const CurrencyPluralInfo& info) const {              return !operator==(info);                                                   }  

U_NAMESPACE_END

#endif 

#endif 

