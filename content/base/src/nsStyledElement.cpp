





#include "nsStyledElement.h"
#include "nsGkAtoms.h"
#include "nsAttrValue.h"
#include "nsAttrValueInlines.h"
#include "mozilla/dom/Element.h"
#include "nsMutationEvent.h"
#include "nsDOMCSSDeclaration.h"
#include "nsDOMCSSAttrDeclaration.h"
#include "nsServiceManagerUtils.h"
#include "nsIDocument.h"
#include "mozilla/css/StyleRule.h"
#include "nsCSSParser.h"
#include "mozilla/css/Loader.h"
#include "nsIDOMMutationEvent.h"
#include "nsXULElement.h"
#include "nsContentUtils.h"

namespace css = mozilla::css;
using namespace mozilla::dom;




nsIAtom*
nsStyledElementNotElementCSSInlineStyle::GetClassAttributeName() const
{
  return nsGkAtoms::_class;
}

nsIAtom*
nsStyledElementNotElementCSSInlineStyle::GetIDAttributeName() const
{
  return nsGkAtoms::id;
}

nsIAtom*
nsStyledElementNotElementCSSInlineStyle::DoGetID() const
{
  NS_ASSERTION(HasID(), "Unexpected call");

  
  
  

  const nsAttrValue* attr = mAttrsAndChildren.GetAttr(nsGkAtoms::id);

  return attr ? attr->GetAtomValue() : nullptr;
}

const nsAttrValue*
nsStyledElementNotElementCSSInlineStyle::DoGetClasses() const
{
  NS_ASSERTION(HasFlag(NODE_MAY_HAVE_CLASS), "Unexpected call");
  return mAttrsAndChildren.GetAttr(nsGkAtoms::_class);
}

bool
nsStyledElementNotElementCSSInlineStyle::ParseAttribute(int32_t aNamespaceID,
                                                        nsIAtom* aAttribute,
                                                        const nsAString& aValue,
                                                        nsAttrValue& aResult)
{
  if (aNamespaceID == kNameSpaceID_None) {
    if (aAttribute == nsGkAtoms::style) {
      SetMayHaveStyle();
      ParseStyleAttribute(aValue, aResult, false);
      return true;
    }
    if (aAttribute == nsGkAtoms::_class) {
      SetFlags(NODE_MAY_HAVE_CLASS);
      aResult.ParseAtomArray(aValue);
      return true;
    }
    if (aAttribute == nsGkAtoms::id) {
      
      
      RemoveFromIdTable();
      if (aValue.IsEmpty()) {
        ClearHasID();
        return false;
      }
      aResult.ParseAtom(aValue);
      SetHasID();
      AddToIdTable(aResult.GetAtomValue());
      return true;
    }
  }

  return nsStyledElementBase::ParseAttribute(aNamespaceID, aAttribute, aValue,
                                             aResult);
}

nsresult
nsStyledElementNotElementCSSInlineStyle::UnsetAttr(int32_t aNameSpaceID,
                                                   nsIAtom* aAttribute,
                                                   bool aNotify)
{
  nsAutoScriptBlocker scriptBlocker;
  if (aAttribute == nsGkAtoms::id && aNameSpaceID == kNameSpaceID_None) {
    
    RemoveFromIdTable();
  }

  return Element::UnsetAttr(aNameSpaceID, aAttribute, aNotify);
}

nsresult
nsStyledElementNotElementCSSInlineStyle::AfterSetAttr(int32_t aNamespaceID,
                                                      nsIAtom* aAttribute,
                                                      const nsAttrValue* aValue,
                                                      bool aNotify)
{
  if (aNamespaceID == kNameSpaceID_None && !aValue &&
      aAttribute == nsGkAtoms::id) {
    
    
    
    ClearHasID();
  }

  return Element::AfterSetAttr(aNamespaceID, aAttribute, aValue, aNotify);
}

