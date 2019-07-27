















#ifndef _CNUMDEPTST
#define _CNUMDEPTST

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "cintltst.h"


static void TestPatterns(void);


static void TestQuotes(void);


static void TestExponential(void);


static void TestCurrencySign(void); 


static void TestRounding487(void);


static void TestRounding5350(void);


static void TestCurrency(void);


static void TestDoubleAttribute(void);

static void TestSecondaryGrouping(void);


static void roundingTest(UNumberFormat*, double,  int32_t, const char*);
static void roundingTest2(UNumberFormat*, double, int32_t, const char*);

static void TestCurrencyKeywords(void);

static void TestGetKeywordValuesForLocale(void);

#endif 

#endif
