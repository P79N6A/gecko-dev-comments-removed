





#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

#ifndef _COLL
#include "unicode/coll.h"
#endif

#ifndef _TBLCOLL
#include "unicode/tblcoll.h"
#endif

#ifndef _UNISTR
#include "unicode/unistr.h"
#endif

#ifndef _SORTKEY
#include "unicode/sortkey.h"
#endif

#ifndef _CURRCOLL
#include "currcoll.h"
#endif

#include "sfwdchit.h"

#define ARRAY_LENGTH(array) (sizeof array / sizeof array[0])

CollationCurrencyTest::CollationCurrencyTest()
{
}

CollationCurrencyTest::~CollationCurrencyTest()
{
}

void CollationCurrencyTest::currencyTest()
{
    
    static const UChar currency[][2] =
    {
      { 0x00A4, 0x0000}, 
      { 0x00A2, 0x0000}, 
      { 0xFFE0, 0x0000}, 
      { 0x0024, 0x0000}, 
      { 0xFF04, 0x0000}, 
      { 0xFE69, 0x0000}, 
      { 0x00A3, 0x0000}, 
      { 0xFFE1, 0x0000}, 
      { 0x00A5, 0x0000}, 
      { 0xFFE5, 0x0000}, 
      { 0x09F2, 0x0000}, 
      { 0x09F3, 0x0000}, 
      { 0x0E3F, 0x0000}, 
      { 0x17DB, 0x0000}, 
      { 0x20A0, 0x0000}, 
      { 0x20A1, 0x0000}, 
      { 0x20A2, 0x0000}, 
      { 0x20A3, 0x0000}, 
      { 0x20A4, 0x0000}, 
      { 0x20A5, 0x0000}, 
      { 0x20A6, 0x0000}, 
      { 0x20A7, 0x0000}, 
      { 0x20A9, 0x0000}, 
      { 0xFFE6, 0x0000}, 
      { 0x20AA, 0x0000}, 
      { 0x20AB, 0x0000}, 
      { 0x20AC, 0x0000}, 
      { 0x20AD, 0x0000}, 
      { 0x20AE, 0x0000}, 
      { 0x20AF, 0x0000}, 
    };
    

    uint32_t i, j;
    UErrorCode status = U_ZERO_ERROR;
    Collator::EComparisonResult expectedResult = Collator::EQUAL;
    RuleBasedCollator *c = (RuleBasedCollator *)Collator::createInstance("en_US", status);

    if (U_FAILURE(status))
    {
        errcheckln (status, "Collator::createInstance() failed! - %s", u_errorName(status));
        return;
    }

    
    
    for (i = 0; i < ARRAY_LENGTH(currency); i += 1)
    {
        for (j = 0; j < ARRAY_LENGTH(currency); j += 1)
        {
            UnicodeString source(currency[i], 1);
            UnicodeString target(currency[j], 1);

            if (i < j)
            {
                expectedResult = Collator::LESS;
            }
            else if ( i == j)
            {
                expectedResult = Collator::EQUAL;
            }
            else
            {
                expectedResult = Collator::GREATER;
            }

            Collator::EComparisonResult compareResult = c->compare(source, target);

            CollationKey sourceKey, targetKey;
            UErrorCode status = U_ZERO_ERROR;

            c->getCollationKey(source, sourceKey, status);

            if (U_FAILURE(status))
            {
                errln("Couldn't get collationKey for source");
                continue;
            }

            c->getCollationKey(target, targetKey, status);

            if (U_FAILURE(status))
            {
                errln("Couldn't get collationKey for target");
                continue;
            }

            Collator::EComparisonResult keyResult = sourceKey.compareTo(targetKey);

            reportCResult( source, target, sourceKey, targetKey, compareResult, keyResult, compareResult, expectedResult );

        }
    }
    delete c;
}

void CollationCurrencyTest::runIndexedTest(int32_t index, UBool exec, const char* &name, char* )
{
    if (exec)
    {
        logln("Collation Currency Tests: ");
    }

    switch (index)
    {
        case  0: name = "currencyTest"; if (exec) currencyTest(); break;
        default: name = ""; break;
    }
}

#endif 
