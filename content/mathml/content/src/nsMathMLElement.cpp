




#include "mozilla/Util.h"

#include "nsMathMLElement.h"
#include "nsGkAtoms.h"
#include "nsCRT.h"
#include "nsRuleData.h"
#include "nsCSSValue.h"
#include "nsMappedAttributes.h"
#include "nsStyleConsts.h"
#include "nsIDocument.h"
#include "nsEventStates.h"
#include "nsIPresShell.h"
#include "nsPresContext.h"
#include "mozAutoDocUpdate.h"
#include "nsIScriptError.h"
#include "nsContentUtils.h"

#include "mozilla/dom/ElementBinding.h"

using namespace mozilla;
using namespace mozilla::dom;




NS_INTERFACE_TABLE_HEAD(nsMathMLElement)
  NS_NODE_OFFSET_AND_INTERFACE_TABLE_BEGIN(nsMathMLElement)
    NS_INTERFACE_TABLE_ENTRY(nsMathMLElement, nsIDOMNode)
    NS_INTERFACE_TABLE_ENTRY(nsMathMLElement, nsIDOMElement)
    NS_INTERFACE_TABLE_ENTRY(nsMathMLElement, nsILink)
    NS_INTERFACE_TABLE_ENTRY(nsMathMLElement, Link)
  NS_OFFSET_AND_INTERFACE_TABLE_END
  NS_ELEMENT_INTERFACE_TABLE_TO_MAP_SEGUE
NS_ELEMENT_INTERFACE_MAP_END

NS_IMPL_ADDREF_INHERITED(nsMathMLElement, nsMathMLElementBase)
NS_IMPL_RELEASE_INHERITED(nsMathMLElement, nsMathMLElementBase)

static nsresult 
WarnDeprecated(const PRUnichar* aDeprecatedAttribute, 
               const PRUnichar* aFavoredAttribute, nsIDocument* aDocument)
{
  const PRUnichar *argv[] = 
    { aDeprecatedAttribute, aFavoredAttribute };
  return nsContentUtils::
          ReportToConsole(nsIScriptError::warningFlag, "MathML", aDocument,
                          nsContentUtils::eMATHML_PROPERTIES,
                          "DeprecatedSupersededBy", argv, 2);
}

static nsresult 
ReportLengthParseError(const nsString& aValue, nsIDocument* aDocument)
{
  const PRUnichar *arg = aValue.get();
  return nsContentUtils::
         ReportToConsole(nsIScriptError::errorFlag, "MathML", aDocument,
                         nsContentUtils::eMATHML_PROPERTIES,
                         "LengthParsingError", &arg, 1);
}

static nsresult
ReportParseErrorNoTag(const nsString& aValue, 
                      nsIAtom*        aAtom,
                      nsIDocument*    aDocument)
{
  const PRUnichar *argv[] = 
    { aValue.get(), aAtom->GetUTF16String() };
  return nsContentUtils::
         ReportToConsole(nsIScriptError::errorFlag, "MathML", aDocument,
                         nsContentUtils::eMATHML_PROPERTIES,
                         "AttributeParsingErrorNoTag", argv, 2);
}

nsresult
nsMathMLElement::BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                            nsIContent* aBindingParent,
                            bool aCompileEventHandlers)
{
  static const char kMathMLStyleSheetURI[] = "resource://gre-resources/mathml.css";

  Link::ResetLinkState(false, Link::ElementHasHref());

  nsresult rv = nsMathMLElementBase::BindToTree(aDocument, aParent,
                                                aBindingParent,
                                                aCompileEventHandlers);
  NS_ENSURE_SUCCESS(rv, rv);

  if (aDocument) {
    aDocument->RegisterPendingLinkUpdate(this);
    
    if (!aDocument->GetMathMLEnabled()) {
      
      
      
      aDocument->SetMathMLEnabled();
      aDocument->EnsureCatalogStyleSheet(kMathMLStyleSheetURI);

      
      
      
      nsCOMPtr<nsIPresShell> shell = aDocument->GetShell();
      if (shell) {
        shell->GetPresContext()->PostRebuildAllStyleDataEvent(nsChangeHint(0));
      }
    }
  }

  return rv;
}

