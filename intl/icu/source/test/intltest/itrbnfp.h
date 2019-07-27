






#ifndef ITRBNFP_H
#define ITRBNFP_H

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "intltest.h"
#include "unicode/rbnf.h"


class IntlTestRBNFParse : public IntlTest {
 public:

  
  virtual void runIndexedTest(int32_t index, UBool exec, const char* &name, char* par);

#if U_HAVE_RBNF
  


  virtual void TestParse();

  void testfmt(RuleBasedNumberFormat* formatter, double val, UErrorCode& status);
  void testfmt(RuleBasedNumberFormat* formatter, int val, UErrorCode& status);

 protected:


#else

  virtual void TestRBNFParseDisabled();


#endif
};

#endif 


#endif
