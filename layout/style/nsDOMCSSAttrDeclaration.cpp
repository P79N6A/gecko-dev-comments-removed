







































#include "nsDOMCSSAttrDeclaration.h"

#include "mozilla/css/Declaration.h"
#include "mozilla/css/Loader.h"
#include "mozilla/dom/Element.h"
#include "nsICSSStyleRule.h"
#include "nsIDocument.h"
#include "nsIDOMMutationEvent.h"
#include "nsIPrincipal.h"
#include "nsIURI.h"
#include "nsNodeUtils.h"

namespace css = mozilla::css;
namespace dom = mozilla::dom;

nsDOMCSSAttributeDeclaration::nsDOMCSSAttributeDeclaration(dom::Element* aElement
#ifdef MOZ_SMIL
                                                           , PRBool aIsSMILOverride
#endif 
                                                           )
  : mElement(aElement)
#ifdef MOZ_SMIL
  , mIsSMILOverride(aIsSMILOverride)
#endif 
{
  MOZ_COUNT_CTOR(nsDOMCSSAttributeDeclaration);

  NS_ASSERTION(aElement, "Inline style for a NULL element?");
}

nsDOMCSSAttributeDeclaration::~nsDOMCSSAttributeDeclaration()
{
  MOZ_COUNT_DTOR(nsDOMCSSAttributeDeclaration);
}

NS_IMPL_CYCLE_COLLECTION_1(nsDOMCSSAttributeDeclaration, mElement)

NS_INTERFACE_MAP_BEGIN(nsDOMCSSAttributeDeclaration)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRIES_CYCLE_COLLECTION(nsDOMCSSAttributeDeclaration)
NS_IMPL_QUERY_TAIL_INHERITING(nsDOMCSSDeclaration)

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsDOMCSSAttributeDeclaration)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsDOMCSSAttributeDeclaration)

nsresult
nsDOMCSSAttributeDeclaration::SetCSSDeclaration(css::Declaration* aDecl)
{
  NS_ASSERTION(mElement, "Must have Element to set the declaration!");
  nsICSSStyleRule* oldRule =
#ifdef MOZ_SMIL
    mIsSMILOverride ? mElement->GetSMILOverrideStyleRule() :
#endif 
    mElement->GetInlineStyleRule();
  NS_ASSERTION(oldRule, "Element must have rule");

  nsCOMPtr<nsICSSStyleRule> newRule =
    oldRule->DeclarationChanged(aDecl, PR_FALSE);
  if (!newRule) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  return
#ifdef MOZ_SMIL
    mIsSMILOverride ? mElement->SetSMILOverrideStyleRule(newRule, PR_TRUE) :
#endif 
    mElement->SetInlineStyleRule(newRule, PR_TRUE);
}

nsIDocument*
nsDOMCSSAttributeDeclaration::DocToUpdate()
{
  
  
  
  
#ifdef MOZ_SMIL
  if (!mIsSMILOverride)
#endif
  {
    nsNodeUtils::AttributeWillChange(mElement, kNameSpaceID_None,
                                     nsGkAtoms::style,
                                     nsIDOMMutationEvent::MODIFICATION);
  }
 
  
  
  return mElement->GetOwnerDoc();
}

css::Declaration*
nsDOMCSSAttributeDeclaration::GetCSSDeclaration(PRBool aAllocate)
{
  if (!mElement)
    return nsnull;

  nsICSSStyleRule* cssRule;
#ifdef MOZ_SMIL
  if (mIsSMILOverride)
    cssRule = mElement->GetSMILOverrideStyleRule();
  else
#endif 
    cssRule = mElement->GetInlineStyleRule();

  if (cssRule) {
    return cssRule->GetDeclaration();
  }
  if (!aAllocate) {
    return nsnull;
  }

  
  css::Declaration *decl = new css::Declaration();
  decl->InitializeEmpty();
  nsCOMPtr<nsICSSStyleRule> newRule = NS_NewCSSStyleRule(nsnull, decl);

  
  nsresult rv;
#ifdef MOZ_SMIL
  if (mIsSMILOverride)
    rv = mElement->SetSMILOverrideStyleRule(newRule, PR_FALSE);
  else
#endif 
    rv = mElement->SetInlineStyleRule(newRule, PR_FALSE);

  if (NS_FAILED(rv)) {
    return nsnull; 
  }

  return decl;
}






nsresult
nsDOMCSSAttributeDeclaration::GetCSSParsingEnvironment(nsIURI** aSheetURI,
                                                       nsIURI** aBaseURI,
                                                       nsIPrincipal** aSheetPrincipal,
                                                       mozilla::css::Loader** aCSSLoader)
{
  NS_ASSERTION(mElement, "Something is severely broken -- there should be an Element here!");
  
  *aSheetURI = nsnull;
  *aBaseURI = nsnull;
  *aSheetPrincipal = nsnull;
  *aCSSLoader = nsnull;

  nsIDocument* doc = mElement->GetOwnerDoc();
  if (!doc) {
    
    return NS_ERROR_NOT_AVAILABLE;
  }

  nsCOMPtr<nsIURI> baseURI = mElement->GetBaseURI();
  nsCOMPtr<nsIURI> sheetURI = doc->GetDocumentURI();

  NS_ADDREF(*aCSSLoader = doc->CSSLoader());

  baseURI.swap(*aBaseURI);
  sheetURI.swap(*aSheetURI);
  NS_ADDREF(*aSheetPrincipal = mElement->NodePrincipal());

  return NS_OK;
}

NS_IMETHODIMP
nsDOMCSSAttributeDeclaration::GetParentRule(nsIDOMCSSRule **aParent)
{
  NS_ENSURE_ARG_POINTER(aParent);

  *aParent = nsnull;
  return NS_OK;
}

 nsINode*
nsDOMCSSAttributeDeclaration::GetParentObject()
{
  return mElement;
}
