







#ifndef STRMATCH_H
#define STRMATCH_H

#include "unicode/utypes.h"

#if !UCONFIG_NO_TRANSLITERATION

#include "unicode/unistr.h"
#include "unicode/unifunct.h"
#include "unicode/unimatch.h"
#include "unicode/unirepl.h"

U_NAMESPACE_BEGIN

class TransliterationRuleData;


















class StringMatcher : public UnicodeFunctor, public UnicodeMatcher, public UnicodeReplacer {

 public:

    











    StringMatcher(const UnicodeString& string,
                  int32_t start,
                  int32_t limit,
                  int32_t segmentNum,
                  const TransliterationRuleData& data);

    



    StringMatcher(const StringMatcher& o);
        
    


    virtual ~StringMatcher();

    



    virtual UnicodeFunctor* clone() const;

    




    virtual UnicodeMatcher* toMatcher() const;

    




    virtual UnicodeReplacer* toReplacer() const;

    





















    virtual UMatchDegree matches(const Replaceable& text,
                                 int32_t& offset,
                                 int32_t limit,
                                 UBool incremental);

    





    virtual UnicodeString& toPattern(UnicodeString& result,
                                     UBool escapeUnprintable = FALSE) const;

    









    virtual UBool matchesIndexValue(uint8_t v) const;

    


    virtual void addMatchSetTo(UnicodeSet& toUnionTo) const;

    


    virtual void setData(const TransliterationRuleData*);

    
















    virtual int32_t replace(Replaceable& text,
                            int32_t start,
                            int32_t limit,
                            int32_t& cursor);

    












    virtual UnicodeString& toReplacerPattern(UnicodeString& result,
                                             UBool escapeUnprintable) const;

    



    void resetMatch();

    


    virtual UClassID getDynamicClassID() const;

    


    static UClassID U_EXPORT2 getStaticClassID();

    




    virtual void addReplacementSetTo(UnicodeSet& toUnionTo) const;

 private:

    


    UnicodeString pattern;

    



    const TransliterationRuleData* data;

    


    int32_t segmentNumber;

    



    int32_t matchStart;

    



    int32_t matchLimit;

};

U_NAMESPACE_END

#endif 

#endif
