






#ifndef RBT_RULE_H
#define RBT_RULE_H

#include "unicode/utypes.h"

#if !UCONFIG_NO_TRANSLITERATION

#include "unicode/uobject.h"
#include "unicode/unistr.h"
#include "unicode/utrans.h"
#include "unicode/unimatch.h"

U_NAMESPACE_BEGIN

class Replaceable;
class TransliterationRuleData;
class StringMatcher;
class UnicodeFunctor;





























class TransliterationRule : public UMemory {

private:

    
    
    
    
    

    



    StringMatcher *anteContext;

    


    StringMatcher *key;

    



    StringMatcher *postContext;

    



    UnicodeFunctor* output;

    






    UnicodeString pattern;

    








    UnicodeFunctor** segments;

    


    int32_t segmentsCount;

    




    int32_t anteContextLength;

    




    int32_t keyLength;

    


    int8_t flags;

    


    enum {
        ANCHOR_START = 1,
        ANCHOR_END   = 2
    };

    



    const TransliterationRuleData* data;

public:

    































    TransliterationRule(const UnicodeString& input,
                        int32_t anteContextPos, int32_t postContextPos,
                        const UnicodeString& outputStr,
                        int32_t cursorPosition, int32_t cursorOffset,
                        UnicodeFunctor** segs,
                        int32_t segsCount,
                        UBool anchorStart, UBool anchorEnd,
                        const TransliterationRuleData* data,
                        UErrorCode& status);

    



    TransliterationRule(TransliterationRule& other);

    


    virtual ~TransliterationRule();

    




    void setData(const TransliterationRuleData* data);

    









    virtual int32_t getContextLength(void) const;

    






    int16_t getIndexValue() const;

    











    UBool matchesIndexValue(uint8_t v) const;

    







    virtual UBool masks(const TransliterationRule& r2) const;

    




















    UMatchDegree matchAndReplace(Replaceable& text,
                                 UTransPosition& pos,
                                 UBool incremental) const;

    



    virtual UnicodeString& toRule(UnicodeString& pat,
                                  UBool escapeUnprintable) const;

    



    void addSourceSetTo(UnicodeSet& toUnionTo) const;

    



    void addTargetSetTo(UnicodeSet& toUnionTo) const;

 private:

    friend class StringMatcher;

    TransliterationRule &operator=(const TransliterationRule &other); 
};

U_NAMESPACE_END

#endif 

#endif
