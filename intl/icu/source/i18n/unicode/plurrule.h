














#ifndef PLURRULE
#define PLURRULE

#include "unicode/utypes.h"






#if !UCONFIG_NO_FORMATTING

#include "unicode/format.h"
#include "unicode/upluralrules.h"






#define UPLRULES_NO_UNIQUE_VALUE ((double)-0.00123456777)

U_NAMESPACE_BEGIN

class Hashtable;
class RuleChain;
class RuleParser;
class PluralKeywordEnumeration;



















































































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

    








    UnicodeString select(int32_t number) const;

    








    UnicodeString select(double number) const;

    









    StringEnumeration* getKeywords(UErrorCode& status) const;

    









    double getUniqueKeywordValue(const UnicodeString& keyword);

    

















    int32_t getAllKeywordValues(const UnicodeString &keyword,
                                double *dest, int32_t destCapacity,
                                UErrorCode& status);

    

















    int32_t getSamples(const UnicodeString &keyword,
                       double *dest, int32_t destCapacity,
                       UErrorCode& status);

    








    UBool isKeyword(const UnicodeString& keyword) const;


    





    UnicodeString getKeywordOther() const;

    







    virtual UBool operator==(const PluralRules& other) const;

    







    UBool operator!=(const PluralRules& other) const  {return !operator==(other);}


    





    static UClassID U_EXPORT2 getStaticClassID(void);

    




    virtual UClassID getDynamicClassID() const;


private:
    RuleChain  *mRules;
    RuleParser *mParser;
    double     *mSamples;
    int32_t    *mSampleInfo;
    int32_t    mSampleInfoCount;

    PluralRules();   
    int32_t getRepeatLimit() const;
    void parseDescription(UnicodeString& ruleData, RuleChain& rules, UErrorCode &status);
    void getNextLocale(const UnicodeString& localeData, int32_t* curIndex, UnicodeString& localeName);
    void addRules(RuleChain& rules);
    int32_t getNumberValue(const UnicodeString& token) const;
    UnicodeString getRuleFromResource(const Locale& locale, UPluralType type, UErrorCode& status);

    static const int32_t MAX_SAMPLES = 3;

    int32_t getSamplesInternal(const UnicodeString &keyword, double *dest,
                               int32_t destCapacity, UBool includeUnlimited,
                               UErrorCode& status);
    int32_t getKeywordIndex(const UnicodeString& keyword,
                            UErrorCode& status) const;
    void initSamples(UErrorCode& status);

};

U_NAMESPACE_END

#endif 

#endif 

