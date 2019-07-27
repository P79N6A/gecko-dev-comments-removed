










#ifndef INTLTESTSPOOF_H
#define INTLTESTSPOOF_H

#include "unicode/utypes.h"
#if !UCONFIG_NO_REGULAR_EXPRESSIONS && !UCONFIG_NO_NORMALIZATION && !UCONFIG_NO_FILE_IO
#include "unicode/uspoof.h"
#include "intltest.h"


class IntlTestSpoof: public IntlTest {
public:
    void runIndexedTest( int32_t index, UBool exec, const char* &name, char* par = NULL );
    
    
    
    void  testSpoofAPI();

    void  testSkeleton();

    void testAreConfusable();
    
    void testInvisible();

    void testConfData();

    void testBug8654();

    void testIdentifierInfo();

    void testScriptSet();

    void testRestrictionLevel();

    void testMixedNumbers();

    
    void  checkSkeleton(const USpoofChecker *sc, uint32_t flags, 
                        const char *input, const char *expected, int32_t lineNum);
};

#endif  
#endif
