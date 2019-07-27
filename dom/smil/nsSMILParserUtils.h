





#ifndef NS_SMILPARSERUTILS_H_
#define NS_SMILPARSERUTILS_H_

#include "nsTArray.h"
#include "nsStringFwd.h"

class nsISMILAttr;
class nsSMILKeySpline;
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
  
  class MOZ_STACK_CLASS GenericValueParser {
  public:
    virtual bool Parse(const nsAString& aValueStr) = 0;
  };

  static const nsDependentSubstring TrimWhitespace(const nsAString& aString);

  static bool ParseKeySplines(const nsAString& aSpec,
                              FallibleTArray<nsSMILKeySpline>& aKeySplines);

  
  static bool ParseSemicolonDelimitedProgressList(const nsAString& aSpec,
                                                  bool aNonDecreasing,
                                                  FallibleTArray<double>& aArray);

  static bool ParseValues(const nsAString& aSpec,
                          const mozilla::dom::SVGAnimationElement* aSrcElement,
                          const nsISMILAttr& aAttribute,
                          FallibleTArray<nsSMILValue>& aValuesArray,
                          bool& aPreventCachingOfSandwich);

  
  
  static bool ParseValuesGeneric(const nsAString& aSpec,
                                 GenericValueParser& aParser);

  static bool ParseRepeatCount(const nsAString& aSpec,
                               nsSMILRepeatCount& aResult);

  static bool ParseTimeValueSpecParams(const nsAString& aSpec,
                                       nsSMILTimeValueSpecParams& aResult);

  








  static bool ParseClockValue(const nsAString& aSpec,
                              nsSMILTimeValue* aResult);

  






  static int32_t CheckForNegativeNumber(const nsAString& aStr);
};

#endif 
