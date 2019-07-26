








#ifndef NAME2UNI_H
#define NAME2UNI_H

#include "unicode/utypes.h"

#if !UCONFIG_NO_TRANSLITERATION

#include "unicode/translit.h"
#include "unicode/uniset.h"

U_NAMESPACE_BEGIN






class NameUnicodeTransliterator : public Transliterator {
public:

    



    NameUnicodeTransliterator(UnicodeFilter* adoptedFilter = 0);

    


    virtual ~NameUnicodeTransliterator();

    


    NameUnicodeTransliterator(const NameUnicodeTransliterator&);

    



    virtual Transliterator* clone(void) const;

    


    virtual UClassID getDynamicClassID() const;

    


    U_I18N_API static UClassID U_EXPORT2 getStaticClassID();

 protected:

    








    virtual void handleTransliterate(Replaceable& text, UTransPosition& offset,
                                     UBool isIncremental) const;

    


    UnicodeSet legal;
private:
    


    NameUnicodeTransliterator& operator=(const NameUnicodeTransliterator&);
};

U_NAMESPACE_END

#endif 

#endif
