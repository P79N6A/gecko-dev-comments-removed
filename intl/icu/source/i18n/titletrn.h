








#ifndef TITLETRN_H
#define TITLETRN_H

#include "unicode/utypes.h"

#if !UCONFIG_NO_TRANSLITERATION

#include "unicode/translit.h"
#include "ucase.h"
#include "casetrn.h"

U_NAMESPACE_BEGIN








class TitlecaseTransliterator : public CaseMapTransliterator {
 public:

    



    TitlecaseTransliterator();

    


    virtual ~TitlecaseTransliterator();

    


    TitlecaseTransliterator(const TitlecaseTransliterator&);

    



    virtual Transliterator* clone(void) const;

    


    virtual UClassID getDynamicClassID() const;

    


    U_I18N_API static UClassID U_EXPORT2 getStaticClassID();

protected:

    








    virtual void handleTransliterate(Replaceable& text, UTransPosition& offset,
                             UBool isIncremental) const;

private:
    


    TitlecaseTransliterator& operator=(const TitlecaseTransliterator&);
};

U_NAMESPACE_END

#endif 

#endif
