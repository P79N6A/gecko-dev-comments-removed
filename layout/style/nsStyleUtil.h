



#ifndef nsStyleUtil_h___
#define nsStyleUtil_h___

#include "nsCoord.h"
#include "nsCSSProperty.h"

class nsCSSValue;
class nsStringComparator;
class nsIContent;
struct gfxFontFeature;
template <class E> class nsTArray;


class nsStyleUtil {
public:

 static bool DashMatchCompare(const nsAString& aAttributeValue,
                                const nsAString& aSelectorValue,
                                const nsStringComparator& aComparator);

  
  
  static void AppendEscapedCSSString(const nsAString& aString,
                                     nsAString& aResult,
                                     PRUnichar quoteChar = '"');

  
  
  
  static void AppendEscapedCSSIdent(const nsAString& aIdent,
                                    nsAString& aResult);

  
  static void AppendBitmaskCSSValue(nsCSSProperty aProperty,
                                    int32_t aMaskedValue,
                                    int32_t aFirstMask,
                                    int32_t aLastMask,
                                    nsAString& aResult);

  static void AppendPaintOrderValue(uint8_t aValue, nsAString& aResult);

  static void AppendFontFeatureSettings(const nsTArray<gfxFontFeature>& aFeatures,
                                        nsAString& aResult);

  static void AppendFontFeatureSettings(const nsCSSValue& src,
                                        nsAString& aResult);

  



  static uint8_t FloatToColorComponent(float aAlpha)
  {
    NS_ASSERTION(0.0 <= aAlpha && aAlpha <= 1.0, "out of range");
    return NSToIntRound(aAlpha * 255);
  }

  






  static float ColorComponentToFloat(uint8_t aAlpha);

  


  static bool IsSignificantChild(nsIContent* aChild,
                                   bool aTextIsSignificant,
                                   bool aWhitespaceIsSignificant);
};


#endif 
