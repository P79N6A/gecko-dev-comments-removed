








#ifndef RBT_PARS_H
#define RBT_PARS_H

#include "unicode/utypes.h"

#if !UCONFIG_NO_TRANSLITERATION
#ifdef __cplusplus

#include "unicode/uobject.h"
#include "unicode/parseerr.h"
#include "unicode/unorm.h"
#include "rbt.h"
#include "hash.h"
#include "uvector.h"

U_NAMESPACE_BEGIN

class TransliterationRuleData;
class UnicodeFunctor;
class ParseData;
class RuleHalf;
class ParsePosition;
class StringMatcher;

class TransliteratorParser : public UMemory {

 public:

    



    UVector dataVector;

    



    UVector idBlockVector;

    


    UnicodeSet* compoundFilter;

 private:

    


    TransliterationRuleData* curData;

    UTransDirection direction;

    


    UParseError parseError;

    


    ParseData* parseData;

    




    UVector variablesVector;

    



    Hashtable variableNames;    
    
    




    UnicodeString segmentStandins;

    





    UVector segmentObjects;

    





    UChar variableNext;

    




    UChar variableLimit;

    






    UnicodeString undefinedVariableName;

    




    UChar dotStandIn;

public:

    


    TransliteratorParser(UErrorCode &statusReturn);

    


    ~TransliteratorParser();

    















    void parse(const UnicodeString& rules,
               UTransDirection direction,
               UParseError& pe,
               UErrorCode& ec);

    


 
    UnicodeSet* orphanCompoundFilter();

private:

    




    void parseRules(const UnicodeString& rules,
                    UTransDirection direction,
                    UErrorCode& status);

    
















    int32_t parseRule(const UnicodeString& rule, int32_t pos, int32_t limit, UErrorCode& status);

    




    void setVariableRange(int32_t start, int32_t end, UErrorCode& status);

    






    UBool checkVariableRange(UChar32 ch) const;

    




    void pragmaMaximumBackup(int32_t backup);

    




    void pragmaNormalizeRules(UNormalizationMode mode);

    






    static UBool resemblesPragma(const UnicodeString& rule, int32_t pos, int32_t limit);

    








    int32_t parsePragma(const UnicodeString& rule, int32_t pos, int32_t limit, UErrorCode& status);

    









    int32_t syntaxError(UErrorCode parseErrorCode, const UnicodeString&, int32_t start,
                        UErrorCode& status);

    







    UChar parseSet(const UnicodeString& rule,
                   ParsePosition& pos,
                   UErrorCode& status);

    





    UChar generateStandInFor(UnicodeFunctor* adopted, UErrorCode& status);

    




    UChar getSegmentStandin(int32_t seg, UErrorCode& status);

    




    void setSegmentObject(int32_t seg, StringMatcher* adopted, UErrorCode& status);

    




    UChar getDotStandIn(UErrorCode& status);

    





    void appendVariableDef(const UnicodeString& name,
                           UnicodeString& buf,
                           UErrorCode& status);

    


    


    friend class RuleHalf;

    
    


    TransliteratorParser(const TransliteratorParser&);
    
    


    TransliteratorParser& operator=(const TransliteratorParser&);
};

U_NAMESPACE_END

#endif 











U_CAPI int32_t
utrans_stripRules(const UChar *source, int32_t sourceLen, UChar *target, UErrorCode *status);

#endif 

#endif
