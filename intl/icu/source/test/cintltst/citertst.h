


















#ifndef _CITERCOLLTST
#define _CITERCOLLTST

#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

#include "unicode/ucol.h"
#include "unicode/ucoleitr.h"
#include "cintltst.h"

#define MAX_TOKEN_LEN 16





static void TestUnicodeChar(void);




static void TestNormalizedUnicodeChar(void);



static void TestNormalization(void);
 





static void TestPrevious(void);




static void TestOffset(void);



static void TestSetText(void);



static void TestMaxExpansion(void);




static void TestBug672(void);





static void TestBug672Normalize(void);



static void TestSmallBuffer(void);



static void TestDiscontiguos(void);




static void TestSearchCollatorElements(void);






static void assertEqual(UCollationElements *i1, UCollationElements *i2);


#endif 

#endif
