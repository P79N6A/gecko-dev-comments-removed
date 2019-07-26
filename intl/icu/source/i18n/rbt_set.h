








#ifndef RBT_SET_H
#define RBT_SET_H

#include "unicode/utypes.h"

#if !UCONFIG_NO_TRANSLITERATION

#include "unicode/uobject.h"
#include "unicode/utrans.h"
#include "uvector.h"

U_NAMESPACE_BEGIN

class Replaceable;
class TransliterationRule;
class TransliterationRuleData;
class UnicodeFilter;
class UnicodeString;
class UnicodeSet;





class TransliterationRuleSet : public UMemory {
    






    UVector* ruleVector;

    





    TransliterationRule** rules;

    




    int32_t index[257];

    


    int32_t maxContextLength;

public:

    



    TransliterationRuleSet(UErrorCode& status);

    


    TransliterationRuleSet(const TransliterationRuleSet&);

    


    virtual ~TransliterationRuleSet();

    




    void setData(const TransliterationRuleData* data);

    



    virtual int32_t getMaximumContextLength(void) const;

    








    virtual void addRule(TransliterationRule* adoptedRule,
                         UErrorCode& status);

    












    virtual void freeze(UParseError& parseError, UErrorCode& status);
    
    












    UBool transliterate(Replaceable& text,
                        UTransPosition& index,
                        UBool isIncremental);

    






    virtual UnicodeString& toRules(UnicodeString& result,
                                   UBool escapeUnprintable) const;

    



    UnicodeSet& getSourceTargetSet(UnicodeSet& result,
                   UBool getTarget) const;

private:

    TransliterationRuleSet &operator=(const TransliterationRuleSet &other); 
};

U_NAMESPACE_END

#endif 

#endif
