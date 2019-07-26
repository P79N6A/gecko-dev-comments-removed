




#include "nsStyleUtil.h"
#include "nsStyleConsts.h"

#include "nsIContent.h"
#include "nsCSSProps.h"
#include "nsRuleNode.h"
#include "nsROCSSPrimitiveValue.h"
#include "nsIContentPolicy.h"
#include "nsIContentSecurityPolicy.h"
#include "nsIURI.h"

using namespace mozilla;






bool nsStyleUtil::DashMatchCompare(const nsAString& aAttributeValue,
                                     const nsAString& aSelectorValue,
                                     const nsStringComparator& aComparator)
{
  bool result;
  uint32_t selectorLen = aSelectorValue.Length();
  uint32_t attributeLen = aAttributeValue.Length();
  if (selectorLen > attributeLen) {
    result = false;
  }
  else {
    nsAString::const_iterator iter;
    if (selectorLen != attributeLen &&
        *aAttributeValue.BeginReading(iter).advance(selectorLen) !=
            PRUnichar('-')) {
      
      
      
      result = false;
    }
    else {
      result = StringBeginsWith(aAttributeValue, aSelectorValue, aComparator);
    }
  }
  return result;
}

void nsStyleUtil::AppendEscapedCSSString(const nsAString& aString,
                                         nsAString& aReturn,
                                         PRUnichar quoteChar)
{
  NS_PRECONDITION(quoteChar == '\'' || quoteChar == '"',
                  "CSS strings must be quoted with ' or \"");
  aReturn.Append(quoteChar);

  const PRUnichar* in = aString.BeginReading();
  const PRUnichar* const end = aString.EndReading();
  for (; in != end; in++) {
    if (*in < 0x20 || (*in >= 0x7F && *in < 0xA0)) {
      
      aReturn.AppendPrintf("\\%hX ", *in);
    } else {
      if (*in == '"' || *in == '\'' || *in == '\\') {
        
        
        
        
        aReturn.Append(PRUnichar('\\'));
      }
      aReturn.Append(*in);
    }
  }

  aReturn.Append(quoteChar);
}

 void
nsStyleUtil::AppendEscapedCSSIdent(const nsAString& aIdent, nsAString& aReturn)
{
  
  
  
  
  
  
  
  

  const PRUnichar* in = aIdent.BeginReading();
  const PRUnichar* const end = aIdent.EndReading();

  if (in == end)
    return;

  
  
  if (in + 1 != end && *in == '-') {
    aReturn.Append(PRUnichar('-'));
    ++in;
  }

  
  
  
  
  
  if (in != end && (*in == '-' ||
                    ('0' <= *in && *in <= '9'))) {
    if (*in == '-') {
      aReturn.Append(PRUnichar('\\'));
      aReturn.Append(PRUnichar('-'));
    } else {
      aReturn.AppendPrintf("\\%hX ", *in);
    }
    ++in;
  }

  for (; in != end; ++in) {
    PRUnichar ch = *in;
    if (ch < 0x20 || (0x7F <= ch && ch < 0xA0)) {
      
      aReturn.AppendPrintf("\\%hX ", *in);
    } else {
      
      
      if (ch < 0x7F &&
          ch != '_' && ch != '-' &&
          (ch < '0' || '9' < ch) &&
          (ch < 'A' || 'Z' < ch) &&
          (ch < 'a' || 'z' < ch)) {
        aReturn.Append(PRUnichar('\\'));
      }
      aReturn.Append(ch);
    }
  }
}

 void
nsStyleUtil::AppendBitmaskCSSValue(nsCSSProperty aProperty,
                                   int32_t aMaskedValue,
                                   int32_t aFirstMask,
                                   int32_t aLastMask,
                                   nsAString& aResult)
{
  for (int32_t mask = aFirstMask; mask <= aLastMask; mask <<= 1) {
    if (mask & aMaskedValue) {
      AppendASCIItoUTF16(nsCSSProps::LookupPropertyValue(aProperty, mask),
                         aResult);
      aMaskedValue &= ~mask;
      if (aMaskedValue) { 
        aResult.Append(PRUnichar(' '));
      }
    }
  }
  NS_ABORT_IF_FALSE(aMaskedValue == 0, "unexpected bit remaining in bitfield");
}

 void
