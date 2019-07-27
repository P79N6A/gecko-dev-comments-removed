













#ifndef _CCOLLTST
#define _CCOLLTST

#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

#include "cintltst.h"
#include "unicode/ucol.h"



void reportCResult( const UChar source[], const UChar target[], 
                         uint8_t *sourceKey, uint8_t *targetKey,
                         UCollationResult compareResult,
                         UCollationResult keyResult,
                         UCollationResult incResult,
                         UCollationResult expectedResult );

UChar* appendCompareResult(UCollationResult result, UChar* target);

void addCollAPITest(TestNode**);
void addCurrencyCollTest(TestNode**);
void addNormTest(TestNode**);
void addDanishCollTest(TestNode**);
void addGermanCollTest(TestNode**);
void addSpanishCollTest(TestNode**);
void addFrenchCollTest(TestNode**);
void addKannaCollTest(TestNode**);
void addTurkishCollTest(TestNode**);
void addEnglishCollTest(TestNode**);
void addFinnishCollTest(TestNode**);

void addRuleBasedCollTest(TestNode**);
void addCollIterTest(TestNode**);
void addAllCollTest(TestNode**);
void addMiscCollTest(TestNode**);
void addSearchTest(TestNode**);

#endif 

#endif
