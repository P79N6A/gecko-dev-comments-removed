




#ifndef NS_SMILPARSERUTILS_H_
#define NS_SMILPARSERUTILS_H_

#include "nscore.h"
#include "nsTArray.h"
#include "nsString.h"

class nsISMILAttr;
class nsSMILTimeValue;
class nsSMILValue;
class nsSMILRepeatCount;
class nsSMILTimeValueSpecParams;

namespace mozilla {
namespace dom {
class SVGAnimationElement;
}
}






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
                              const mozilla::dom::SVGAnimationElement* aSrcElement,
                              const nsISMILAttr& aAttribute,
                              nsTArray<nsSMILValue>& aValuesArray,
                              bool& aPreventCachingOfSandwich);

  
  
  static nsresult ParseValuesGeneric(const nsAString& aSpec,
                                     GenericValueParser& aParser);

  static nsresult ParseRepeatCount(const nsAString& aSpec,
                                   nsSMILRepeatCount& aResult);

  static nsresult ParseTimeValueSpecParams(const nsAString& aSpec,
                                           nsSMILTimeValueSpecParams& aResult);


  
  static const int8_t kClockValueAllowSign       = 1;
  
  static const int8_t kClockValueAllowIndefinite = 2;

  

























  static nsresult ParseClockValue(const nsAString& aSpec,
                                  nsSMILTimeValue* aResult,
                                  uint32_t aFlags = 0,
                                  bool* aIsMedia = nullptr);

  






  static int32_t CheckForNegativeNumber(const nsAString& aStr);
};

#endif 