void
nsMathMLElement::UnbindFromTree(bool aDeep, bool aNullParent)
{
  
  
  Link::ResetLinkState(false, Link::ElementHasHref());
  
  nsIDocument* doc = GetCurrentDoc();
  if (doc) {
    doc->UnregisterPendingLinkUpdate(this);
  }

  nsMathMLElementBase::UnbindFromTree(aDeep, aNullParent);
}

bool
nsMathMLElement::ParseAttribute(int32_t aNamespaceID,
                                nsIAtom* aAttribute,
                                const nsAString& aValue,
                                nsAttrValue& aResult)
{
  if (aNamespaceID == kNameSpaceID_None) {
    if (Tag() == nsGkAtoms::math && aAttribute == nsGkAtoms::mode) {
      WarnDeprecated(nsGkAtoms::mode->GetUTF16String(),
                     nsGkAtoms::display->GetUTF16String(), OwnerDoc());
    }
    if (aAttribute == nsGkAtoms::color) {
      WarnDeprecated(nsGkAtoms::color->GetUTF16String(),
                     nsGkAtoms::mathcolor_->GetUTF16String(), OwnerDoc());
    }
    if (aAttribute == nsGkAtoms::color ||
        aAttribute == nsGkAtoms::mathcolor_ ||
        aAttribute == nsGkAtoms::background ||
        aAttribute == nsGkAtoms::mathbackground_) {
      return aResult.ParseColor(aValue);
    }
  }

  return nsMathMLElementBase::ParseAttribute(aNamespaceID, aAttribute,
                                             aValue, aResult);
}

static Element::MappedAttributeEntry sMtableStyles[] = {
  { &nsGkAtoms::width },
  { nullptr }
};

static Element::MappedAttributeEntry sTokenStyles[] = {
  { &nsGkAtoms::mathsize_ },
  { &nsGkAtoms::fontsize_ },
  { &nsGkAtoms::color },
  { &nsGkAtoms::fontfamily_ },
  { nullptr }
};

static Element::MappedAttributeEntry sEnvironmentStyles[] = {
  { &nsGkAtoms::scriptlevel_ },
  { &nsGkAtoms::scriptminsize_ },
  { &nsGkAtoms::scriptsizemultiplier_ },
  { &nsGkAtoms::background },
  { nullptr }
};

static Element::MappedAttributeEntry sCommonPresStyles[] = {
  { &nsGkAtoms::mathcolor_ },
  { &nsGkAtoms::mathbackground_ },
  { nullptr }
};

bool
nsMathMLElement::IsAttributeMapped(const nsIAtom* aAttribute) const
{
  static const MappedAttributeEntry* const mtableMap[] = {
    sMtableStyles,
    sCommonPresStyles
  };
  static const MappedAttributeEntry* const tokenMap[] = {
    sTokenStyles,
    sCommonPresStyles
  };
  static const MappedAttributeEntry* const mstyleMap[] = {
    sTokenStyles,
    sEnvironmentStyles,
    sCommonPresStyles
  };
  static const MappedAttributeEntry* const commonPresMap[] = {
    sCommonPresStyles
  };
  
  
  nsIAtom* tag = Tag();
  if (tag == nsGkAtoms::ms_ || tag == nsGkAtoms::mi_ ||
      tag == nsGkAtoms::mn_ || tag == nsGkAtoms::mo_ ||
      tag == nsGkAtoms::mtext_ || tag == nsGkAtoms::mspace_)
    return FindAttributeDependence(aAttribute, tokenMap);
  if (tag == nsGkAtoms::mstyle_ ||
      tag == nsGkAtoms::math)
    return FindAttributeDependence(aAttribute, mstyleMap);

  if (tag == nsGkAtoms::mtable_)
    return FindAttributeDependence(aAttribute, mtableMap);

  if (tag == nsGkAtoms::maction_ ||
      tag == nsGkAtoms::maligngroup_ ||
      tag == nsGkAtoms::malignmark_ ||
      tag == nsGkAtoms::menclose_ ||
      tag == nsGkAtoms::merror_ ||
      tag == nsGkAtoms::mfenced_ ||
      tag == nsGkAtoms::mfrac_ ||
      tag == nsGkAtoms::mover_ ||
      tag == nsGkAtoms::mpadded_ ||
      tag == nsGkAtoms::mphantom_ ||
      tag == nsGkAtoms::mprescripts_ ||
      tag == nsGkAtoms::mroot_ ||
      tag == nsGkAtoms::mrow_ ||
      tag == nsGkAtoms::msqrt_ ||
      tag == nsGkAtoms::msub_ ||
      tag == nsGkAtoms::msubsup_ ||
      tag == nsGkAtoms::msup_ ||
      tag == nsGkAtoms::mtd_ ||
      tag == nsGkAtoms::mtr_ ||
      tag == nsGkAtoms::munder_ ||
      tag == nsGkAtoms::munderover_ ||
      tag == nsGkAtoms::none) {
    return FindAttributeDependence(aAttribute, commonPresMap);
  }

  return false;
}

