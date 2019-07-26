











 
#ifndef B_M_SEARCH_H
#define B_M_SEARCH_H

#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION && !UCONFIG_NO_BREAK_ITERATION

#include "unicode/uobject.h"
#include "unicode/ucol.h"

#include "unicode/colldata.h"

U_NAMESPACE_BEGIN

class BadCharacterTable;
class GoodSuffixTable;
class Target;

#ifndef U_HIDE_INTERNAL_API











































































class U_I18N_API BoyerMooreSearch : public UObject
{
public:
    














    BoyerMooreSearch(CollData *theData, const UnicodeString &patternString, const UnicodeString *targetString, UErrorCode &status);

    




    ~BoyerMooreSearch();

    






    UBool empty();

    










    UBool search(int32_t offset, int32_t &start, int32_t &end);

    







    void setTargetString(const UnicodeString *targetString, UErrorCode &status);

    
    






    CollData *getData();

    






    CEList   *getPatternCEs();

    






    BadCharacterTable *getBadCharacterTable();

    






    GoodSuffixTable   *getGoodSuffixTable();

    



    virtual UClassID getDynamicClassID() const;
    



    static UClassID getStaticClassID();
    
private:
    CollData *data;
    CEList *patCEs;
    BadCharacterTable *badCharacterTable;
    GoodSuffixTable   *goodSuffixTable;
    UnicodeString pattern;
    Target *target;
};
#endif  

U_NAMESPACE_END

#endif 
#endif 