nsStyleUtil::AppendAngleValue(const nsStyleCoord& aAngle, nsAString& aResult)
{
  MOZ_ASSERT(aAngle.IsAngleValue(), "Should have angle value");

  nsROCSSPrimitiveValue tmpVal;
  nsAutoString tokenString;

  
  tmpVal.SetNumber(aAngle.GetAngleValue());
  tmpVal.GetCssText(tokenString);
  aResult.Append(tokenString);

  
  switch (aAngle.GetUnit()) {
    case eStyleUnit_Degree: aResult.AppendLiteral("deg");  break;
    case eStyleUnit_Grad:   aResult.AppendLiteral("grad"); break;
    case eStyleUnit_Radian: aResult.AppendLiteral("rad");  break;
    case eStyleUnit_Turn:   aResult.AppendLiteral("turn"); break;
    default: NS_NOTREACHED("unrecognized angle unit");
  }
}

 void
nsStyleUtil::AppendPaintOrderValue(uint8_t aValue,
                                   nsAString& aResult)
{
  static_assert
    (NS_STYLE_PAINT_ORDER_BITWIDTH * NS_STYLE_PAINT_ORDER_LAST_VALUE <= 8,
     "SVGStyleStruct::mPaintOrder and local variables not big enough");

  if (aValue == NS_STYLE_PAINT_ORDER_NORMAL) {
    aResult.AppendLiteral("normal");
    return;
  }

  
  static_assert(NS_STYLE_PAINT_ORDER_LAST_VALUE == 3,
                "paint-order values added; check serialization");

  
  

  const uint8_t MASK = (1 << NS_STYLE_PAINT_ORDER_BITWIDTH) - 1;

  uint32_t lastPositionToSerialize = 0;
  for (uint32_t position = NS_STYLE_PAINT_ORDER_LAST_VALUE - 1;
       position > 0;
       position--) {
    uint8_t component =
      (aValue >> (position * NS_STYLE_PAINT_ORDER_BITWIDTH)) & MASK;
    uint8_t earlierComponent =
      (aValue >> ((position - 1) * NS_STYLE_PAINT_ORDER_BITWIDTH)) & MASK;
    if (component < earlierComponent) {
      lastPositionToSerialize = position - 1;
      break;
    }
  }

  for (uint32_t position = 0; position <= lastPositionToSerialize; position++) {
    if (position > 0) {
      aResult.AppendLiteral(" ");
    }
    uint8_t component = aValue & MASK;
    switch (component) {
      case NS_STYLE_PAINT_ORDER_FILL:
        aResult.AppendLiteral("fill");
        break;

      case NS_STYLE_PAINT_ORDER_STROKE:
        aResult.AppendLiteral("stroke");
        break;

      case NS_STYLE_PAINT_ORDER_MARKERS:
        aResult.AppendLiteral("markers");
        break;

      default:
        NS_NOTREACHED("unexpected paint-order component value");
    }
    aValue >>= NS_STYLE_PAINT_ORDER_BITWIDTH;
  }
}

 void
nsStyleUtil::AppendFontFeatureSettings(const nsTArray<gfxFontFeature>& aFeatures,
                                       nsAString& aResult)
{
  for (uint32_t i = 0, numFeat = aFeatures.Length(); i < numFeat; i++) {
    const gfxFontFeature& feat = aFeatures[i];

    if (i != 0) {
        aResult.AppendLiteral(", ");
    }

    
    char tag[7];
    tag[0] = '"';
    tag[1] = (feat.mTag >> 24) & 0xff;
    tag[2] = (feat.mTag >> 16) & 0xff;
    tag[3] = (feat.mTag >> 8) & 0xff;
    tag[4] = feat.mTag & 0xff;
    tag[5] = '"';
    tag[6] = 0;
    aResult.AppendASCII(tag);

    
    if (feat.mValue == 0) {
      
      aResult.AppendLiteral(" off");
    } else if (feat.mValue > 1) {
      aResult.AppendLiteral(" ");
      aResult.AppendInt(feat.mValue);
    }
    
  }
}

 void