nsMapRuleToAttributesFunc
nsMathMLElement::GetAttributeMappingFunction() const
{
  
  
  
  return &MapMathMLAttributesInto;
}

 bool
nsMathMLElement::ParseNamedSpaceValue(const nsString& aString,
                                      nsCSSValue&     aCSSValue,
                                      uint32_t        aFlags)
{
   int32_t i = 0;
   
   if (aString.EqualsLiteral("veryverythinmathspace")) {
     i = 1;
   } else if (aString.EqualsLiteral("verythinmathspace")) {
     i = 2;
   } else if (aString.EqualsLiteral("thinmathspace")) {
     i = 3;
   } else if (aString.EqualsLiteral("mediummathspace")) {
     i = 4;
   } else if (aString.EqualsLiteral("thickmathspace")) {
     i = 5;
   } else if (aString.EqualsLiteral("verythickmathspace")) {
     i = 6;
   } else if (aString.EqualsLiteral("veryverythickmathspace")) {
     i = 7;
   } else if (aFlags & PARSE_ALLOW_NEGATIVE) {
     if (aString.EqualsLiteral("negativeveryverythinmathspace")) {
       i = -1;
     } else if (aString.EqualsLiteral("negativeverythinmathspace")) {
       i = -2;
     } else if (aString.EqualsLiteral("negativethinmathspace")) {
       i = -3;
     } else if (aString.EqualsLiteral("negativemediummathspace")) {
       i = -4;
     } else if (aString.EqualsLiteral("negativethickmathspace")) {
       i = -5;
     } else if (aString.EqualsLiteral("negativeverythickmathspace")) {
       i = -6;
     } else if (aString.EqualsLiteral("negativeveryverythickmathspace")) {
       i = -7;
     }
   }
   if (0 != i) { 
     aCSSValue.SetFloatValue(float(i)/float(18), eCSSUnit_EM);
     return true;
   }
   
   return false;
}
 




































 bool
