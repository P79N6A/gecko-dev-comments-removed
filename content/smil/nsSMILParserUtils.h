




































#ifndef NS_SMILPARSERUTILS_H_
#define NS_SMILPARSERUTILS_H_

#include "nscore.h"
#include "nsTArray.h"
#include "nsString.h"

class nsISMILAttr;
class nsISMILAnimationElement;
class nsSMILTimeValue;
class nsSMILValue;
class nsSMILRepeatCount;
class nsSMILTimeValueSpecParams;






class nsSMILParserUtils
{
public:
  
  class GenericValueParser {
  public:
    virtual nsresult Parse(const nsAString& aValueStr) = 0;
  };

  static nsresult ParseKeySplines(const nsAString& aSpec,
                                  nsTArray<double>& aSplineArray);

  
  static nsresult ParseSemicolonDelimitedProgressList(const nsAString& aSpec,
                                                      bool aNonDecreasing,
                                                      nsTArray<double>& aArray);

  static nsresult ParseValues(const nsAString& aSpec,
                              const nsISMILAnimationElement* aSrcElement,
                              const nsISMILAttr& aAttribute,
                              nsTArray<nsSMILValue>& aValuesArray,
                              bool& aPreventCachingOfSandwich);

  
  
  static nsresult ParseValuesGeneric(const nsAString& aSpec,
                                     GenericValueParser& aParser);

  static nsresult ParseRepeatCount(const nsAString& aSpec,
                                   nsSMILRepeatCount& aResult);

  static nsresult ParseTimeValueSpecParams(const nsAString& aSpec,
                                           nsSMILTimeValueSpecParams& aResult);


  
  static const PRInt8 kClockValueAllowSign       = 1;
  
  static const PRInt8 kClockValueAllowIndefinite = 2;

  

























  static nsresult ParseClockValue(const nsAString& aSpec,
                                  nsSMILTimeValue* aResult,
                                  PRUint32 aFlags = 0,
                                  bool* aIsMedia = nsnull);

  






  static PRInt32 CheckForNegativeNumber(const nsAString& aStr);
};

#endif 
