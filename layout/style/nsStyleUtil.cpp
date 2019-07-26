




#include <math.h>

#include "mozilla/Util.h"

#include "nsStyleUtil.h"
#include "nsCRT.h"
#include "nsStyleConsts.h"

#include "nsGkAtoms.h"
#include "nsIContent.h"
#include "nsINameSpaceManager.h"
#include "nsIURI.h"
#include "nsNetUtil.h"
#include "nsReadableUtils.h"
#include "nsTextFormatter.h"
#include "nsCSSProps.h"
#include "nsRuleNode.h"

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

