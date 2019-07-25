







































#include "nsStyledElement.h"
#include "nsGkAtoms.h"
#include "nsAttrValue.h"
#include "nsGenericElement.h"
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

#ifdef MOZ_SVG
#include "nsIDOMSVGStylable.h"
#endif

namespace css = mozilla::css;




nsIAtom*
nsStyledElement::GetClassAttributeName() const
{
  return nsGkAtoms::_class;
}

nsIAtom*
nsStyledElement::GetIDAttributeName() const
{
  return nsGkAtoms::id;
}

nsIAtom*
nsStyledElement::DoGetID() const
{
  NS_ASSERTION(HasID(), "Unexpected call");

  
  
  

  const nsAttrValue* attr = mAttrsAndChildren.GetAttr(nsGkAtoms::id);

  return attr ? attr->GetAtomValue() : nsnull;
}

const nsAttrValue*
nsStyledElement::DoGetClasses() const
{
  NS_ASSERTION(HasFlag(NODE_MAY_HAVE_CLASS), "Unexpected call");
  return mAttrsAndChildren.GetAttr(nsGkAtoms::_class);
}

PRBool
nsStyledElement::ParseAttribute(PRInt32 aNamespaceID, nsIAtom* aAttribute,
                                const nsAString& aValue, nsAttrValue& aResult)
{
  if (aNamespaceID == kNameSpaceID_None) {
    if (aAttribute == nsGkAtoms::style) {
      SetFlags(NODE_MAY_HAVE_STYLE);
      ParseStyleAttribute(aValue, aResult, PR_FALSE);
      return PR_TRUE;
    }
    if (aAttribute == nsGkAtoms::_class) {
      SetFlags(NODE_MAY_HAVE_CLASS);
      aResult.ParseAtomArray(aValue);
      return PR_TRUE;
    }
    if (aAttribute == nsGkAtoms::id) {
      
      
      RemoveFromIdTable();
      if (aValue.IsEmpty()) {
        ClearHasID();
        return PR_FALSE;
      }
      aResult.ParseAtom(aValue);
      SetHasID();
      AddToIdTable(aResult.GetAtomValue());
      return PR_TRUE;
    }
  }

  return nsStyledElementBase::ParseAttribute(aNamespaceID, aAttribute, aValue,
                                             aResult);
}

nsresult
nsStyledElement::UnsetAttr(PRInt32 aNameSpaceID, nsIAtom* aAttribute,
                           PRBool aNotify)
{
  nsAutoRemovableScriptBlocker scriptBlocker;
  if (aAttribute == nsGkAtoms::id && aNameSpaceID == kNameSpaceID_None) {
    
    RemoveFromIdTable();
  }

  return nsGenericElement::UnsetAttr(aNameSpaceID, aAttribute, aNotify);
}

nsresult
nsStyledElement::AfterSetAttr(PRInt32 aNamespaceID, nsIAtom* aAttribute,
                              const nsAString* aValue, PRBool aNotify)
{
  if (aNamespaceID == kNameSpaceID_None && !aValue &&
      aAttribute == nsGkAtoms::id) {
    
    
    
    ClearHasID();
  }

  return nsGenericElement::AfterSetAttr(aNamespaceID, aAttribute, aValue,
                                        aNotify);
}

NS_IMETHODIMP
nsStyledElement::SetInlineStyleRule(css::StyleRule* aStyleRule, PRBool aNotify)
{
  SetFlags(NODE_MAY_HAVE_STYLE);
  PRBool modification = PR_FALSE;
  nsAutoString oldValueStr;

  PRBool hasListeners = aNotify &&
    nsContentUtils::HasMutationListeners(this,
                                         NS_EVENT_BITS_MUTATION_ATTRMODIFIED,
                                         this);

  
  
  
  
  if (hasListeners) {
    
    
    
    modification = GetAttr(kNameSpaceID_None, nsGkAtoms::style,
                           oldValueStr);
  }
  else if (aNotify && IsInDoc()) {
    modification = !!mAttrsAndChildren.GetAttr(nsGkAtoms::style);
  }

  nsAttrValue attrValue(aStyleRule, nsnull);

  
  PRUint8 modType = modification ?
    static_cast<PRUint8>(nsIDOMMutationEvent::MODIFICATION) :
    static_cast<PRUint8>(nsIDOMMutationEvent::ADDITION);

  return SetAttrAndNotify(kNameSpaceID_None, nsGkAtoms::style, nsnull,
                          oldValueStr, attrValue, modType, hasListeners,
                          aNotify, nsnull);
}