nsMathMLElement::ParseNumericValue(const nsString& aString,
                                   nsCSSValue&     aCSSValue,
                                   uint32_t        aFlags,
                                   nsIDocument*    aDocument)
{
  nsAutoString str(aString);
  str.CompressWhitespace(); 

  int32_t stringLength = str.Length();
  if (!stringLength) {
    if (!(aFlags & PARSE_SUPPRESS_WARNINGS)) {
      ReportLengthParseError(aString, aDocument);
    }
    return false;
  }

  if (ParseNamedSpaceValue(aString, aCSSValue, aFlags)) {
    return true;
  }

  nsAutoString number, unit;

  
  int32_t i = 0;
  PRUnichar c = str[0];
  if (c == '-') {
    number.Append(c);
    i++;
  }

  
  bool gotDot = false;
  for ( ; i < stringLength; i++) {
    c = str[i];
    if (gotDot && c == '.') {
      if (!(aFlags & PARSE_SUPPRESS_WARNINGS)) {
        ReportLengthParseError(aString, aDocument);
      }
      return false;  
    }
    else if (c == '.')
      gotDot = true;
    else if (!nsCRT::IsAsciiDigit(c)) {
      str.Right(unit, stringLength - i);
      
      
      break;
    }
    number.Append(c);
  }

  
  nsresult errorCode;
  float floatValue = number.ToFloat(&errorCode);
  if (NS_FAILED(errorCode)) {
    if (!(aFlags & PARSE_SUPPRESS_WARNINGS)) {
      ReportLengthParseError(aString, aDocument);
    }
    return false;
  }
  if (floatValue < 0 && !(aFlags & PARSE_ALLOW_NEGATIVE)) {
    if (!(aFlags & PARSE_SUPPRESS_WARNINGS)) {
      ReportLengthParseError(aString, aDocument);
    }
    return false;
  }

  nsCSSUnit cssUnit;
  if (unit.IsEmpty()) {
    if (aFlags & PARSE_ALLOW_UNITLESS) {
      
      if (!(aFlags & PARSE_SUPPRESS_WARNINGS)) {
        nsContentUtils::ReportToConsole(nsIScriptError::warningFlag,
                                        "MathML", aDocument,
                                        nsContentUtils::eMATHML_PROPERTIES,
                                        "UnitlessValuesAreDeprecated");
      }
      if (aFlags & CONVERT_UNITLESS_TO_PERCENT) {
        aCSSValue.SetPercentValue(floatValue);
        return true;
      }
      else
        cssUnit = eCSSUnit_Number;
    } else {
      
      
      
      if (floatValue != 0.0) {
        if (!(aFlags & PARSE_SUPPRESS_WARNINGS)) {
          ReportLengthParseError(aString, aDocument);
        }
        return false;
      }
      cssUnit = eCSSUnit_Pixel;
    }
  }
  else if (unit.EqualsLiteral("%")) {
    aCSSValue.SetPercentValue(floatValue / 100.0f);
    return true;
  }
  else if (unit.EqualsLiteral("em")) cssUnit = eCSSUnit_EM;
  else if (unit.EqualsLiteral("ex")) cssUnit = eCSSUnit_XHeight;
  else if (unit.EqualsLiteral("px")) cssUnit = eCSSUnit_Pixel;
  else if (unit.EqualsLiteral("in")) cssUnit = eCSSUnit_Inch;
  else if (unit.EqualsLiteral("cm")) cssUnit = eCSSUnit_Centimeter;
  else if (unit.EqualsLiteral("mm")) cssUnit = eCSSUnit_Millimeter;
  else if (unit.EqualsLiteral("pt")) cssUnit = eCSSUnit_Point;
  else if (unit.EqualsLiteral("pc")) cssUnit = eCSSUnit_Pica;
  else { 
    if (!(aFlags & PARSE_SUPPRESS_WARNINGS)) {
      ReportLengthParseError(aString, aDocument);
    }
    return false;
  }

  aCSSValue.SetFloatValue(floatValue, cssUnit);
  return true;
}