nsStyleUtil::AppendFontFeatureSettings(const nsCSSValue& aSrc,
                                       nsAString& aResult)
{
  nsCSSUnit unit = aSrc.GetUnit();

  if (unit == eCSSUnit_Normal) {
    aResult.AppendLiteral("normal");
    return;
  }

  NS_PRECONDITION(unit == eCSSUnit_PairList || unit == eCSSUnit_PairListDep,
                  "improper value unit for font-feature-settings:");

  nsTArray<gfxFontFeature> featureSettings;
  nsRuleNode::ComputeFontFeatures(aSrc.GetPairListValue(), featureSettings);
  AppendFontFeatureSettings(featureSettings, aResult);
}

 void
nsStyleUtil::GetFunctionalAlternatesName(int32_t aFeature,
                                         nsAString& aFeatureName)
{
  aFeatureName.Truncate();
  nsCSSKeyword key =
    nsCSSProps::ValueToKeywordEnum(aFeature,
                           nsCSSProps::kFontVariantAlternatesFuncsKTable);

  NS_ASSERTION(key != eCSSKeyword_UNKNOWN, "bad alternate feature type");
  AppendUTF8toUTF16(nsCSSKeywords::GetStringValue(key), aFeatureName);
}

 void
nsStyleUtil::SerializeFunctionalAlternates(
    const nsTArray<gfxAlternateValue>& aAlternates,
    nsAString& aResult)
{
  nsAutoString funcName, funcParams;
  uint32_t numValues = aAlternates.Length();

  uint32_t feature = 0;
  for (uint32_t i = 0; i < numValues; i++) {
    const gfxAlternateValue& v = aAlternates.ElementAt(i);
    if (feature != v.alternate) {
      feature = v.alternate;
      if (!funcName.IsEmpty() && !funcParams.IsEmpty()) {
        if (!aResult.IsEmpty()) {
          aResult.Append(PRUnichar(' '));
        }

        
        aResult.Append(funcName);
        aResult.Append(PRUnichar('('));
        aResult.Append(funcParams);
        aResult.Append(PRUnichar(')'));
      }

      
      GetFunctionalAlternatesName(v.alternate, funcName);
      NS_ASSERTION(!funcName.IsEmpty(), "unknown property value name");

      
      funcParams.Truncate();
      AppendEscapedCSSIdent(v.value, funcParams);
    } else {
      if (!funcParams.IsEmpty()) {
        funcParams.Append(NS_LITERAL_STRING(", "));
      }
      AppendEscapedCSSIdent(v.value, funcParams);
    }
  }

    
  if (!funcName.IsEmpty() && !funcParams.IsEmpty()) {
    if (!aResult.IsEmpty()) {
      aResult.Append(PRUnichar(' '));
    }

    aResult.Append(funcName);
    aResult.Append(PRUnichar('('));
    aResult.Append(funcParams);
    aResult.Append(PRUnichar(')'));
  }
}

 void
nsStyleUtil::ComputeFunctionalAlternates(const nsCSSValueList* aList,
                                  nsTArray<gfxAlternateValue>& aAlternateValues)
{
  gfxAlternateValue v;

  aAlternateValues.Clear();
  for (const nsCSSValueList* curr = aList; curr != nullptr; curr = curr->mNext) {
    
    if (curr->mValue.GetUnit() != eCSSUnit_Function) {
      continue;
    }

    
    const nsCSSValue::Array *func = curr->mValue.GetArrayValue();

    
    nsCSSKeyword key = func->Item(0).GetKeywordValue();
    NS_ASSERTION(key != eCSSKeyword_UNKNOWN, "unknown alternate property value");

    int32_t alternate;
    if (key == eCSSKeyword_UNKNOWN ||
        !nsCSSProps::FindKeyword(key,
                                 nsCSSProps::kFontVariantAlternatesFuncsKTable,
                                 alternate)) {
      NS_NOTREACHED("keyword not a font-variant-alternates value");
      continue;
    }
    v.alternate = alternate;

    
    
    uint32_t numElems = func->Count();
    for (uint32_t i = 1; i < numElems; i++) {
      const nsCSSValue& value = func->Item(i);
      NS_ASSERTION(value.GetUnit() == eCSSUnit_Ident,
                   "weird unit found in variant alternate");
      if (value.GetUnit() != eCSSUnit_Ident) {
        continue;
      }
      value.GetStringValue(v.value);
      aAlternateValues.AppendElement(v);
    }
  }
}

 float
