












#ifndef DECFMTST_H
#define DECFMTST_H

#include "unicode/utypes.h"

 #if !UCONFIG_NO_FORMATTING

U_NAMESPACE_BEGIN

class  UnicodeSet;


class DecimalFormatStaticSets : public UMemory
{
public:
    
    
    DecimalFormatStaticSets(UErrorCode &status);
    ~DecimalFormatStaticSets();

    


    static const DecimalFormatStaticSets *getStaticSets(UErrorCode &status);

    static const UnicodeSet *getSimilarDecimals(UChar32 decimal, UBool strictParse);

    UnicodeSet *fDotEquivalents;
    UnicodeSet *fCommaEquivalents;
    UnicodeSet *fOtherGroupingSeparators;
    UnicodeSet *fDashEquivalents;

    UnicodeSet *fStrictDotEquivalents;
    UnicodeSet *fStrictCommaEquivalents;
    UnicodeSet *fStrictOtherGroupingSeparators;
    UnicodeSet *fStrictDashEquivalents;

    UnicodeSet *fDefaultGroupingSeparators;
    UnicodeSet *fStrictDefaultGroupingSeparators;

    UnicodeSet *fMinusSigns;
    UnicodeSet *fPlusSigns;
private:
    void cleanup();

};


U_NAMESPACE_END

#endif   
#endif   
