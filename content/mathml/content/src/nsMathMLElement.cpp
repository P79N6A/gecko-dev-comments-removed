






































#include "nsMathMLElement.h"
#include "nsDOMClassInfoID.h" 
#include "nsGkAtoms.h"
#include "nsCRT.h"
#include "nsRuleData.h"
#include "nsCSSValue.h"
#include "nsMappedAttributes.h"
#include "nsStyleConsts.h"
#include "nsIDocument.h"
#include "nsIEventStateManager.h"
#include "nsPresShellIterator.h"
#include "nsIPresShell.h"
#include "nsPresContext.h"
#include "nsDOMClassInfoID.h"
#include "mozAutoDocUpdate.h"




NS_INTERFACE_TABLE_HEAD(nsMathMLElement)
  NS_NODE_OFFSET_AND_INTERFACE_TABLE_BEGIN(nsMathMLElement)
    NS_INTERFACE_TABLE_ENTRY(nsMathMLElement, nsIDOMNode)
    NS_INTERFACE_TABLE_ENTRY(nsMathMLElement, nsIDOMElement)
  NS_OFFSET_AND_INTERFACE_TABLE_END
  NS_ELEMENT_INTERFACE_TABLE_TO_MAP_SEGUE
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(MathMLElement)
NS_ELEMENT_INTERFACE_MAP_END

NS_IMPL_ADDREF_INHERITED(nsMathMLElement, nsMathMLElementBase)
NS_IMPL_RELEASE_INHERITED(nsMathMLElement, nsMathMLElementBase)

nsresult
nsMathMLElement::BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                            nsIContent* aBindingParent,
                            PRBool aCompileEventHandlers)
{
  static const char kMathMLStyleSheetURI[] = "resource://gre/res/mathml.css";

  nsresult rv = nsMathMLElementBase::BindToTree(aDocument, aParent,
                                                aBindingParent,
                                                aCompileEventHandlers);
  NS_ENSURE_SUCCESS(rv, rv);

  if (aDocument && !aDocument->GetMathMLEnabled()) {
    
    
    
    aDocument->SetMathMLEnabled();
    aDocument->EnsureCatalogStyleSheet(kMathMLStyleSheetURI);

    
    
    
    
    
    
    nsPresShellIterator iter(aDocument);
    nsCOMPtr<nsIPresShell> shell;
    while ((shell = iter.GetNextShell()) != nsnull) {
      shell->GetPresContext()->PostRebuildAllStyleDataEvent(nsChangeHint(0));
    }
  }

  return rv;
}

PRBool
nsMathMLElement::ParseAttribute(PRInt32 aNamespaceID,
                                nsIAtom* aAttribute,
                                const nsAString& aValue,
                                nsAttrValue& aResult)
{
  if (aNamespaceID == kNameSpaceID_None) {
    if (aAttribute == nsGkAtoms::color ||
        aAttribute == nsGkAtoms::mathcolor_ ||
        aAttribute == nsGkAtoms::background ||
        aAttribute == nsGkAtoms::mathbackground_) {
      return aResult.ParseColor(aValue, GetOwnerDoc());
    }
  }

  return nsMathMLElementBase::ParseAttribute(aNamespaceID, aAttribute,
                                             aValue, aResult);
}

static nsGenericElement::MappedAttributeEntry sTokenStyles[] = {
  { &nsGkAtoms::mathsize_ },
  { &nsGkAtoms::fontsize_ },
  { &nsGkAtoms::mathcolor_ },
  { &nsGkAtoms::color },
  { &nsGkAtoms::mathbackground_ },
  { &nsGkAtoms::fontfamily_ },
  { nsnull }
};

static nsGenericElement::MappedAttributeEntry sEnvironmentStyles[] = {
  { &nsGkAtoms::scriptlevel_ },
  { &nsGkAtoms::scriptminsize_ },
  { &nsGkAtoms::scriptsizemultiplier_ },
  { &nsGkAtoms::background },
  { nsnull }
};

PRBool
nsMathMLElement::IsAttributeMapped(const nsIAtom* aAttribute) const
{
  static const MappedAttributeEntry* const tokenMap[] = {
    sTokenStyles
  };
  static const MappedAttributeEntry* const mstyleMap[] = {
    sTokenStyles,
    sEnvironmentStyles
  };
  
  
  nsIAtom* tag = Tag();
  if (tag == nsGkAtoms::ms_ || tag == nsGkAtoms::mi_ ||
      tag == nsGkAtoms::mn_ || tag == nsGkAtoms::mo_ ||
      tag == nsGkAtoms::mtext_)
    return FindAttributeDependence(aAttribute, tokenMap,
                                   NS_ARRAY_LENGTH(tokenMap));
  if (tag == nsGkAtoms::mstyle_)
    return FindAttributeDependence(aAttribute, mstyleMap,
                                   NS_ARRAY_LENGTH(mstyleMap));
  return PR_FALSE;
}

