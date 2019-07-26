








#ifndef QUANT_H
#define QUANT_H

#include "unicode/utypes.h"

#if !UCONFIG_NO_TRANSLITERATION

#include "unicode/unifunct.h"
#include "unicode/unimatch.h"

U_NAMESPACE_BEGIN

class Quantifier : public UnicodeFunctor, public UnicodeMatcher {

 public:

    enum { MAX = 0x7FFFFFFF };

    Quantifier(UnicodeFunctor *adoptedMatcher,
               uint32_t minCount, uint32_t maxCount);

    Quantifier(const Quantifier& o);

    virtual ~Quantifier();

    




    virtual UnicodeMatcher* toMatcher() const;

    



    virtual UnicodeFunctor* clone() const;

    





















    virtual UMatchDegree matches(const Replaceable& text,
                                 int32_t& offset,
                                 int32_t limit,
                                 UBool incremental);

    





    virtual UnicodeString& toPattern(UnicodeString& result,
                                     UBool escapeUnprintable = FALSE) const;

    




    virtual UBool matchesIndexValue(uint8_t v) const;

    


    virtual void addMatchSetTo(UnicodeSet& toUnionTo) const;

    


    virtual void setData(const TransliterationRuleData*);

    


    virtual UClassID getDynamicClassID() const;

    


    static UClassID U_EXPORT2 getStaticClassID();

 private:

    UnicodeFunctor* matcher; 

    uint32_t minCount;

    uint32_t maxCount;
};

U_NAMESPACE_END

#endif

#endif
