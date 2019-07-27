




#include "nsStyleUtil.h"
#include "nsStyleConsts.h"

#include "nsIContent.h"
#include "nsCSSProps.h"
#include "nsRuleNode.h"
#include "nsROCSSPrimitiveValue.h"
#include "nsStyleStruct.h"
#include "nsIContentPolicy.h"
#include "nsIContentSecurityPolicy.h"
#include "nsIURI.h"
#include "nsPrintfCString.h"

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
            char16_t('-')) {
      
      
      
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
                                         char16_t quoteChar)
{
  NS_PRECONDITION(quoteChar == '\'' || quoteChar == '"',
                  "CSS strings must be quoted with ' or \"");
  aReturn.Append(quoteChar);

  const char16_t* in = aString.BeginReading();
  const char16_t* const end = aString.EndReading();
  for (; in != end; in++) {
    if (*in < 0x20 || (*in >= 0x7F && *in < 0xA0)) {
      
      aReturn.AppendPrintf("\\%hx ", *in);
    } else {
      if (*in == '"' || *in == '\'' || *in == '\\') {
        
        
        
        
        aReturn.Append(char16_t('\\'));
      }
      aReturn.Append(*in);
    }
  }

  aReturn.Append(quoteChar);
}

 bool
nsStyleUtil::AppendEscapedCSSIdent(const nsAString& aIdent, nsAString& aReturn)
{
  
  
  
  
  
  
  
  
  
  
  

  const char16_t* in = aIdent.BeginReading();
  const char16_t* const end = aIdent.EndReading();

  if (in == end)
    return true;

  
  
  if (*in == '-') {
    if (in + 1 == end) {
      aReturn.Append(char16_t('\\'));
      aReturn.Append(char16_t('-'));
      return true;
    }

    aReturn.Append(char16_t('-'));
    ++in;
  }

  
  
  
  if (in != end && ('0' <= *in && *in <= '9')) {
    aReturn.AppendPrintf("\\%hx ", *in);
    ++in;
  }

  for (; in != end; ++in) {
    char16_t ch = *in;
    if (ch == 0x00) {
      return false;
    }
    if (ch < 0x20 || (0x7F <= ch && ch < 0xA0)) {
      
      aReturn.AppendPrintf("\\%hx ", *in);
    } else {
      
      
      if (ch < 0x7F &&
          ch != '_' && ch != '-' &&
          (ch < '0' || '9' < ch) &&
          (ch < 'A' || 'Z' < ch) &&
          (ch < 'a' || 'z' < ch)) {
        aReturn.Append(char16_t('\\'));
      }
      aReturn.Append(ch);
    }
  }
  return true;
}



static void
AppendUnquotedFamilyName(const nsAString& aFamilyName, nsAString& aResult)
{
  const char16_t *p, *p_end;
  aFamilyName.BeginReading(p);
  aFamilyName.EndReading(p_end);

   bool moreThanOne = false;
   while (p < p_end) {
     const char16_t* identStart = p;
     while (++p != p_end && *p != ' ')
        ;

     nsDependentSubstring ident(identStart, p);
     if (!ident.IsEmpty()) {
       if (moreThanOne) {
         aResult.Append(' ');
       }
       nsStyleUtil::AppendEscapedCSSIdent(ident, aResult);
       moreThanOne = true;
     }

     ++p;
  }
}

 void
nsStyleUtil::AppendEscapedCSSFontFamilyList(
  const mozilla::FontFamilyList& aFamilyList,
  nsAString& aResult)
{
  const nsTArray<FontFamilyName>& fontlist = aFamilyList.GetFontlist();
  size_t i, len = fontlist.Length();
  for (i = 0; i < len; i++) {
    if (i != 0) {
      aResult.Append(',');
    }
    const FontFamilyName& name = fontlist[i];
    switch (name.mType) {
      case eFamily_named:
        AppendUnquotedFamilyName(name.mName, aResult);
        break;
      case eFamily_named_quoted:
        AppendEscapedCSSString(name.mName, aResult);
        break;
      default:
        name.AppendToString(aResult);
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
        aResult.Append(char16_t(' '));
      }
    }
  }
  MOZ_ASSERT(aMaskedValue == 0, "unexpected bit remaining in bitfield");
}

 void