nsMapRuleToAttributesFunc
nsMathMLElement::GetAttributeMappingFunction() const
{
  
  
  
  return &MapMathMLAttributesInto;
}



























 PRBool
nsMathMLElement::ParseNumericValue(const nsString& aString,
                                   nsCSSValue&     aCSSValue,
                                   PRUint32        aFlags)
{
  nsAutoString str(aString);
  str.CompressWhitespace(); 

  PRInt32 stringLength = str.Length();
  if (!stringLength)
    return PR_FALSE;

  nsAutoString number, unit;

  
  PRInt32 i = 0;
  PRUnichar c = str[0];
  if (c == '-') {
    number.Append(c);
    i++;

    
    if (i < stringLength && nsCRT::IsAsciiSpace(str[i]))
      i++;
  }

  
  PRBool gotDot = PR_FALSE;
  for ( ; i < stringLength; i++) {
    c = str[i];
    if (gotDot && c == '.')
      return PR_FALSE;  
    else if (c == '.')
      gotDot = PR_TRUE;
    else if (!nsCRT::IsAsciiDigit(c)) {
      str.Right(unit, stringLength - i);
      
      
      break;
    }
    number.Append(c);
  }

  
  PRInt32 errorCode;
  float floatValue = number.ToFloat(&errorCode);
  if (NS_FAILED(errorCode))
    return PR_FALSE;
  if (floatValue < 0 && !(aFlags & PARSE_ALLOW_NEGATIVE))
    return PR_FALSE;

  nsCSSUnit cssUnit;
  if (unit.IsEmpty()) {
    if (aFlags & PARSE_ALLOW_UNITLESS) {
      
      cssUnit = eCSSUnit_Number;
    } else {
      
      
      
      if (floatValue != 0.0)
        return PR_FALSE;
      cssUnit = eCSSUnit_Pixel;
    }
  }
  else if (unit.EqualsLiteral("%")) {
    aCSSValue.SetPercentValue(floatValue / 100.0f);
    return PR_TRUE;
  }
  else if (unit.EqualsLiteral("em")) cssUnit = eCSSUnit_EM;
  else if (unit.EqualsLiteral("ex")) cssUnit = eCSSUnit_XHeight;
  else if (unit.EqualsLiteral("px")) cssUnit = eCSSUnit_Pixel;
  else if (unit.EqualsLiteral("in")) cssUnit = eCSSUnit_Inch;
  else if (unit.EqualsLiteral("cm")) cssUnit = eCSSUnit_Centimeter;
  else if (unit.EqualsLiteral("mm")) cssUnit = eCSSUnit_Millimeter;
  else if (unit.EqualsLiteral("pt")) cssUnit = eCSSUnit_Point;
  else if (unit.EqualsLiteral("pc")) cssUnit = eCSSUnit_Pica;
  else 
    return PR_FALSE;

  aCSSValue.SetFloatValue(floatValue, cssUnit);
  return PR_TRUE;
}

