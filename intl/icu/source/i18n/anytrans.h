








#ifndef _ANYTRANS_H_
#define _ANYTRANS_H_

#include "unicode/utypes.h"

#if !UCONFIG_NO_TRANSLITERATION

#include "unicode/translit.h"
#include "unicode/uscript.h"
#include "uhash.h"

U_NAMESPACE_BEGIN













class AnyTransliterator : public Transliterator {

    


    UHashtable* cache;

    


    UnicodeString target;

    


    UScriptCode targetScript;

public:

    


    virtual ~AnyTransliterator();

    


    AnyTransliterator(const AnyTransliterator&);

    


    virtual Transliterator* clone() const;

    


    virtual void handleTransliterate(Replaceable& text, UTransPosition& index,
                                     UBool incremental) const;

    


    virtual UClassID getDynamicClassID() const;

    


    U_I18N_API static UClassID U_EXPORT2 getStaticClassID();

private:

    












    AnyTransliterator(const UnicodeString& id,
                      const UnicodeString& theTarget,
                      const UnicodeString& theVariant,
                      UScriptCode theTargetScript,
                      UErrorCode& ec);

    







    Transliterator* getTransliterator(UScriptCode source) const;

    



    static void registerIDs();

    friend class Transliterator; 
};

U_NAMESPACE_END

#endif 

#endif
