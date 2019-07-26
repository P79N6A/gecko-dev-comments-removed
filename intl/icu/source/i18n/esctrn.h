








#ifndef ESCTRN_H
#define ESCTRN_H

#include "unicode/utypes.h"

#if !UCONFIG_NO_TRANSLITERATION

#include "unicode/translit.h"

U_NAMESPACE_BEGIN
























class EscapeTransliterator : public Transliterator {

 private:

    


    UnicodeString prefix;

    


    UnicodeString suffix;

    



    int32_t radix;

    



    int32_t minDigits;

    



    UBool grokSupplementals;

    






    EscapeTransliterator* supplementalHandler;

 public:

    



    static void registerIDs();

    



    EscapeTransliterator(const UnicodeString& ID,
                         const UnicodeString& prefix, const UnicodeString& suffix,
                         int32_t radix, int32_t minDigits,
                         UBool grokSupplementals,
                         EscapeTransliterator* adoptedSupplementalHandler);

    


    EscapeTransliterator(const EscapeTransliterator&);

    


    virtual ~EscapeTransliterator();

    


    virtual Transliterator* clone() const;

    


    virtual UClassID getDynamicClassID() const;

    


    U_I18N_API static UClassID U_EXPORT2 getStaticClassID();

 protected:

    


    virtual void handleTransliterate(Replaceable& text, UTransPosition& offset,
                             UBool isIncremental) const;

};

U_NAMESPACE_END

#endif 

#endif