nsStyleUtil::ColorComponentToFloat(uint8_t aAlpha)
{
  
  
  
  
  float rounded = NS_roundf(float(aAlpha) * 100.0f / 255.0f) / 100.0f;
  if (FloatToColorComponent(rounded) != aAlpha) {
    
    rounded = NS_roundf(float(aAlpha) * 1000.0f / 255.0f) / 1000.0f;
  }
  return rounded;
}

 bool
nsStyleUtil::IsSignificantChild(nsIContent* aChild, bool aTextIsSignificant,
                                bool aWhitespaceIsSignificant)
{
  NS_ASSERTION(!aWhitespaceIsSignificant || aTextIsSignificant,
               "Nonsensical arguments");

  bool isText = aChild->IsNodeOfType(nsINode::eTEXT);

  if (!isText && !aChild->IsNodeOfType(nsINode::eCOMMENT) &&
      !aChild->IsNodeOfType(nsINode::ePROCESSING_INSTRUCTION)) {
    return true;
  }

  return aTextIsSignificant && isText && aChild->TextLength() != 0 &&
         (aWhitespaceIsSignificant ||
          !aChild->TextIsOnlyWhitespace());
}

 bool
nsStyleUtil::CSPAllowsInlineStyle(nsIContent* aContent,
                                  nsIPrincipal* aPrincipal,
                                  nsIURI* aSourceURI,
                                  uint32_t aLineNumber,
                                  const nsSubstring& aStyleText,
                                  nsresult* aRv)
{
  nsresult rv;

  if (aRv) {
    *aRv = NS_OK;
  }

  MOZ_ASSERT(!aContent || aContent->Tag() == nsGkAtoms::style,
      "aContent passed to CSPAllowsInlineStyle "
      "for an element that is not <style>");

  nsCOMPtr<nsIContentSecurityPolicy> csp;
  rv = aPrincipal->GetCsp(getter_AddRefs(csp));

  if (NS_FAILED(rv)) {
    if (aRv)
      *aRv = rv;
    return false;
  }

  if (csp) {
    bool reportViolation;
    bool allowInlineStyle = true;
    rv = csp->GetAllowsInlineStyle(&reportViolation, &allowInlineStyle);
    if (NS_FAILED(rv)) {
      if (aRv)
        *aRv = rv;
      return false;
    }

    bool foundNonce = false;
    nsAutoString nonce;
    
    
    if (!allowInlineStyle) {
      
      foundNonce = !!aContent &&
        aContent->GetAttr(kNameSpaceID_None, nsGkAtoms::nonce, nonce);
      if (foundNonce) {
        
        
        
        
        
        
        rv = csp->GetAllowsNonce(nonce, nsIContentPolicy::TYPE_STYLESHEET,
                                 &reportViolation, &allowInlineStyle);
        if (NS_FAILED(rv)) {
          if (aRv)
            *aRv = rv;
          return false;
        }
      }
    }

    if (reportViolation) {
      
      nsAutoCString asciiSpec;
      aSourceURI->GetAsciiSpec(asciiSpec);
      nsAutoString styleText(aStyleText);

      
      if (styleText.Length() > 40) {
        styleText.Truncate(40);
        styleText.AppendLiteral("...");
      }

      
      
      unsigned short violationType = foundNonce ?
        nsIContentSecurityPolicy::VIOLATION_TYPE_NONCE_STYLE :
        nsIContentSecurityPolicy::VIOLATION_TYPE_INLINE_STYLE;
      csp->LogViolationDetails(violationType, NS_ConvertUTF8toUTF16(asciiSpec),
                               styleText, aLineNumber, nonce);
    }

    if (!allowInlineStyle) {
      NS_ASSERTION(reportViolation,
          "CSP blocked inline style but is not reporting a violation");
      
      return false;
    }
  }
  
  return true;
}
