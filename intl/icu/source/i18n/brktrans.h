








#ifndef BRKTRANS_H
#define BRKTRANS_H

#include "unicode/utypes.h"

#if !UCONFIG_NO_TRANSLITERATION && !UCONFIG_NO_BREAK_ITERATION

#include "unicode/translit.h"


U_NAMESPACE_BEGIN

class UVector32;







class BreakTransliterator : public Transliterator {
public:

    BreakTransliterator(const UnicodeString &ID, 
                        UnicodeFilter *adoptedFilter,
                        BreakIterator *bi, 
                        const UnicodeString &insertion);
    



    BreakTransliterator(UnicodeFilter* adoptedFilter = 0);

    


    virtual ~BreakTransliterator();

    


    BreakTransliterator(const BreakTransliterator&);

    



    virtual Transliterator* clone(void) const;

    virtual const UnicodeString &getInsertion() const;

    virtual void setInsertion(const UnicodeString &insertion);

    




    virtual BreakIterator *getBreakIterator();


    


    virtual UClassID getDynamicClassID() const;

    


    U_I18N_API static UClassID U_EXPORT2 getStaticClassID();

 protected:

    








    virtual void handleTransliterate(Replaceable& text, UTransPosition& offset,
                                     UBool isIncremental) const;

 private:
     BreakIterator     *bi;
     UnicodeString      fInsertion;
     UVector32         *boundaries;
     UnicodeString      sText;  

     static UnicodeString replaceableAsString(Replaceable &r);

    


    BreakTransliterator& operator=(const BreakTransliterator&);
};

U_NAMESPACE_END

#endif 

#endif
