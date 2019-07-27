











#ifndef _UCACONF_TST
#define _UCACONF_TST

#include "unicode/utypes.h"

#if !UCONFIG_NO_COLLATION

#include "unicode/tblcoll.h"
#include "tscoll.h"

#include <stdio.h>

class UCAConformanceTest: public IntlTestCollator {
public:
  UCAConformanceTest();
  virtual ~UCAConformanceTest();

  void runIndexedTest( int32_t index, UBool exec, const char* &name, char* );

  void TestTableNonIgnorable();
  void TestTableShifted();     
  void TestRulesNonIgnorable();
  void TestRulesShifted();     
private:
  void initRbUCA();
  void setCollNonIgnorable(Collator *coll);
  void setCollShifted(Collator *coll);
  void testConformance(const Collator *coll);
  void openTestFile(const char *type);

  RuleBasedCollator *UCA;  
  Collator *rbUCA;
  FILE *testFile;
  UErrorCode status;
  char testDataPath[1024];
  UBool isAtLeastUCA62;
};

#endif 

#endif
