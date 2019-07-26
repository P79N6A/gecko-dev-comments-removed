



#ifndef nsStyleUtil_h___
#define nsStyleUtil_h___

#include "nsCoord.h"
#include "nsCSSProperty.h"
#include "gfxFontFeatures.h"
#include "nsTArray.h"
#include "nsCSSValue.h"

struct nsStyleBackground;
class nsString;
class nsStringComparator;
class nsIContent;




class nsStyleUtil {
public:

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

  static void AppendFontFeatureSettings(const nsTArray<gfxFontFeature>& aFeatures,
                                        nsAString& aResult);

  static void AppendFontFeatureSettings(const nsCSSValue& src,
                                        nsAString& aResult);

  



  static PRUint8 FloatToColorComponent(float aAlpha)
  {
    NS_ASSERTION(0.0 <= aAlpha && aAlpha <= 1.0, "out of range");
    return NSToIntRound(aAlpha * 255);
  }

  






  static float ColorComponentToFloat(PRUint8 aAlpha);

  


  static bool IsSignificantChild(nsIContent* aChild,
                                   bool aTextIsSignificant,
                                   bool aWhitespaceIsSignificant);
};


#endif 
