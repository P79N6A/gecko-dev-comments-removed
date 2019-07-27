





#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

#include "unicode/coll.h"
#include "unicode/tblcoll.h"
#include "unicode/unistr.h"
#include "unicode/sortkey.h"
#include "jacoll.h"

#include "sfwdchit.h"

CollationKanaTest::CollationKanaTest()
: myCollation(0)
{
    UErrorCode status = U_ZERO_ERROR;
    myCollation = Collator::createInstance(Locale::getJapan(), status);
    if(!myCollation || U_FAILURE(status)) {
        errcheckln(status, __FILE__ "failed to create! err " + UnicodeString(u_errorName(status)));
        
        delete myCollation;
        myCollation = NULL;
        return;
    }
}

CollationKanaTest::~CollationKanaTest()
{
    delete myCollation;
}

const UChar CollationKanaTest::testSourceCases[][CollationKanaTest::MAX_TOKEN_LEN] = {
    {0xff9E, 0x0000},
    {0x3042, 0x0000},
    {0x30A2, 0x0000},
    {0x3042, 0x3042, 0x0000},
    {0x30A2, 0x30FC, 0x0000},
    {0x30A2, 0x30FC, 0x30C8, 0x0000}                               
};

const UChar CollationKanaTest::testTargetCases[][CollationKanaTest::MAX_TOKEN_LEN] = {
    {0xFF9F, 0x0000},
    {0x30A2, 0x0000},
    {0x3042, 0x3042, 0x0000},
    {0x30A2, 0x30FC, 0x0000},
    {0x30A2, 0x30FC, 0x30C8, 0x0000},
    {0x3042, 0x3042, 0x3068, 0x0000}                              
};

const Collator::EComparisonResult CollationKanaTest::results[] = {
    Collator::LESS,
    Collator::EQUAL,   
    Collator::LESS,
    Collator::GREATER, 
    Collator::LESS,
    Collator::LESS,    
};

const UChar CollationKanaTest::testBaseCases[][CollationKanaTest::MAX_TOKEN_LEN] = {
  {0x30AB, 0x0000},
  {0x30AB, 0x30AD, 0x0000},
  {0x30AD, 0x0000},
  {0x30AD, 0x30AD, 0x0000}
};

const UChar CollationKanaTest::testPlainDakutenHandakutenCases[][CollationKanaTest::MAX_TOKEN_LEN] = {
  {0x30CF, 0x30AB, 0x0000},
  {0x30D0, 0x30AB, 0x0000},
  {0x30CF, 0x30AD, 0x0000},
  {0x30D0, 0x30AD, 0x0000}
};

const UChar CollationKanaTest::testSmallLargeCases[][CollationKanaTest::MAX_TOKEN_LEN] = {
  {0x30C3, 0x30CF, 0x0000},
  {0x30C4, 0x30CF, 0x0000},
  {0x30C3, 0x30D0, 0x0000},
  {0x30C4, 0x30D0, 0x0000}
};

const UChar CollationKanaTest::testKatakanaHiraganaCases[][CollationKanaTest::MAX_TOKEN_LEN] = {
  {0x3042, 0x30C3, 0x0000},
  {0x30A2, 0x30C3, 0x0000},
  {0x3042, 0x30C4, 0x0000},
  {0x30A2, 0x30C4, 0x0000}
};

const UChar CollationKanaTest::testChooonKigooCases[][CollationKanaTest::MAX_TOKEN_LEN] = {
   {0x30AB, 0x30FC, 0x3042, 0x0000},
   {0x30AB, 0x30FC, 0x30A2, 0x0000},
   {0x30AB, 0x30A4, 0x3042, 0x0000},
   {0x30AB, 0x30A4, 0x30A2, 0x0000},
   {0x30AD, 0x30FC, 0x3042, 0x0000}, 
   {0x30AD, 0x30FC, 0x30A2, 0x0000}, 
   {0x30AD, 0x30A4, 0x3042, 0x0000},
   {0x30AD, 0x30A4, 0x30A2, 0x0000},
};

void CollationKanaTest::TestTertiary()
{
    int32_t i = 0;
    UErrorCode status = U_ZERO_ERROR;
    myCollation->setStrength(Collator::TERTIARY);
    
    
    myCollation->setAttribute(UCOL_NORMALIZATION_MODE, UCOL_ON, status);
    myCollation->setAttribute(UCOL_CASE_LEVEL, UCOL_ON, status);
    for (i = 0; i < 6; i++) {
        doTest(myCollation, testSourceCases[i], testTargetCases[i], results[i]);
    }
}


void CollationKanaTest::TestBase()
{
    int32_t i;
    myCollation->setStrength(Collator::PRIMARY);
    for (i = 0; i < 3 ; i++)
        doTest(myCollation, testBaseCases[i], testBaseCases[i + 1], Collator::LESS);
}


void CollationKanaTest::TestPlainDakutenHandakuten(void)
{
    int32_t i;
    myCollation->setStrength(Collator::SECONDARY);
    for (i = 0; i < 3 ; i++)
        doTest(myCollation, testPlainDakutenHandakutenCases[i], testPlainDakutenHandakutenCases[i + 1], 
        Collator::LESS);
}




void CollationKanaTest::TestSmallLarge(void)
{
  int32_t i;
  UErrorCode status = U_ZERO_ERROR;
  myCollation->setStrength(Collator::TERTIARY);
  myCollation->setAttribute(UCOL_CASE_LEVEL, UCOL_ON, status);
  for (i = 0; i < 3 ; i++)
    doTest(myCollation, testSmallLargeCases[i], testSmallLargeCases[i + 1], Collator::LESS);
}




void CollationKanaTest::TestKatakanaHiragana(void)
{
  int32_t i;
  UErrorCode status = U_ZERO_ERROR;
  myCollation->setStrength(Collator::QUATERNARY);
  myCollation->setAttribute(UCOL_CASE_LEVEL, UCOL_ON, status);
  for (i = 0; i < 3 ; i++) {
    doTest(myCollation, testKatakanaHiraganaCases[i], testKatakanaHiraganaCases[i + 1], 
      Collator::LESS);
  }
}




void CollationKanaTest::TestChooonKigoo(void)
{
  int32_t i;
  UErrorCode status = U_ZERO_ERROR;
  myCollation->setStrength(Collator::QUATERNARY);
  myCollation->setAttribute(UCOL_CASE_LEVEL, UCOL_ON, status);
  for (i = 0; i < 7 ; i++) {
    doTest(myCollation, testChooonKigooCases[i], testChooonKigooCases[i + 1], Collator::LESS);
  }
}


void CollationKanaTest::runIndexedTest( int32_t index, UBool exec, const char* &name, char*  )
{
    if (exec) logln("TestSuite CollationKanaTest: ");
    if(myCollation) {
      switch (index) {
          case 0: name = "TestTertiary";  if (exec)   TestTertiary(); break;
          case 1: name = "TestBase";  if (exec)   TestBase(); break;
          case 2: name = "TestSmallLarge";  if (exec)   TestSmallLarge(); break;
          case 3: name = "TestTestPlainDakutenHandakuten";  if (exec)   TestPlainDakutenHandakuten(); break;
          case 4: name = "TestKatakanaHiragana";  if (exec)   TestKatakanaHiragana(); break;
          case 5: name = "TestChooonKigoo";  if (exec)   TestChooonKigoo(); break;
          default: name = ""; break;
      }
    } else {
      dataerrln("Collator couldn't be instantiated!");
      name = "";
    }
}

#endif 
