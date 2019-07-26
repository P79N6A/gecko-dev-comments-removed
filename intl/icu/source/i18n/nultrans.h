








#ifndef NULTRANS_H
#define NULTRANS_H

#include "unicode/utypes.h"

#if !UCONFIG_NO_TRANSLITERATION

#include "unicode/translit.h"

U_NAMESPACE_BEGIN






class NullTransliterator : public Transliterator {

public:

    



    NullTransliterator();

    



    virtual ~NullTransliterator();

    



    virtual Transliterator* clone(void) const;

    



    virtual void handleTransliterate(Replaceable& text, UTransPosition& offset,
                                     UBool isIncremental) const;

    


    virtual UClassID getDynamicClassID() const;

    


    U_I18N_API static UClassID U_EXPORT2 getStaticClassID();

};

U_NAMESPACE_END

#endif 

#endif
