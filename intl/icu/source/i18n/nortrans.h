








#ifndef NORTRANS_H
#define NORTRANS_H

#include "unicode/utypes.h"

#if !UCONFIG_NO_TRANSLITERATION

#include "unicode/translit.h"
#include "unicode/normalizer2.h"

U_NAMESPACE_BEGIN





class NormalizationTransliterator : public Transliterator {
    const Normalizer2 &fNorm2;

 public:

    


    virtual ~NormalizationTransliterator();

    


    NormalizationTransliterator(const NormalizationTransliterator&);

    



    virtual Transliterator* clone(void) const;

    


    virtual UClassID getDynamicClassID() const;

    


    U_I18N_API static UClassID U_EXPORT2 getStaticClassID();

 protected:

    








    virtual void handleTransliterate(Replaceable& text, UTransPosition& offset,
                             UBool isIncremental) const;
 public:

    


    static void registerIDs();

 private:

    
    static Transliterator* _create(const UnicodeString& ID,
                                   Token context);

    



    NormalizationTransliterator(const UnicodeString& id, const Normalizer2 &norm2);

private:
    


    NormalizationTransliterator& operator=(const NormalizationTransliterator&);
};

U_NAMESPACE_END

#endif 

#endif