nsresult
nsStyledElementNotElementCSSInlineStyle::SetInlineStyleRule(css::StyleRule* aStyleRule,
                                                            const nsAString* aSerialized,
                                                            bool aNotify)
{
  SetMayHaveStyle();
  bool modification = false;
  nsAttrValue oldValue;

  bool hasListeners = aNotify &&
    nsContentUtils::HasMutationListeners(this,
                                         NS_EVENT_BITS_MUTATION_ATTRMODIFIED,
                                         this);

  
  
  
  
  if (hasListeners) {
    
    
    
    nsAutoString oldValueStr;
    modification = GetAttr(kNameSpaceID_None, nsGkAtoms::style,
                           oldValueStr);
    if (modification) {
      oldValue.SetTo(oldValueStr);
    }
  }
  else if (aNotify && IsInDoc()) {
    modification = !!mAttrsAndChildren.GetAttr(nsGkAtoms::style);
  }

  nsAttrValue attrValue(aStyleRule, aSerialized);

  
  uint8_t modType = modification ?
    static_cast<uint8_t>(nsIDOMMutationEvent::MODIFICATION) :
    static_cast<uint8_t>(nsIDOMMutationEvent::ADDITION);

  return SetAttrAndNotify(kNameSpaceID_None, nsGkAtoms::style, nullptr,
                          oldValue, attrValue, modType, hasListeners,
                          aNotify, kDontCallAfterSetAttr);
}

css::StyleRule*
nsStyledElementNotElementCSSInlineStyle::GetInlineStyleRule()
{
  if (!MayHaveStyle()) {
    return nullptr;
  }
  const nsAttrValue* attrVal = mAttrsAndChildren.GetAttr(nsGkAtoms::style);

  if (attrVal && attrVal->Type() == nsAttrValue::eCSSStyleRule) {
    return attrVal->GetCSSStyleRuleValue();
  }

  return nullptr;
}




nsICSSDeclaration*
nsStyledElementNotElementCSSInlineStyle::Style()
{
  Element::nsDOMSlots *slots = DOMSlots();

  if (!slots->mStyle) {
    
    ReparseStyleAttribute(true);

    slots->mStyle = new nsDOMCSSAttributeDeclaration(this, false);
    SetMayHaveStyle();
  }

  return slots->mStyle;
}

nsresult
nsStyledElementNotElementCSSInlineStyle::ReparseStyleAttribute(bool aForceInDataDoc)
{
  if (!MayHaveStyle()) {
    return NS_OK;
  }
  const nsAttrValue* oldVal = mAttrsAndChildren.GetAttr(nsGkAtoms::style);
  
  if (oldVal && oldVal->Type() != nsAttrValue::eCSSStyleRule) {
    nsAttrValue attrValue;
    nsAutoString stringValue;
    oldVal->ToString(stringValue);
    ParseStyleAttribute(stringValue, attrValue, aForceInDataDoc);
    
    
    nsresult rv = mAttrsAndChildren.SetAndTakeAttr(nsGkAtoms::style, attrValue);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  
  return NS_OK;
}

void
nsStyledElementNotElementCSSInlineStyle::ParseStyleAttribute(const nsAString& aValue,
                                                             nsAttrValue& aResult,
                                                             bool aForceInDataDoc)
{
  nsIDocument* doc = OwnerDoc();

  if (aForceInDataDoc ||
      !doc->IsLoadedAsData() ||
      doc->IsStaticDocument()) {
    bool isCSS = true; 

    if (!IsInNativeAnonymousSubtree()) {  
                                          
      nsAutoString styleType;
      doc->GetHeaderData(nsGkAtoms::headerContentStyleType, styleType);
      if (!styleType.IsEmpty()) {
        static const char textCssStr[] = "text/css";
        isCSS = (styleType.EqualsIgnoreCase(textCssStr, sizeof(textCssStr) - 1));
      }
    }

    if (isCSS && aResult.ParseStyleAttribute(aValue, this)) {
      return;
    }
  }

  aResult.SetTo(aValue);
}
