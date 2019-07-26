












#ifndef DECFMTST_H
#define DECFMTST_H

#include "unicode/utypes.h"

 #if !UCONFIG_NO_FORMATTING

U_NAMESPACE_BEGIN

class  UnicodeSet;


class DecimalFormatStaticSets : public UMemory
{
public:
    static DecimalFormatStaticSets *gStaticSets;  
                                                  

    DecimalFormatStaticSets(UErrorCode *status);
    ~DecimalFormatStaticSets();

    static void    initSets(UErrorCode *status);
    static UBool   cleanup();

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

};


U_NAMESPACE_END

#endif   
#endif   
