



































#ifndef nsStyleUtil_h___
#define nsStyleUtil_h___

#include "nsCoord.h"
#include "nsCSSProperty.h"

class nsPresContext;
struct nsStyleBackground;
class nsString;
class nsStringComparator;
class nsIContent;

enum nsFontSizeType {
  eFontSize_HTML = 1,
  eFontSize_CSS = 2
};



class nsStyleUtil {
public:
  
  static nscoord CalcFontPointSize(PRInt32 aHTMLSize, PRInt32 aBasePointSize, 
                                   nsPresContext* aPresContext,
                                   nsFontSizeType aFontSizeType = eFontSize_HTML);

  static nscoord FindNextSmallerFontSize(nscoord aFontSize, PRInt32 aBasePointSize, 
                                         nsPresContext* aPresContext,
                                         nsFontSizeType aFontSizeType = eFontSize_HTML);

  static nscoord FindNextLargerFontSize(nscoord aFontSize, PRInt32 aBasePointSize, 
                                        nsPresContext* aPresContext,
                                        nsFontSizeType aFontSizeType = eFontSize_HTML);

  static PRInt32 ConstrainFontWeight(PRInt32 aWeight);

 static bool DashMatchCompare(const nsAString& aAttributeValue,
                                const nsAString& aSelectorValue,
                                const nsStringComparator& aComparator);
                                
  
  static void AppendEscapedCSSString(const nsString& aString,
                                     nsAString& aResult);
  
  
  
  static void AppendEscapedCSSIdent(const nsString& aIdent,
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

  


  static bool IsSignificantChild(const nsIContent* aChild,
                                   bool aTextIsSignificant,
                                   bool aWhitespaceIsSignificant);
};


#endif 
