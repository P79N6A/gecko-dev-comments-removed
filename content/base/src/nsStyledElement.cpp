







































#include "nsStyledElement.h"
#include "nsGkAtoms.h"
#include "nsAttrValue.h"
#include "nsGenericElement.h"
#include "nsMutationEvent.h"
#include "nsDOMCSSDeclaration.h"
#include "nsICSSOMFactory.h"
#include "nsServiceManagerUtils.h"
#include "nsIDocument.h"
#include "nsICSSStyleRule.h"
#include "nsICSSParser.h"
#include "nsICSSLoader.h"

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

const nsAttrValue*
nsStyledElement::GetClasses() const
{
  return mAttrsAndChildren.GetAttr(nsGkAtoms::_class);
}

PRBool
nsStyledElement::ParseAttribute(PRInt32 aNamespaceID, nsIAtom* aAttribute,
                                const nsAString& aValue, nsAttrValue& aResult)
{
  if (aNamespaceID == kNameSpaceID_None) {
    if (aAttribute == nsGkAtoms::style) {
      ParseStyleAttribute(this, aValue, aResult);
      return PR_TRUE;
    }
    if (aAttribute == nsGkAtoms::_class) {
#ifdef MOZ_SVG
      NS_ASSERTION(!nsCOMPtr<nsIDOMSVGStylable>(do_QueryInterface(this)),
                   "SVG code should have handled this 'class' attribute!");
#endif
      aResult.ParseAtomArray(aValue);
      return PR_TRUE;
    }
  }

  return nsStyledElementBase::ParseAttribute(aNamespaceID, aAttribute, aValue,
                                             aResult);
}

NS_IMETHODIMP
nsStyledElement::SetInlineStyleRule(nsICSSStyleRule* aStyleRule, PRBool aNotify)
{
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

  return SetAttrAndNotify(kNameSpaceID_None, nsGkAtoms::style, nsnull,
                          oldValueStr, attrValue, modification, hasListeners,
                          aNotify);
}

nsICSSStyleRule*
nsStyledElement::GetInlineStyleRule()
{
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

  
  
  ReparseStyleAttribute();

  return rv;
}




static nsICSSOMFactory* gCSSOMFactory = nsnull;
static NS_DEFINE_CID(kCSSOMFactoryCID, NS_CSSOMFACTORY_CID);

nsresult
nsStyledElement::GetStyle(nsIDOMCSSStyleDeclaration** aStyle)
{
  nsGenericElement::nsDOMSlots *slots = GetDOMSlots();
  NS_ENSURE_TRUE(slots, NS_ERROR_OUT_OF_MEMORY);

  if (!slots->mStyle) {
    
    ReparseStyleAttribute();

    nsresult rv;
    if (!gCSSOMFactory) {
      rv = CallGetService(kCSSOMFactoryCID, &gCSSOMFactory);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    rv = gCSSOMFactory->CreateDOMCSSAttributeDeclaration(this,
                                                 getter_AddRefs(slots->mStyle));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  NS_ADDREF(*aStyle = slots->mStyle);
  return NS_OK;
}

nsresult
nsStyledElement::ReparseStyleAttribute()
{
  const nsAttrValue* oldVal = mAttrsAndChildren.GetAttr(nsGkAtoms::style);
  
  if (oldVal && oldVal->Type() != nsAttrValue::eCSSStyleRule) {
    nsAttrValue attrValue;
    nsAutoString stringValue;
    oldVal->ToString(stringValue);
    ParseStyleAttribute(this, stringValue, attrValue);
    
    
    nsresult rv = mAttrsAndChildren.SetAndTakeAttr(nsGkAtoms::style, attrValue);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  
  return NS_OK;
}

void
nsStyledElement::ParseStyleAttribute(nsIContent* aContent,
                                     const nsAString& aValue,
                                     nsAttrValue& aResult)
{
  nsresult result = NS_OK;
  nsIDocument* doc = aContent->GetOwnerDoc();

  if (doc) {
    PRBool isCSS = PR_TRUE; 

    if (!aContent->IsNativeAnonymous()) {  
                                           
      nsAutoString styleType;
      doc->GetHeaderData(nsGkAtoms::headerContentStyleType, styleType);
      if (!styleType.IsEmpty()) {
        static const char textCssStr[] = "text/css";
        isCSS = (styleType.EqualsIgnoreCase(textCssStr, sizeof(textCssStr) - 1));
      }
    }

    if (isCSS) {
      nsICSSLoader* cssLoader = doc->CSSLoader();
      nsCOMPtr<nsICSSParser> cssParser;
      result = cssLoader->GetParserFor(nsnull, getter_AddRefs(cssParser));
      if (cssParser) {
        nsCOMPtr<nsIURI> baseURI = aContent->GetBaseURI();

        nsCOMPtr<nsICSSStyleRule> rule;
        result = cssParser->ParseStyleAttribute(aValue, doc->GetDocumentURI(),
                                                baseURI,
                                                aContent->NodePrincipal(),
                                                getter_AddRefs(rule));
        cssLoader->RecycleParser(cssParser);

        if (rule) {
          aResult.SetTo(rule);
          return;
        }
      }
    }
  }

  aResult.SetTo(aValue);
}


 void
nsStyledElement::Shutdown()
{
  NS_IF_RELEASE(gCSSOMFactory);
}
