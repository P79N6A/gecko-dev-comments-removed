








#ifndef REMTRANS_H
#define REMTRANS_H

#include "unicode/utypes.h"

#if !UCONFIG_NO_TRANSLITERATION

#include "unicode/translit.h"

U_NAMESPACE_BEGIN





class RemoveTransliterator : public Transliterator {

public:

    


    RemoveTransliterator();

    


    virtual ~RemoveTransliterator();

    


    static void registerIDs();

    



    virtual Transliterator* clone(void) const;

    








    virtual void handleTransliterate(Replaceable& text, UTransPosition& offset,
                                     UBool isIncremental) const;

    


    virtual UClassID getDynamicClassID() const;

    


    U_I18N_API static UClassID U_EXPORT2 getStaticClassID();

};

U_NAMESPACE_END

#endif 

#endif