void
nsMathMLElement::MapMathMLAttributesInto(const nsMappedAttributes* aAttributes,
                                         nsRuleData* aData)
{
  if (aData->mSIDs & NS_STYLE_INHERIT_BIT(Font)) {
    
    
    
    
    
    
    
    
    const nsAttrValue* value =
      aAttributes->GetAttr(nsGkAtoms::scriptsizemultiplier_);
    nsCSSValue* scriptSizeMultiplier =
      aData->ValueForScriptSizeMultiplier();
    if (value && value->Type() == nsAttrValue::eString &&
        scriptSizeMultiplier->GetUnit() == eCSSUnit_Null) {
      nsAutoString str(value->GetStringValue());
      str.CompressWhitespace();
      
      if (str.Length() > 0 && str.CharAt(0) != '+') {
        nsresult errorCode;
        float floatValue = str.ToFloat(&errorCode);
        
        if (NS_SUCCEEDED(errorCode) && floatValue >= 0.0f) {
          scriptSizeMultiplier->SetFloatValue(floatValue, eCSSUnit_Number);
        } else {
          ReportParseErrorNoTag(str,
                                nsGkAtoms::scriptsizemultiplier_,
                                aData->mPresContext->Document());
        }
      }
    }

    
    
    
    
    
    
    
    
    
    
    
    value = aAttributes->GetAttr(nsGkAtoms::scriptminsize_);
    nsCSSValue* scriptMinSize = aData->ValueForScriptMinSize();
    if (value && value->Type() == nsAttrValue::eString &&
        scriptMinSize->GetUnit() == eCSSUnit_Null) {
      ParseNumericValue(value->GetStringValue(), *scriptMinSize,
                        PARSE_ALLOW_UNITLESS | CONVERT_UNITLESS_TO_PERCENT,
                        aData->mPresContext->Document());

      if (scriptMinSize->GetUnit() == eCSSUnit_Percent) {
        scriptMinSize->SetFloatValue(8.0 * scriptMinSize->GetPercentValue(),
                                     eCSSUnit_Point);
      }
    }

    
    
    
    
    
    
    
    
    
    
    
    value = aAttributes->GetAttr(nsGkAtoms::scriptlevel_);
    nsCSSValue* scriptLevel = aData->ValueForScriptLevel();
    if (value && value->Type() == nsAttrValue::eString &&
        scriptLevel->GetUnit() == eCSSUnit_Null) {
      nsAutoString str(value->GetStringValue());
      str.CompressWhitespace();
      if (str.Length() > 0) {
        nsresult errorCode;
        int32_t intValue = str.ToInteger(&errorCode);
        if (NS_SUCCEEDED(errorCode)) {
          
          
          
          
          PRUnichar ch = str.CharAt(0);
          if (ch == '+' || ch == '-') {
            scriptLevel->SetIntValue(intValue, eCSSUnit_Integer);
          } else {
            scriptLevel->SetFloatValue(intValue, eCSSUnit_Number);
          }
        } else {
          ReportParseErrorNoTag(str,
                                nsGkAtoms::scriptlevel_,
                                aData->mPresContext->Document());
        }
      }
    }

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    bool parseSizeKeywords = true;
    value = aAttributes->GetAttr(nsGkAtoms::mathsize_);
    if (!value) {
      parseSizeKeywords = false;
      value = aAttributes->GetAttr(nsGkAtoms::fontsize_);
      if (value) {
        WarnDeprecated(nsGkAtoms::fontsize_->GetUTF16String(),
                       nsGkAtoms::mathsize_->GetUTF16String(),
                       aData->mPresContext->Document());
      }
    }
    nsCSSValue* fontSize = aData->ValueForFontSize();
    if (value && value->Type() == nsAttrValue::eString &&
        fontSize->GetUnit() == eCSSUnit_Null) {
      nsAutoString str(value->GetStringValue());
      if (!ParseNumericValue(str, *fontSize, PARSE_SUPPRESS_WARNINGS |
                             PARSE_ALLOW_UNITLESS | CONVERT_UNITLESS_TO_PERCENT,
                             nullptr)
          && parseSizeKeywords) {
        static const char sizes[3][7] = { "small", "normal", "big" };
        static const int32_t values[NS_ARRAY_LENGTH(sizes)] = {
          NS_STYLE_FONT_SIZE_SMALL, NS_STYLE_FONT_SIZE_MEDIUM,
          NS_STYLE_FONT_SIZE_LARGE
        };
        str.CompressWhitespace();
        for (uint32_t i = 0; i < ArrayLength(sizes); ++i) {
          if (str.EqualsASCII(sizes[i])) {
            fontSize->SetIntValue(values[i], eCSSUnit_Enumerated);
            break;
          }
        }
      }
    }

    
    
    
    
    
    
    
    
    value = aAttributes->GetAttr(nsGkAtoms::fontfamily_);
    nsCSSValue* fontFamily = aData->ValueForFontFamily();
    if (value) {
      WarnDeprecated(nsGkAtoms::fontfamily_->GetUTF16String(),
                     nsGkAtoms::mathvariant_->GetUTF16String(),
                     aData->mPresContext->Document());
    }
    if (value && value->Type() == nsAttrValue::eString &&
        fontFamily->GetUnit() == eCSSUnit_Null) {
      fontFamily->SetStringValue(value->GetStringValue(), eCSSUnit_Families);
    }
  }

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  if (aData->mSIDs & NS_STYLE_INHERIT_BIT(Background)) {
    const nsAttrValue* value =
      aAttributes->GetAttr(nsGkAtoms::mathbackground_);
    if (!value) {
      value = aAttributes->GetAttr(nsGkAtoms::background);
      if (value) {
        WarnDeprecated(nsGkAtoms::background->GetUTF16String(),
                       nsGkAtoms::mathbackground_->GetUTF16String(),
                       aData->mPresContext->Document());
      }
    }
    nsCSSValue* backgroundColor = aData->ValueForBackgroundColor();
    if (value && backgroundColor->GetUnit() == eCSSUnit_Null) {
      nscolor color;
      if (value->GetColorValue(color)) {
        backgroundColor->SetColorValue(color);
      }
    }
  }

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  if (aData->mSIDs & NS_STYLE_INHERIT_BIT(Color)) {
    const nsAttrValue* value = aAttributes->GetAttr(nsGkAtoms::mathcolor_);
    if (!value) {
      value = aAttributes->GetAttr(nsGkAtoms::color);
      if (value) {
        WarnDeprecated(nsGkAtoms::color->GetUTF16String(),
                       nsGkAtoms::mathcolor_->GetUTF16String(),
                       aData->mPresContext->Document());
      }
    }
    nscolor color;
    nsCSSValue* colorValue = aData->ValueForColor();
    if (value && value->GetColorValue(color) &&
        colorValue->GetUnit() == eCSSUnit_Null) {
      colorValue->SetColorValue(color);
    }
  }

  if (aData->mSIDs & NS_STYLE_INHERIT_BIT(Position)) {
    
    nsCSSValue* width = aData->ValueForWidth();
    if (width->GetUnit() == eCSSUnit_Null) {
      const nsAttrValue* value = aAttributes->GetAttr(nsGkAtoms::width);
      
      if (value && value->Type() == nsAttrValue::eString) {
        ParseNumericValue(value->GetStringValue(), *width, 0,
                          aData->mPresContext->Document());
      }
    }
  }

}

