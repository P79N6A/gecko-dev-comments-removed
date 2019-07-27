



#ifndef nsStyleUtil_h___
#define nsStyleUtil_h___

#include "nsCoord.h"
#include "nsCSSProperty.h"
#include "nsString.h"
#include "nsTArrayForwardDeclare.h"
#include "gfxFontFamilyList.h"

class nsCSSValue;
class nsStringComparator;
class nsStyleCoord;
class nsIContent;
class nsIPrincipal;
class nsIURI;
struct gfxFontFeature;
struct gfxAlternateValue;
struct nsCSSValueList;


class nsStyleUtil {
public:

 static bool DashMatchCompare(const nsAString& aAttributeValue,
                                const nsAString& aSelectorValue,
                                const nsStringComparator& aComparator);

  
  
  static void AppendEscapedCSSString(const nsAString& aString,
                                     nsAString& aResult,
                                     char16_t quoteChar = '"');

  
  
  
  
  
  static bool AppendEscapedCSSIdent(const nsAString& aIdent,
                                    nsAString& aResult);

  static void
  AppendEscapedCSSFontFamilyList(const mozilla::FontFamilyList& aFamilyList,
                                 nsAString& aResult);

  
  static void AppendBitmaskCSSValue(nsCSSProperty aProperty,
                                    int32_t aMaskedValue,
                                    int32_t aFirstMask,
                                    int32_t aLastMask,
                                    nsAString& aResult);

  static void AppendAngleValue(const nsStyleCoord& aValue, nsAString& aResult);

  static void AppendPaintOrderValue(uint8_t aValue, nsAString& aResult);

  static void AppendFontFeatureSettings(const nsTArray<gfxFontFeature>& aFeatures,
                                        nsAString& aResult);

  static void AppendFontFeatureSettings(const nsCSSValue& src,
                                        nsAString& aResult);

  static void AppendUnicodeRange(const nsCSSValue& aValue, nsAString& aResult);

  static void AppendCSSNumber(float aNumber, nsAString& aResult)
  {
    aResult.AppendFloat(aNumber);
  }

  static void AppendSerializedFontSrc(const nsCSSValue& aValue,
                                      nsAString& aResult);

  
  static void GetFunctionalAlternatesName(int32_t aFeature,
                                          nsAString& aFeatureName);

  
  static void
  SerializeFunctionalAlternates(const nsTArray<gfxAlternateValue>& aAlternates,
                                nsAString& aResult);

  
  static void
  ComputeFunctionalAlternates(const nsCSSValueList* aList,
                              nsTArray<gfxAlternateValue>& aAlternateValues);

  



  static uint8_t FloatToColorComponent(float aAlpha)
  {
    NS_ASSERTION(0.0 <= aAlpha && aAlpha <= 1.0, "out of range");
    return NSToIntRound(aAlpha * 255);
  }

  






  static float ColorComponentToFloat(uint8_t aAlpha);

  


  static bool IsSignificantChild(nsIContent* aChild,
                                   bool aTextIsSignificant,
                                   bool aWhitespaceIsSignificant);

  












  static bool IsFlexBasisMainSize(const nsStyleCoord& aFlexBasis,
                                  bool aIsMainAxisHorizontal);

  

























  static bool CSPAllowsInlineStyle(nsIContent* aContent,
                                   nsIPrincipal* aPrincipal,
                                   nsIURI* aSourceURI,
                                   uint32_t aLineNumber,
                                   const nsSubstring& aStyleText,
                                   nsresult* aRv);

};


#endif 
