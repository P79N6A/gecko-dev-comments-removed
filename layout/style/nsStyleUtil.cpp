




#include <math.h>

#include "mozilla/Util.h"

#include "nsStyleUtil.h"
#include "nsCRT.h"
#include "nsStyleConsts.h"

#include "nsGkAtoms.h"
#include "nsIContent.h"
#include "nsIDocument.h"
#include "nsINameSpaceManager.h"
#include "nsIURI.h"
#include "nsNetUtil.h"
#include "nsReadableUtils.h"
#include "nsContentUtils.h"
#include "nsTextFormatter.h"
#include "nsCSSProps.h"
#include "nsRuleNode.h"

using namespace mozilla;






bool nsStyleUtil::DashMatchCompare(const nsAString& aAttributeValue,
                                     const nsAString& aSelectorValue,
                                     const nsStringComparator& aComparator)
{
  bool result;
  PRUint32 selectorLen = aSelectorValue.Length();
  PRUint32 attributeLen = aAttributeValue.Length();
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

void nsStyleUtil::AppendEscapedCSSString(const nsString& aString,
                                         nsAString& aReturn)
{
  aReturn.Append(PRUnichar('"'));

  const nsString::char_type* in = aString.get();
  const nsString::char_type* const end = in + aString.Length();
  for (; in != end; in++)
  {
    if (*in < 0x20)
    {
     
   
     




     PRUnichar buf[5];
     nsTextFormatter::snprintf(buf, ArrayLength(buf), NS_LITERAL_STRING("\\%hX ").get(), *in);
     aReturn.Append(buf);
   
    } else switch (*in) {
      
      case '\\':
      case '\"':
      case '\'':
       aReturn.Append(PRUnichar('\\'));
      
      default:
       aReturn.Append(PRUnichar(*in));
    }
  }

  aReturn.Append(PRUnichar('"'));
}

 void
nsStyleUtil::AppendEscapedCSSIdent(const nsString& aIdent, nsAString& aReturn)
{
  
  
  
  
  
  
  
  

  const nsString::char_type* in = aIdent.get();
  const nsString::char_type* const end = in + aIdent.Length();

  
  
  if (in != end && *in == '-') {
    aReturn.Append(PRUnichar('-'));
    ++in;
  }

  bool first = true;
  for (; in != end; ++in, first = false)
  {
    if (*in < 0x20 || (first && '0' <= *in && *in <= '9'))
    {
      
      
      
      

      




      PRUnichar buf[5];
      nsTextFormatter::snprintf(buf, ArrayLength(buf),
                                NS_LITERAL_STRING("\\%hX ").get(), *in);
      aReturn.Append(buf);
    } else {
      PRUnichar ch = *in;
      if (!((ch == PRUnichar('_')) ||
            (PRUnichar('A') <= ch && ch <= PRUnichar('Z')) ||
            (PRUnichar('a') <= ch && ch <= PRUnichar('z')) ||
            PRUnichar(0x80) <= ch ||
            (!first && ch == PRUnichar('-')) ||
            (PRUnichar('0') <= ch && ch <= PRUnichar('9')))) {
        
        aReturn.Append(PRUnichar('\\'));
      }
      aReturn.Append(ch);
    }
  }
}

 void
nsStyleUtil::AppendBitmaskCSSValue(nsCSSProperty aProperty,
                                   PRInt32 aMaskedValue,
                                   PRInt32 aFirstMask,
                                   PRInt32 aLastMask,
                                   nsAString& aResult)
{
  for (PRInt32 mask = aFirstMask; mask <= aLastMask; mask <<= 1) {
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
  for (PRUint32 i = 0, numFeat = aFeatures.Length(); i < numFeat; i++) {
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
nsStyleUtil::ColorComponentToFloat(PRUint8 aAlpha)
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

