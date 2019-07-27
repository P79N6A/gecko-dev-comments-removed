






#ifndef ITRBNF_H
#define ITRBNF_H

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "intltest.h"
#include "unicode/rbnf.h"


class IntlTestRBNF : public IntlTest {
 public:

  
  virtual void runIndexedTest(int32_t index, UBool exec, const char* &name, char* par);

#if U_HAVE_RBNF
  


  virtual void TestAPI();

  void TestMultiplePluralRules();

  


  virtual void TestFractionalRuleSet();

#if 0
  


  virtual void TestLLong();
  virtual void TestLLongConstructors();
  virtual void TestLLongSimpleOperators();
#endif

  


  virtual void TestEnglishSpellout();

  


  virtual void TestOrdinalAbbreviations();

  


  virtual void TestDurations();

  


  virtual void TestSpanishSpellout();

  


  virtual void TestFrenchSpellout();

  


  virtual void TestSwissFrenchSpellout();

  


  virtual void TestBelgianFrenchSpellout();

  


  virtual void TestItalianSpellout();

  


  virtual void TestPortugueseSpellout();

  


  virtual void TestGermanSpellout();

  


  virtual void TestThaiSpellout();

  


  virtual void TestSwedishSpellout();

  


  virtual void TestSmallValues();

  


  virtual void TestLocalizations();

  


  virtual void TestAllLocales();

  


  virtual void TestHebrewFraction();

  



  virtual void TestMultiplierSubstitution();

  


  virtual void TestSetDecimalFormatSymbols();

  


  virtual void TestPluralRules();

 protected:
  virtual void doTest(RuleBasedNumberFormat* formatter, const char* const testData[][2], UBool testParsing);
  virtual void doLenientParseTest(RuleBasedNumberFormat* formatter, const char* testData[][2]);


#else

  virtual void TestRBNFDisabled();


#endif
};

#endif 


#endif
