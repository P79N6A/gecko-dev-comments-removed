














#ifndef PLURRULE
#define PLURRULE

#include "unicode/utypes.h"






#if !UCONFIG_NO_FORMATTING

#include "unicode/format.h"
#include "unicode/upluralrules.h"






#define UPLRULES_NO_UNIQUE_VALUE ((double)-0.00123456777)

U_NAMESPACE_BEGIN

class Hashtable;
class FixedDecimal;
class RuleChain;
class PluralRuleParser;
class PluralKeywordEnumeration;
class AndConstraint;
class SharedPluralRules;
















































































































































class U_I18N_API PluralRules : public UObject {
public:

    






    PluralRules(UErrorCode& status);

    



    PluralRules(const PluralRules& other);

    



    virtual ~PluralRules();

    



    PluralRules* clone() const;

    



    PluralRules& operator=(const PluralRules&);

    









    static PluralRules* U_EXPORT2 createRules(const UnicodeString& description,
                                              UErrorCode& status);

    







    static PluralRules* U_EXPORT2 createDefaultRules(UErrorCode& status);

    















    static PluralRules* U_EXPORT2 forLocale(const Locale& locale, UErrorCode& status);

    















    static PluralRules* U_EXPORT2 forLocale(const Locale& locale, UPluralType type, UErrorCode& status);

#ifndef U_HIDE_INTERNAL_API
    




    static StringEnumeration* U_EXPORT2 getAvailableLocales(UErrorCode &status);

    





    static UBool hasOverride(const Locale &locale);

    




    static PluralRules* U_EXPORT2 internalForLocale(const Locale& locale, UPluralType type, UErrorCode& status);

    






    static const SharedPluralRules* U_EXPORT2 createSharedInstance(
            const Locale& locale, UPluralType type, UErrorCode& status);


#endif  

    








    UnicodeString select(int32_t number) const;

    








    UnicodeString select(double number) const;

#ifndef U_HIDE_INTERNAL_API
    


    UnicodeString select(const FixedDecimal &number) const;
#endif  

    









    StringEnumeration* getKeywords(UErrorCode& status) const;

#ifndef U_HIDE_DEPRECATED_API
    









    double getUniqueKeywordValue(const UnicodeString& keyword);

    



















    int32_t getAllKeywordValues(const UnicodeString &keyword,
                                double *dest, int32_t destCapacity,
                                UErrorCode& status);
#endif  

    

















    int32_t getSamples(const UnicodeString &keyword,
                       double *dest, int32_t destCapacity,
                       UErrorCode& status);

    








    UBool isKeyword(const UnicodeString& keyword) const;


    





    UnicodeString getKeywordOther() const;

#ifndef U_HIDE_INTERNAL_API
    



     UnicodeString getRules() const;
#endif  

    







    virtual UBool operator==(const PluralRules& other) const;

    







    UBool operator!=(const PluralRules& other) const  {return !operator==(other);}


    





    static UClassID U_EXPORT2 getStaticClassID(void);

    




    virtual UClassID getDynamicClassID() const;


private:
    RuleChain  *mRules;

    PluralRules();   
    void            parseDescription(const UnicodeString& ruleData, UErrorCode &status);
    int32_t         getNumberValue(const UnicodeString& token) const;
    UnicodeString   getRuleFromResource(const Locale& locale, UPluralType type, UErrorCode& status);
    RuleChain      *rulesForKeyword(const UnicodeString &keyword) const;

    friend class PluralRuleParser;
};

U_NAMESPACE_END

#endif 

#endif 

