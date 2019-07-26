








#ifndef RBT_H
#define RBT_H

#include "unicode/utypes.h"

#if !UCONFIG_NO_TRANSLITERATION

#include "unicode/translit.h"
#include "unicode/utypes.h"
#include "unicode/parseerr.h"
#include "unicode/udata.h"

#define U_ICUDATA_TRANSLIT U_ICUDATA_NAME U_TREE_SEPARATOR_STRING "translit"

U_NAMESPACE_BEGIN

class TransliterationRuleData;






































































































































































































































































class RuleBasedTransliterator : public Transliterator {
private:
    




    TransliterationRuleData* fData;

    


    UBool isDataOwned;

public:

    






    RuleBasedTransliterator(const UnicodeString& id,
                            const UnicodeString& rules,
                            UTransDirection direction,
                            UnicodeFilter* adoptedFilter,
                            UParseError& parseError,
                            UErrorCode& status);

    






    





    



    




    



    



    



    



private:

     friend class TransliteratorRegistry; 
    





    RuleBasedTransliterator(const UnicodeString& id,
                            const TransliterationRuleData* theData,
                            UnicodeFilter* adoptedFilter = 0);


    friend class Transliterator; 

    





    RuleBasedTransliterator(const UnicodeString& id,
                            TransliterationRuleData* data,
                            UBool isDataAdopted);

public:

    



    RuleBasedTransliterator(const RuleBasedTransliterator&);

    virtual ~RuleBasedTransliterator();

    



    virtual Transliterator* clone(void) const;

protected:
    



    virtual void handleTransliterate(Replaceable& text, UTransPosition& offsets,
                                     UBool isIncremental) const;

public:
    











    virtual UnicodeString& toRules(UnicodeString& result,
                                   UBool escapeUnprintable) const;

protected:
    


    virtual void handleGetSourceSet(UnicodeSet& result) const;

public:
    


    virtual UnicodeSet& getTargetSet(UnicodeSet& result) const;

    










    U_I18N_API static UClassID U_EXPORT2 getStaticClassID(void);

    









    virtual UClassID getDynamicClassID(void) const;

private:

    void _construct(const UnicodeString& rules,
                    UTransDirection direction,
                    UParseError& parseError,
                    UErrorCode& status);
};


U_NAMESPACE_END

#endif 

#endif