void
nsMathMLElement::MapMathMLAttributesInto(const nsMappedAttributes* aAttributes,
                                         nsRuleData* aData)
{
  if (aData->mSIDs & NS_STYLE_INHERIT_BIT(Font)) {
    const nsAttrValue* value =
      aAttributes->GetAttr(nsGkAtoms::scriptsizemultiplier_);
    if (value && value->Type() == nsAttrValue::eString &&
        aData->mFontData->mScriptSizeMultiplier.GetUnit() == eCSSUnit_Null) {
      nsAutoString str(value->GetStringValue());
      str.CompressWhitespace();
      
      if (str.Length() > 0 && str.CharAt(0) != '+') {
        PRInt32 errorCode;
        float floatValue = str.ToFloat(&errorCode);
        
        if (NS_SUCCEEDED(errorCode) && floatValue >= 0.0f) {
          aData->mFontData->mScriptSizeMultiplier.
            SetFloatValue(floatValue, eCSSUnit_Number);
        }
      }
    }

    value = aAttributes->GetAttr(nsGkAtoms::scriptminsize_);
    if (value && value->Type() == nsAttrValue::eString &&
        aData->mFontData->mScriptMinSize.GetUnit() == eCSSUnit_Null) {
      ParseNumericValue(value->GetStringValue(),
                        aData->mFontData->mScriptMinSize, 0);
    }

    value = aAttributes->GetAttr(nsGkAtoms::scriptlevel_);
    if (value && value->Type() == nsAttrValue::eString &&
        aData->mFontData->mScriptLevel.GetUnit() == eCSSUnit_Null) {
      nsAutoString str(value->GetStringValue());
      str.CompressWhitespace();
      if (str.Length() > 0) {
        PRInt32 errorCode;
        PRInt32 intValue = str.ToInteger(&errorCode);
        if (NS_SUCCEEDED(errorCode)) {
          
          
          
          
          PRUnichar ch = str.CharAt(0);
          if (ch == '+' || ch == '-') {
            aData->mFontData->mScriptLevel.SetIntValue(intValue, eCSSUnit_Integer);
          } else {
            aData->mFontData->mScriptLevel.SetFloatValue(intValue, eCSSUnit_Number);
          }
        }
      }
    }

    PRBool parseSizeKeywords = PR_TRUE;
    value = aAttributes->GetAttr(nsGkAtoms::mathsize_);
    if (!value) {
      parseSizeKeywords = PR_FALSE;
      value = aAttributes->GetAttr(nsGkAtoms::fontsize_);
    }
    if (value && value->Type() == nsAttrValue::eString &&
        aData->mFontData->mSize.GetUnit() == eCSSUnit_Null) {
      nsAutoString str(value->GetStringValue());
      if (!ParseNumericValue(str, aData->mFontData->mSize, 0) &&
          parseSizeKeywords) {
        static const char sizes[3][7] = { "small", "normal", "big" };
        static const PRInt32 values[NS_ARRAY_LENGTH(sizes)] = {
          NS_STYLE_FONT_SIZE_SMALL, NS_STYLE_FONT_SIZE_MEDIUM,
          NS_STYLE_FONT_SIZE_LARGE
        };
        str.CompressWhitespace();
        for (PRUint32 i = 0; i < NS_ARRAY_LENGTH(sizes); ++i) {
          if (str.EqualsASCII(sizes[i])) {
            aData->mFontData->mSize.SetIntValue(values[i], eCSSUnit_Enumerated);
            break;
          }
        }
      }
    }

    value = aAttributes->GetAttr(nsGkAtoms::fontfamily_);
    if (value && value->Type() == nsAttrValue::eString &&
        aData->mFontData->mFamily.GetUnit() == eCSSUnit_Null) {
      aData->mFontData->mFamily.SetStringValue(value->GetStringValue(),
                                               eCSSUnit_Families);
      aData->mFontData->mFamilyFromHTML = PR_FALSE;
    }
  }

  if (aData->mSIDs & NS_STYLE_INHERIT_BIT(Background)) {
    const nsAttrValue* value =
      aAttributes->GetAttr(nsGkAtoms::mathbackground_);
    if (!value) {
      value = aAttributes->GetAttr(nsGkAtoms::background);
    }
    if (value && aData->mColorData->mBackColor.GetUnit() == eCSSUnit_Null) {
      nscolor color;
      if (value->GetColorValue(color)) {
        aData->mColorData->mBackColor.SetColorValue(color);
      }
    }
  }

  if (aData->mSIDs & NS_STYLE_INHERIT_BIT(Color)) {
    const nsAttrValue* value = aAttributes->GetAttr(nsGkAtoms::mathcolor_);
    if (!value) {
      value = aAttributes->GetAttr(nsGkAtoms::color);
    }
    nscolor color;
    if (value && value->GetColorValue(color) &&
        aData->mColorData->mColor.GetUnit() == eCSSUnit_Null) {
      aData->mColorData->mColor.SetColorValue(color);
    }
  }
}

NS_IMPL_ELEMENT_CLONE(nsMathMLElement)

PRInt32
nsMathMLElement::IntrinsicState() const
{
  return nsMathMLElementBase::IntrinsicState() |
    (mIncrementScriptLevel ? NS_EVENT_STATE_INCREMENT_SCRIPT_LEVEL : 0);
}

PRBool
nsMathMLElement::IsNodeOfType(PRUint32 aFlags) const
{
  return !(aFlags & ~(eCONTENT | eELEMENT | eMATHML));
}

void
nsMathMLElement::SetIncrementScriptLevel(PRBool aIncrementScriptLevel,
                                         PRBool aNotify)
{
  if (aIncrementScriptLevel == mIncrementScriptLevel)
    return;
  mIncrementScriptLevel = aIncrementScriptLevel;

  NS_ASSERTION(aNotify, "We always notify!");

  nsIDocument* doc = GetCurrentDoc();
  if (!doc)
    return;

  mozAutoDocUpdate upd(doc, UPDATE_CONTENT_STATE, PR_TRUE);
  doc->ContentStatesChanged(this, nsnull,
                            NS_EVENT_STATE_INCREMENT_SCRIPT_LEVEL);
}
