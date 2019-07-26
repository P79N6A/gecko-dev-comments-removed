








#ifndef TOLOWTRN_H
#define TOLOWTRN_H

#include "unicode/utypes.h"

#if !UCONFIG_NO_TRANSLITERATION

#include "unicode/translit.h"
#include "casetrn.h"

U_NAMESPACE_BEGIN






class LowercaseTransliterator : public CaseMapTransliterator {

 public:

    



    LowercaseTransliterator();

    


    virtual ~LowercaseTransliterator();

    


    LowercaseTransliterator(const LowercaseTransliterator&);

    



    virtual Transliterator* clone(void) const;

    


    virtual UClassID getDynamicClassID() const;

    


    U_I18N_API static UClassID U_EXPORT2 getStaticClassID();
private:

    


    LowercaseTransliterator& operator=(const LowercaseTransliterator&);
};

U_NAMESPACE_END

#endif 

#endif
