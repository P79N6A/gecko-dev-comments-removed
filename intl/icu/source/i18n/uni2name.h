








#ifndef UNI2NAME_H
#define UNI2NAME_H

#include "unicode/utypes.h"

#if !UCONFIG_NO_TRANSLITERATION

#include "unicode/translit.h"

U_NAMESPACE_BEGIN






class UnicodeNameTransliterator : public Transliterator {

 public:

    



    UnicodeNameTransliterator(UnicodeFilter* adoptedFilter = 0);

    


    virtual ~UnicodeNameTransliterator();

    


    UnicodeNameTransliterator(const UnicodeNameTransliterator&);

    


    virtual Transliterator* clone(void) const;

    


    virtual UClassID getDynamicClassID() const;

    


    U_I18N_API static UClassID U_EXPORT2 getStaticClassID();

 protected:

    








    virtual void handleTransliterate(Replaceable& text, UTransPosition& offset,
                                     UBool isIncremental) const;

private:
    


    UnicodeNameTransliterator& operator=(const UnicodeNameTransliterator&);

};

U_NAMESPACE_END

#endif 

#endif