nsresult
nsMathMLElement::PreHandleEvent(nsEventChainPreVisitor& aVisitor)
{
  nsresult rv = Element::PreHandleEvent(aVisitor);
  NS_ENSURE_SUCCESS(rv, rv);

  return PreHandleEventForLinks(aVisitor);
}

nsresult
nsMathMLElement::PostHandleEvent(nsEventChainPostVisitor& aVisitor)
{
  return PostHandleEventForLinks(aVisitor);
}

NS_IMPL_ELEMENT_CLONE(nsMathMLElement)

nsEventStates
nsMathMLElement::IntrinsicState() const
{
  return Link::LinkState() | nsMathMLElementBase::IntrinsicState() |
    (mIncrementScriptLevel ? NS_EVENT_STATE_INCREMENT_SCRIPT_LEVEL : nsEventStates());
}

bool
nsMathMLElement::IsNodeOfType(uint32_t aFlags) const
{
  return !(aFlags & ~eCONTENT);
}

void
nsMathMLElement::SetIncrementScriptLevel(bool aIncrementScriptLevel,
                                         bool aNotify)
{
  if (aIncrementScriptLevel == mIncrementScriptLevel)
    return;
  mIncrementScriptLevel = aIncrementScriptLevel;

  NS_ASSERTION(aNotify, "We always notify!");

  UpdateState(true);
}

bool
nsMathMLElement::IsFocusable(int32_t *aTabIndex, bool aWithMouse)
{
  nsCOMPtr<nsIURI> uri;
  if (IsLink(getter_AddRefs(uri))) {
    if (aTabIndex) {
      *aTabIndex = ((sTabFocusModel & eTabFocus_linksMask) == 0 ? -1 : 0);
    }
    return true;
  }

  if (aTabIndex) {
    *aTabIndex = -1;
  }

  return false;
}

