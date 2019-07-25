







































#include "nsStyledElement.h"
#include "nsGkAtoms.h"
#include "nsAttrValue.h"
#include "nsGenericElement.h"
#include "nsMutationEvent.h"
#include "nsDOMCSSDeclaration.h"
#include "nsDOMCSSAttrDeclaration.h"
#include "nsServiceManagerUtils.h"
#include "nsIDocument.h"
#include "nsICSSStyleRule.h"
#include "nsCSSParser.h"
#include "mozilla/css/Loader.h"
#include "nsIDOMMutationEvent.h"

#ifdef MOZ_SVG
#include "nsIDOMSVGStylable.h"
#endif




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
  NS_ASSERTION(HasFlag(NODE_HAS_ID), "Unexpected call");

  
  
  

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
        UnsetFlags(NODE_HAS_ID);
        return PR_FALSE;
      }
      aResult.ParseAtom(aValue);
      SetFlags(NODE_HAS_ID);
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
  PRBool isId = PR_FALSE;
  if (aAttribute == nsGkAtoms::id && aNameSpaceID == kNameSpaceID_None) {
    
    RemoveFromIdTable();
    isId = PR_TRUE;
  }
  
  nsresult rv = nsGenericElement::UnsetAttr(aNameSpaceID, aAttribute, aNotify);

  if (isId) {
    UnsetFlags(NODE_HAS_ID);
  }

  return rv;
}

NS_IMETHODIMP
nsStyledElement::SetInlineStyleRule(nsICSSStyleRule* aStyleRule, PRBool aNotify)
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

  nsAttrValue attrValue(aStyleRule);

  
  PRUint8 modType = modification ?
    static_cast<PRUint8>(nsIDOMMutationEvent::MODIFICATION) :
    static_cast<PRUint8>(nsIDOMMutationEvent::ADDITION);

  return SetAttrAndNotify(kNameSpaceID_None, nsGkAtoms::style, nsnull,
                          oldValueStr, attrValue, modType, hasListeners,
                          aNotify, nsnull);
}

nsICSSStyleRule*
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

  if (aDocument && HasFlag(NODE_HAS_ID) && !GetBindingParent()) {
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





nsresult
nsStyledElement::GetStyle(nsIDOMCSSStyleDeclaration** aStyle)
{
  nsGenericElement::nsDOMSlots *slots = GetDOMSlots();
  NS_ENSURE_TRUE(slots, NS_ERROR_OUT_OF_MEMORY);

  if (!slots->mStyle) {
    
    ReparseStyleAttribute(PR_TRUE);

    slots->mStyle = new nsDOMCSSAttributeDeclaration(this
#ifdef MOZ_SMIL
                                                     , PR_FALSE
#endif 
                                                     );
    NS_ENSURE_TRUE(slots->mStyle, NS_ERROR_OUT_OF_MEMORY);
    SetFlags(NODE_MAY_HAVE_STYLE);
  }

  
  NS_ADDREF(*aStyle = slots->mStyle);
  return NS_OK;
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
      mozilla::css::Loader* cssLoader = doc->CSSLoader();
      nsCSSParser cssParser(cssLoader);
      if (cssParser) {
        nsCOMPtr<nsIURI> baseURI = GetBaseURI();

        nsCOMPtr<nsICSSStyleRule> rule;
        cssParser.ParseStyleAttribute(aValue, doc->GetDocumentURI(),
                                      baseURI,
                                      NodePrincipal(),
                                      getter_AddRefs(rule));
        if (rule) {
          aResult.SetTo(rule);
          return;
        }
      }
    }
  }

  aResult.SetTo(aValue);
}