css::StyleRule*
nsStyledElement::GetInlineStyleRule()
{
  if (!HasFlag(NODE_MAY_HAVE_STYLE)) {
    return nsnull;
  }
  const nsAttrValue* attrVal = mAttrsAndChildren.GetAttr(nsGkAtoms::style);

  if (attrVal && attrVal->Type() == nsAttrValue::eCSSStyleRule) {
    return attrVal->GetCSSStyleRuleValue();
  }

  return nsnull;
}

nsresult
nsStyledElement::BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                            nsIContent* aBindingParent,
                            PRBool aCompileEventHandlers)
{
  nsresult rv = nsStyledElementBase::BindToTree(aDocument, aParent,
                                                aBindingParent,
                                                aCompileEventHandlers);
  NS_ENSURE_SUCCESS(rv, rv);

  if (aDocument && HasID() && !GetBindingParent()) {
    aDocument->AddToIdTable(this, DoGetID());
  }

  if (!IsXUL()) {
    
    
    ReparseStyleAttribute(PR_FALSE);
  }

  return NS_OK;
}

void
nsStyledElement::UnbindFromTree(PRBool aDeep, PRBool aNullParent)
{
  RemoveFromIdTable();

  nsStyledElementBase::UnbindFromTree(aDeep, aNullParent);
}





nsIDOMCSSStyleDeclaration*
nsStyledElement::GetStyle(nsresult* retval)
{
  nsXULElement* xulElement = nsXULElement::FromContent(this);
  if (xulElement) {
    nsresult rv = xulElement->EnsureLocalStyle();
    if (NS_FAILED(rv)) {
      *retval = rv;
      return nsnull;
    }
  }
    
  nsGenericElement::nsDOMSlots *slots = DOMSlots();

  if (!slots->mStyle) {
    
    ReparseStyleAttribute(PR_TRUE);

    slots->mStyle = new nsDOMCSSAttributeDeclaration(this
#ifdef MOZ_SMIL
                                                     , PR_FALSE
#endif 
                                                     );
    SetFlags(NODE_MAY_HAVE_STYLE);
  }

  *retval = NS_OK;
  return slots->mStyle;
}

nsresult
nsStyledElement::ReparseStyleAttribute(PRBool aForceInDataDoc)
{
  if (!HasFlag(NODE_MAY_HAVE_STYLE)) {
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
nsStyledElement::ParseStyleAttribute(const nsAString& aValue,
                                     nsAttrValue& aResult,
                                     PRBool aForceInDataDoc)
{
  nsIDocument* doc = GetOwnerDoc();

  if (doc && (aForceInDataDoc ||
              !doc->IsLoadedAsData() ||
              doc->IsStaticDocument())) {
    PRBool isCSS = PR_TRUE; 

    if (!IsInNativeAnonymousSubtree()) {  
                                          
      nsAutoString styleType;
      doc->GetHeaderData(nsGkAtoms::headerContentStyleType, styleType);
      if (!styleType.IsEmpty()) {
        static const char textCssStr[] = "text/css";
        isCSS = (styleType.EqualsIgnoreCase(textCssStr, sizeof(textCssStr) - 1));
      }
    }

    if (isCSS) {
      css::Loader* cssLoader = doc->CSSLoader();
      nsCSSParser cssParser(cssLoader);
      if (cssParser) {
        nsCOMPtr<nsIURI> baseURI = GetBaseURI();

        nsRefPtr<css::StyleRule> rule;
        cssParser.ParseStyleAttribute(aValue, doc->GetDocumentURI(),
                                      baseURI,
                                      NodePrincipal(),
                                      getter_AddRefs(rule));
        if (rule) {
          aResult.SetTo(rule, &aValue);
          return;
        }
      }
    }
  }

  aResult.SetTo(aValue);
}
