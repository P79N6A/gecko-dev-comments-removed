



































#ifndef nsStyleUtil_h___
#define nsStyleUtil_h___

#include "nsCoord.h"
#include "nsPresContext.h"
#include "nsILinkHandler.h" 
#include "nsCSSProperty.h"

struct nsStyleBackground;

enum nsFontSizeType {
  eFontSize_HTML = 1,
  eFontSize_CSS = 2
};



class nsStyleUtil {
public:
  
  static float GetScalingFactor(PRInt32 aScaler);

  static nscoord CalcFontPointSize(PRInt32 aHTMLSize, PRInt32 aBasePointSize, 
                                   float aScalingFactor, nsPresContext* aPresContext,
                                   nsFontSizeType aFontSizeType = eFontSize_HTML);

  static nscoord FindNextSmallerFontSize(nscoord aFontSize, PRInt32 aBasePointSize, 
                                         float aScalingFactor, nsPresContext* aPresContext,
                                         nsFontSizeType aFontSizeType = eFontSize_HTML);

  static nscoord FindNextLargerFontSize(nscoord aFontSize, PRInt32 aBasePointSize, 
                                        float aScalingFactor, nsPresContext* aPresContext,
                                        nsFontSizeType aFontSizeType = eFontSize_HTML);

  static PRInt32 ConstrainFontWeight(PRInt32 aWeight);

  static PRBool IsHTMLLink(nsIContent *aContent, nsILinkHandler *aLinkHandler,
                           nsLinkState *aState);
  static PRBool IsLink(nsIContent *aContent, nsILinkHandler *aLinkHandler,
                       nsLinkState *aState);

 static PRBool DashMatchCompare(const nsAString& aAttributeValue,
                                const nsAString& aSelectorValue,
                                const nsStringComparator& aComparator);
                                
  
  static void AppendEscapedCSSString(const nsString& aString,
                                     nsAString& aResult);

  
  static void AppendBitmaskCSSValue(nsCSSProperty aProperty,
                                    PRInt32 aMaskedValue,
                                    PRInt32 aFirstMask,
                                    PRInt32 aLastMask,
                                    nsAString& aResult);

  



  static PRUint8 FloatToColorComponent(float aAlpha)
  {
    NS_ASSERTION(0.0 <= aAlpha && aAlpha <= 1.0, "out of range");
    return NSToIntRound(aAlpha * 255);
  }

  






  static float ColorComponentToFloat(PRUint8 aAlpha);

  


  static PRBool IsSignificantChild(nsIContent* aChild,
                                   PRBool aTextIsSignificant,
                                   PRBool aWhitespaceIsSignificant);
};


#endif 
