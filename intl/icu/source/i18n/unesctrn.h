








#ifndef UNESCTRN_H
#define UNESCTRN_H

#include "unicode/utypes.h"

#if !UCONFIG_NO_TRANSLITERATION

#include "unicode/translit.h"

U_NAMESPACE_BEGIN











class UnescapeTransliterator : public Transliterator {

 private:

    











    UChar* spec; 

 public:

    



    static void registerIDs();

    




    UnescapeTransliterator(const UnicodeString& ID,
                           const UChar *spec);

    


    UnescapeTransliterator(const UnescapeTransliterator&);

    


    virtual ~UnescapeTransliterator();

    


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
