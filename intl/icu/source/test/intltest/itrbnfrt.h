






#ifndef ITRBNFRT_H
#define ITRBNFRT_H

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "intltest.h"
#include "unicode/rbnf.h"

class RbnfRoundTripTest : public IntlTest {

  
  virtual void runIndexedTest(int32_t index, UBool exec, const char* &name, char* par);

#if U_HAVE_RBNF
  


  virtual void TestEnglishSpelloutRT();

  


  virtual void TestDurationsRT();

  


  virtual void TestSpanishSpelloutRT();

  


  virtual void TestFrenchSpelloutRT();

  


  virtual void TestSwissFrenchSpelloutRT();

  


  virtual void TestItalianSpelloutRT();

  


  virtual void TestGermanSpelloutRT();

  


  virtual void TestSwedishSpelloutRT();

  


  virtual void TestDutchSpelloutRT();

  


  virtual void TestJapaneseSpelloutRT();

  


  virtual void TestRussianSpelloutRT();

  


  virtual void TestPortugueseSpelloutRT();

 protected:
  void doTest(const RuleBasedNumberFormat* formatter,  double lowLimit, double highLimit);

  
#else

  void TestRBNFDisabled();

  
#endif
};

#endif 


#endif
