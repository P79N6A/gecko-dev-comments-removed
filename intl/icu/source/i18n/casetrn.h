

















#ifndef __CASETRN_H__
#define __CASETRN_H__

#include "unicode/utypes.h"

#if !UCONFIG_NO_TRANSLITERATION

#include "unicode/translit.h"
#include "ucase.h"

U_NAMESPACE_BEGIN





class CaseMapTransliterator : public Transliterator {
public:
    





    CaseMapTransliterator(const UnicodeString &id, UCaseMapFull *map);

    


    virtual ~CaseMapTransliterator();

    


    CaseMapTransliterator(const CaseMapTransliterator&);

    



    virtual Transliterator* clone(void) const = 0;

    


    

    


    U_I18N_API static UClassID U_EXPORT2 getStaticClassID();

protected:
    








    virtual void handleTransliterate(Replaceable& text,
                                     UTransPosition& offsets, 
                                     UBool isIncremental) const;

    const UCaseProps *fCsp;
    UCaseMapFull *fMap;

private:
    


    CaseMapTransliterator& operator=(const CaseMapTransliterator&);

};

U_NAMESPACE_END


U_CFUNC UChar32 U_CALLCONV
utrans_rep_caseContextIterator(void *context, int8_t dir);

#endif 

#endif
