








#ifndef TOUPPTRN_H
#define TOUPPTRN_H

#include "unicode/utypes.h"

#if !UCONFIG_NO_TRANSLITERATION

#include "unicode/translit.h"
#include "casetrn.h"

U_NAMESPACE_BEGIN






class UppercaseTransliterator : public CaseMapTransliterator {

 public:

    



    UppercaseTransliterator();

    


    virtual ~UppercaseTransliterator();

    


    UppercaseTransliterator(const UppercaseTransliterator&);

    



    virtual Transliterator* clone(void) const;

    


    virtual UClassID getDynamicClassID() const;

    


    U_I18N_API static UClassID U_EXPORT2 getStaticClassID();

private:
    


    UppercaseTransliterator& operator=(const UppercaseTransliterator&);
};

U_NAMESPACE_END

#endif 

#endif
