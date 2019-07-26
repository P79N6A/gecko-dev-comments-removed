









#ifndef STRREPL_H
#define STRREPL_H

#include "unicode/utypes.h"

#if !UCONFIG_NO_TRANSLITERATION

#include "unicode/unifunct.h"
#include "unicode/unirepl.h"
#include "unicode/unistr.h"

U_NAMESPACE_BEGIN

class TransliterationRuleData;










class StringReplacer : public UnicodeFunctor, public UnicodeReplacer {

 private:

    



    UnicodeString output;

    


    int32_t cursorPos;

    


    UBool hasCursor;

    






    UBool isComplex;

    



    const TransliterationRuleData* data;

 public:

    










    StringReplacer(const UnicodeString& theOutput,
                   int32_t theCursorPos,
                   const TransliterationRuleData* theData);

    








    StringReplacer(const UnicodeString& theOutput,
                   const TransliterationRuleData* theData);

    


    StringReplacer(const StringReplacer& other);

    


    virtual ~StringReplacer();

    


    virtual UnicodeFunctor* clone() const;

    



    virtual UnicodeReplacer* toReplacer() const;

    


    virtual int32_t replace(Replaceable& text,
                            int32_t start,
                            int32_t limit,
                            int32_t& cursor);

    


    virtual UnicodeString& toReplacerPattern(UnicodeString& result,
                                             UBool escapeUnprintable) const;

    


    virtual void addReplacementSetTo(UnicodeSet& toUnionTo) const;

    


    virtual void setData(const TransliterationRuleData*);

    


    static UClassID U_EXPORT2 getStaticClassID();

    


    virtual UClassID getDynamicClassID() const;
};

U_NAMESPACE_END

#endif 

#endif