bool
nsMathMLElement::IsLink(nsIURI** aURI) const
{
  
  
  nsIAtom* tag = Tag();
  if (tag == nsGkAtoms::mprescripts_ ||
      tag == nsGkAtoms::none         ||
      tag == nsGkAtoms::malignmark_  ||
      tag == nsGkAtoms::maligngroup_) {
    *aURI = nullptr;
    return false;
  }

  bool hasHref = false;
  const nsAttrValue* href = mAttrsAndChildren.GetAttr(nsGkAtoms::href,
                                                      kNameSpaceID_None);
  if (href) {
    
    
    
    hasHref = true;
  } else {
    
    
    
    
    
    
    
    
    
    
    static nsIContent::AttrValuesArray sTypeVals[] =
      { &nsGkAtoms::_empty, &nsGkAtoms::simple, nullptr };
    
    static nsIContent::AttrValuesArray sShowVals[] =
      { &nsGkAtoms::_empty, &nsGkAtoms::_new, &nsGkAtoms::replace, nullptr };
    
    static nsIContent::AttrValuesArray sActuateVals[] =
      { &nsGkAtoms::_empty, &nsGkAtoms::onRequest, nullptr };
    
    
    href = mAttrsAndChildren.GetAttr(nsGkAtoms::href,
                                     kNameSpaceID_XLink);
    if (href &&
        FindAttrValueIn(kNameSpaceID_XLink, nsGkAtoms::type,
                        sTypeVals, eCaseMatters) !=
        nsIContent::ATTR_VALUE_NO_MATCH &&
        FindAttrValueIn(kNameSpaceID_XLink, nsGkAtoms::show,
                        sShowVals, eCaseMatters) !=
        nsIContent::ATTR_VALUE_NO_MATCH &&
        FindAttrValueIn(kNameSpaceID_XLink, nsGkAtoms::actuate,
                        sActuateVals, eCaseMatters) !=
        nsIContent::ATTR_VALUE_NO_MATCH) {
      hasHref = true;
    }
  }

  if (hasHref) {
    nsCOMPtr<nsIURI> baseURI = GetBaseURI();
    
    nsAutoString hrefStr;
    href->ToString(hrefStr); 
    nsContentUtils::NewURIWithDocumentCharset(aURI, hrefStr,
                                              OwnerDoc(), baseURI);
    
    return !!*aURI;
  }

  *aURI = nullptr;
  return false;
}

void
nsMathMLElement::GetLinkTarget(nsAString& aTarget)
{
  const nsAttrValue* target = mAttrsAndChildren.GetAttr(nsGkAtoms::target,
                                                        kNameSpaceID_XLink);
  if (target) {
    target->ToString(aTarget);
  }

  if (aTarget.IsEmpty()) {

    static nsIContent::AttrValuesArray sShowVals[] =
      { &nsGkAtoms::_new, &nsGkAtoms::replace, nullptr };
    
    switch (FindAttrValueIn(kNameSpaceID_XLink, nsGkAtoms::show,
                            sShowVals, eCaseMatters)) {
    case 0:
      aTarget.AssignLiteral("_blank");
      return;
    case 1:
      return;
    }
    OwnerDoc()->GetBaseTarget(aTarget);
  }
}

nsLinkState
nsMathMLElement::GetLinkState() const
{
  return Link::GetLinkState();
}

already_AddRefed<nsIURI>
nsMathMLElement::GetHrefURI() const
{
  nsCOMPtr<nsIURI> hrefURI;
  return IsLink(getter_AddRefs(hrefURI)) ? hrefURI.forget() : nullptr;
}

nsresult
nsMathMLElement::SetAttr(int32_t aNameSpaceID, nsIAtom* aName,
                         nsIAtom* aPrefix, const nsAString& aValue,
                         bool aNotify)
{
  nsresult rv = nsMathMLElementBase::SetAttr(aNameSpaceID, aName, aPrefix,
                                           aValue, aNotify);

  
  
  
  
  
  if (aName == nsGkAtoms::href &&
      (aNameSpaceID == kNameSpaceID_None ||
       aNameSpaceID == kNameSpaceID_XLink)) {
    if (aNameSpaceID == kNameSpaceID_XLink) {
      WarnDeprecated(NS_LITERAL_STRING("xlink:href").get(),
                     NS_LITERAL_STRING("href").get(), OwnerDoc());
    }
    Link::ResetLinkState(!!aNotify, true);
  }

  return rv;
}

nsresult
nsMathMLElement::UnsetAttr(int32_t aNameSpaceID, nsIAtom* aAttr,
                           bool aNotify)
{
  nsresult rv = nsMathMLElementBase::UnsetAttr(aNameSpaceID, aAttr, aNotify);

  
  
  
  
  
  if (aAttr == nsGkAtoms::href &&
      (aNameSpaceID == kNameSpaceID_None ||
       aNameSpaceID == kNameSpaceID_XLink)) {
    
    
    Link::ResetLinkState(!!aNotify, Link::ElementHasHref());
  }

  return rv;
}

JSObject*
nsMathMLElement::WrapNode(JSContext *aCx, JSObject *aScope, bool *aTriedToWrap)
{
  return ElementBinding::Wrap(aCx, aScope, this, aTriedToWrap);
}
