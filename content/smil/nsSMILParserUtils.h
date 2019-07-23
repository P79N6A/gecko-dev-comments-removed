




































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






class nsSMILParserUtils
{
public:
  static nsresult ParseKeySplines(const nsAString& aSpec,
                                  nsTArray<double>& aSplineArray);

  static nsresult ParseKeyTimes(const nsAString& aSpec,
                                nsTArray<double>& aTimesArray);

  static nsresult ParseValues(const nsAString& aSpec,
                              const nsISMILAnimationElement* aSrcElement,
                              const nsISMILAttr& aAttribute,
                              nsTArray<nsSMILValue>& aValuesArray);

  static nsresult ParseRepeatCount(const nsAString& aSpec,
                                   nsSMILRepeatCount& aResult);

  
  static const PRInt8 kClockValueAllowSign       = 1;
  
  static const PRInt8 kClockValueAllowIndefinite = 2;

  

























  static nsresult ParseClockValue(const nsAString& aSpec,
                                  nsSMILTimeValue* aResult,
                                  PRUint32 aFlags = 0,
                                  PRBool* aIsMedia = nsnull);

  






  static PRInt32 CheckForNegativeNumber(const nsAString& aStr);

private:
  static void   SkipWsp(nsACString::const_iterator& aIter,
                        const nsACString::const_iterator& aIterEnd);
  static void   SkipWsp(nsAString::const_iterator& aIter,
                        const nsAString::const_iterator& aIterEnd);
  static double GetFloat(nsACString::const_iterator& aIter,
                         const nsACString::const_iterator& aIterEnd,
                         nsresult *aErrorCode = nsnull);
  static PRBool IsSpace(const PRUnichar c);
  static PRBool ConsumeSubstring(nsACString::const_iterator& aIter,
                                 const nsACString::const_iterator& aIterEnd,
                                 const char *aSubstring);
  static PRBool ParseClockComponent(nsACString::const_iterator& aSpec,
                                    const nsACString::const_iterator& aEnd,
                                    double& aResult,
                                    PRBool& aIsReal,
                                    PRBool& aCouldBeMin,
                                    PRBool& aCouldBeSec);
  static PRBool ParseMetricMultiplicand(nsACString::const_iterator& aSpec,
                                        const nsACString::const_iterator& aEnd,
                                        PRInt32& multiplicand);

  static const PRUint32 MSEC_PER_SEC;
  static const PRUint32 MSEC_PER_MIN;
  static const PRUint32 MSEC_PER_HOUR;
};

#endif 
