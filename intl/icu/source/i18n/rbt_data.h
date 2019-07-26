








#ifndef RBT_DATA_H
#define RBT_DATA_H

#include "unicode/utypes.h"
#include "unicode/uclean.h"

#if !UCONFIG_NO_TRANSLITERATION

#include "unicode/uobject.h"
#include "rbt_set.h"
#include "hash.h"

U_NAMESPACE_BEGIN

class UnicodeFunctor;
class UnicodeMatcher;
class UnicodeReplacer;


















class TransliterationRuleData : public UMemory {

public:

    

    


    TransliterationRuleSet ruleSet;

    








    Hashtable variableNames;

    








    UnicodeFunctor** variables;

    






    UBool variablesAreOwned;

    




    UChar variablesBase;

    


    int32_t variablesLength;

public:

    



    TransliterationRuleData(UErrorCode& status);

    


    TransliterationRuleData(const TransliterationRuleData&);

    


    ~TransliterationRuleData();

    





    UnicodeFunctor* lookup(UChar32 standIn) const;

    






    UnicodeMatcher* lookupMatcher(UChar32 standIn) const;

    






    UnicodeReplacer* lookupReplacer(UChar32 standIn) const;


private:
    TransliterationRuleData &operator=(const TransliterationRuleData &other); 
};

U_NAMESPACE_END

#endif 

#endif