nsStyleUtil::AppendAngleValue(const nsStyleCoord& aAngle, nsAString& aResult)
{
  MOZ_ASSERT(aAngle.IsAngleValue(), "Should have angle value");

  
  AppendCSSNumber(aAngle.GetAngleValue(), aResult);

  
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
      aResult.Append(' ');
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
      aResult.Append(' ');
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
          aResult.Append(char16_t(' '));
        }

        
        aResult.Append(funcName);
        aResult.Append(char16_t('('));
        aResult.Append(funcParams);
        aResult.Append(char16_t(')'));
      }

      
      GetFunctionalAlternatesName(v.alternate, funcName);
      NS_ASSERTION(!funcName.IsEmpty(), "unknown property value name");

      
      funcParams.Truncate();
      AppendEscapedCSSIdent(v.value, funcParams);
    } else {
      if (!funcParams.IsEmpty()) {
        funcParams.AppendLiteral(", ");
      }
      AppendEscapedCSSIdent(v.value, funcParams);
    }
  }

    
  if (!funcName.IsEmpty() && !funcParams.IsEmpty()) {
    if (!aResult.IsEmpty()) {
      aResult.Append(char16_t(' '));
    }

    aResult.Append(funcName);
    aResult.Append(char16_t('('));
    aResult.Append(funcParams);
    aResult.Append(char16_t(')'));
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

static void
AppendSerializedUnicodePoint(uint32_t aCode, nsACString& aBuf)
{
  aBuf.Append(nsPrintfCString("%0X", aCode));
}





 void
nsStyleUtil::AppendUnicodeRange(const nsCSSValue& aValue, nsAString& aResult)
{
  NS_PRECONDITION(aValue.GetUnit() == eCSSUnit_Null ||
                  aValue.GetUnit() == eCSSUnit_Array,
                  "improper value unit for unicode-range:");
  aResult.Truncate();
  if (aValue.GetUnit() != eCSSUnit_Array)
    return;

  nsCSSValue::Array const & sources = *aValue.GetArrayValue();
  nsAutoCString buf;

  MOZ_ASSERT(sources.Count() % 2 == 0,
             "odd number of entries in a unicode-range: array");

  for (uint32_t i = 0; i < sources.Count(); i += 2) {
    uint32_t min = sources[i].GetIntValue();
    uint32_t max = sources[i+1].GetIntValue();

    
    buf.AppendLiteral("U+");
    AppendSerializedUnicodePoint(min, buf);

    if (min != max) {
      buf.Append('-');
      AppendSerializedUnicodePoint(max, buf);
    }
    buf.AppendLiteral(", ");
  }
  buf.Truncate(buf.Length() - 2); 
  CopyASCIItoUTF16(buf, aResult);
}

 void
nsStyleUtil::AppendSerializedFontSrc(const nsCSSValue& aValue,
                                     nsAString& aResult)
{
  
  
  
  
  

  NS_PRECONDITION(aValue.GetUnit() == eCSSUnit_Array,
                  "improper value unit for src:");

  const nsCSSValue::Array& sources = *aValue.GetArrayValue();
  size_t i = 0;

  while (i < sources.Count()) {
    nsAutoString formats;

    if (sources[i].GetUnit() == eCSSUnit_URL) {
      aResult.AppendLiteral("url(");
      nsDependentString url(sources[i].GetOriginalURLValue());
      nsStyleUtil::AppendEscapedCSSString(url, aResult);
      aResult.Append(')');
    } else if (sources[i].GetUnit() == eCSSUnit_Local_Font) {
      aResult.AppendLiteral("local(");
      nsDependentString local(sources[i].GetStringBufferValue());
      nsStyleUtil::AppendEscapedCSSString(local, aResult);
      aResult.Append(')');
    } else {
      NS_NOTREACHED("entry in src: descriptor with improper unit");
      i++;
      continue;
    }

    i++;
    formats.Truncate();
    while (i < sources.Count() &&
           sources[i].GetUnit() == eCSSUnit_Font_Format) {
      formats.Append('"');
      formats.Append(sources[i].GetStringBufferValue());
      formats.AppendLiteral("\", ");
      i++;
    }
    if (formats.Length() > 0) {
      formats.Truncate(formats.Length() - 2); 
      aResult.AppendLiteral(" format(");
      aResult.Append(formats);
      aResult.Append(')');
    }
    aResult.AppendLiteral(", ");
  }
  aResult.Truncate(aResult.Length() - 2); 
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




typedef nsStyleBackground::Position::PositionCoord PositionCoord;
static bool
ObjectPositionCoordMightCauseOverflow(const PositionCoord& aCoord)
{
  
  
  
  if (aCoord.mLength != 0) {
    return true;
  }

  
  
  
  if (aCoord.mHasPercent &&
      (aCoord.mPercent < 0.0f || aCoord.mPercent > 1.0f)) {
    return true;
  }
  return false;
}


 bool
nsStyleUtil::ObjectPropsMightCauseOverflow(const nsStylePosition* aStylePos)
{
  auto objectFit = aStylePos->mObjectFit;

  
  
  if (objectFit == NS_STYLE_OBJECT_FIT_COVER ||
      objectFit == NS_STYLE_OBJECT_FIT_NONE) {
    return true;
  }
  
  

  
  
  const nsStyleBackground::Position& objectPosistion = aStylePos->mObjectPosition;
  if (ObjectPositionCoordMightCauseOverflow(objectPosistion.mXPosition) ||
      ObjectPositionCoordMightCauseOverflow(objectPosistion.mYPosition)) {
    return true;
  }

  return false;
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

  MOZ_ASSERT(!aContent || aContent->NodeInfo()->NameAtom() == nsGkAtoms::style,
      "aContent passed to CSPAllowsInlineStyle "
      "for an element that is not <style>");

  nsCOMPtr<nsIContentSecurityPolicy> csp;
  rv = aPrincipal->GetCsp(getter_AddRefs(csp));

  if (NS_FAILED(rv)) {
    if (aRv)
      *aRv = rv;
    return false;
  }

  if (!csp) {
    
    return true;
  }

  
  
  
  
  
  
  bool allowInlineStyle = true;
  nsAutoTArray<unsigned short, 3> violations;

  bool reportInlineViolation;
  rv = csp->GetAllowsInlineStyle(&reportInlineViolation, &allowInlineStyle);
  if (NS_FAILED(rv)) {
    if (aRv)
      *aRv = rv;
    return false;
  }
  if (reportInlineViolation) {
    violations.AppendElement(static_cast<unsigned short>(
          nsIContentSecurityPolicy::VIOLATION_TYPE_INLINE_STYLE));
  }

  nsAutoString nonce;
  if (!allowInlineStyle) {
    
    bool foundNonce = !!aContent &&
      aContent->GetAttr(kNameSpaceID_None, nsGkAtoms::nonce, nonce);
    if (foundNonce) {
      bool reportNonceViolation;
      rv = csp->GetAllowsNonce(nonce, nsIContentPolicy::TYPE_STYLESHEET,
                               &reportNonceViolation, &allowInlineStyle);
      if (NS_FAILED(rv)) {
        if (aRv)
          *aRv = rv;
        return false;
      }

      if (reportNonceViolation) {
        violations.AppendElement(static_cast<unsigned short>(
              nsIContentSecurityPolicy::VIOLATION_TYPE_NONCE_STYLE));
      }
    }
  }

  if (!allowInlineStyle) {
    bool reportHashViolation;
    rv = csp->GetAllowsHash(aStyleText, nsIContentPolicy::TYPE_STYLESHEET,
                            &reportHashViolation, &allowInlineStyle);
    if (NS_FAILED(rv)) {
      if (aRv)
        *aRv = rv;
      return false;
    }
    if (reportHashViolation) {
      violations.AppendElement(static_cast<unsigned short>(
            nsIContentSecurityPolicy::VIOLATION_TYPE_HASH_STYLE));
    }
  }

  
  
  
  
  
  
  
  
  
  
  
  
  if (!violations.IsEmpty()) {
    MOZ_ASSERT(violations[0] == nsIContentSecurityPolicy::VIOLATION_TYPE_INLINE_STYLE,
               "How did we get any violations without an initial inline style violation?");
    
    nsAutoCString asciiSpec;
    aSourceURI->GetAsciiSpec(asciiSpec);
    nsAutoString styleSample(aStyleText);

    
    if (styleSample.Length() > 40) {
      styleSample.Truncate(40);
      styleSample.AppendLiteral("...");
    }

    for (uint32_t i = 0; i < violations.Length(); i++) {
      
      
      if (i > 0 || violations.Length() == 1) {
        csp->LogViolationDetails(violations[i], NS_ConvertUTF8toUTF16(asciiSpec),
                                 styleSample, aLineNumber, nonce, aStyleText);
      }
    }
  }

  if (!allowInlineStyle) {
    NS_ASSERTION(!violations.IsEmpty(),
        "CSP blocked inline style but is not reporting a violation");
    
    return false;
  }
  
  return true;
}
