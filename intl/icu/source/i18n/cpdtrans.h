








#ifndef CPDTRANS_H
#define CPDTRANS_H

#include "unicode/utypes.h"

#if !UCONFIG_NO_TRANSLITERATION

#include "unicode/translit.h"

U_NAMESPACE_BEGIN

class U_COMMON_API UVector;
class TransliteratorRegistry;















class U_I18N_API CompoundTransliterator : public Transliterator {

    Transliterator** trans;

    int32_t count;

    int32_t numAnonymousRBTs;

public:

    













    CompoundTransliterator(Transliterator* const transliterators[],
                           int32_t transliteratorCount,
                           UnicodeFilter* adoptedFilter = 0);

    






    CompoundTransliterator(const UnicodeString& id,
                           UTransDirection dir,
                           UnicodeFilter* adoptedFilter,
                           UParseError& parseError,
                           UErrorCode& status);

    



    CompoundTransliterator(const UnicodeString& id,
                           UParseError& parseError,
                           UErrorCode& status);
    


    virtual ~CompoundTransliterator();

    


    CompoundTransliterator(const CompoundTransliterator&);

    


    virtual Transliterator* clone(void) const;

    



    virtual int32_t getCount(void) const;

    




    virtual const Transliterator& getTransliterator(int32_t idx) const;

    


    void setTransliterators(Transliterator* const transliterators[],
                            int32_t count);

    


    void adoptTransliterators(Transliterator* adoptedTransliterators[],
                              int32_t count);

    










    virtual UnicodeString& toRules(UnicodeString& result,
                                   UBool escapeUnprintable) const;

 protected:
    


    virtual void handleGetSourceSet(UnicodeSet& result) const;

 public:
    


    virtual UnicodeSet& getTargetSet(UnicodeSet& result) const;

protected:
    


    virtual void handleTransliterate(Replaceable& text, UTransPosition& idx,
                                     UBool incremental) const;

public:

    


    virtual UClassID getDynamicClassID() const;

    


    static UClassID U_EXPORT2 getStaticClassID();

    
    static const UChar PASS_STRING[];

private:

    friend class Transliterator;
    friend class TransliteratorAlias; 

    


    CompoundTransliterator& operator=(const CompoundTransliterator&);

    


    CompoundTransliterator(const UnicodeString& ID,
                           UVector& list,
                           UnicodeFilter* adoptedFilter,
                           int32_t numAnonymousRBTs,
                           UParseError& parseError,
                           UErrorCode& status);
    
    CompoundTransliterator(UVector& list,
                           UParseError& parseError,
                           UErrorCode& status);

    CompoundTransliterator(UVector& list,
                           int32_t anonymousRBTs,
                           UParseError& parseError,
                           UErrorCode& status);

    void init(const UnicodeString& id,
              UTransDirection direction,
              UBool fixReverseID,
              UErrorCode& status);

    void init(UVector& list,
              UTransDirection direction,
              UBool fixReverseID,
              UErrorCode& status);

    




    UnicodeString joinIDs(Transliterator* const transliterators[],
                          int32_t transCount);

    void freeTransliterators(void);

    void computeMaximumContextLength(void);
};

U_NAMESPACE_END

#endif 

#endif
