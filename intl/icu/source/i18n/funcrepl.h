









#ifndef FUNCREPL_H
#define FUNCREPL_H

#include "unicode/utypes.h"

#if !UCONFIG_NO_TRANSLITERATION

#include "unicode/unifunct.h"
#include "unicode/unirepl.h"

U_NAMESPACE_BEGIN

class Transliterator;









class FunctionReplacer : public UnicodeFunctor, public UnicodeReplacer {

 private:

    


    Transliterator* translit;

    



    UnicodeFunctor* replacer;

 public:

    




    FunctionReplacer(Transliterator* adoptedTranslit,
                     UnicodeFunctor* adoptedReplacer);

    


    FunctionReplacer(const FunctionReplacer& other);

    


    virtual ~FunctionReplacer();

    


    virtual UnicodeFunctor* clone() const;

    



    virtual UnicodeReplacer* toReplacer() const;

    


    virtual int32_t replace(Replaceable& text,
                            int32_t start,
                            int32_t limit,
                            int32_t& cursor);

    


    virtual UnicodeString& toReplacerPattern(UnicodeString& rule,
                                             UBool escapeUnprintable) const;

    


    virtual void addReplacementSetTo(UnicodeSet& toUnionTo) const;

    


    virtual void setData(const TransliterationRuleData*);

    


    virtual UClassID getDynamicClassID() const;

    


    static UClassID U_EXPORT2 getStaticClassID();
};

U_NAMESPACE_END

#endif 
#endif


