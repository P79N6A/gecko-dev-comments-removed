







































#ifndef nsStyleAnimation_h_
#define nsStyleAnimation_h_

#include "prtypes.h"
#include "nsAString.h"
#include "nsCSSProperty.h"

class nsCSSDeclaration;
class nsIContent;
class nsStyleCoord;
class nsStyleContext;




class nsStyleAnimation {
public:

  
  
  











  static PRBool Add(nsStyleCoord& aDest, const nsStyleCoord& aValueToAdd,
                    PRUint32 aCount);

  












  static PRBool ComputeDistance(const nsStyleCoord& aStartValue,
                                const nsStyleCoord& aEndValue,
                                double& aDistance);

  















  static PRBool Interpolate(const nsStyleCoord& aStartValue,
                            const nsStyleCoord& aEndValue,
                            double aPortion,
                            nsStyleCoord& aResultValue);

  
  
  



















  static PRBool ComputeValue(nsCSSProperty aProperty,
                             nsIContent* aElement,
                             const nsAString& aSpecifiedValue,
                             nsStyleCoord& aComputedValue);

  








  static PRBool ExtractComputedValue(nsCSSProperty aProperty,
                                     nsStyleContext* aStyleContext,
                                     nsStyleCoord& aComputedValue);
};

#endif
