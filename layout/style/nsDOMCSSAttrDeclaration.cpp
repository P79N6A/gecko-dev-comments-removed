






































#include "nsDOMCSSAttrDeclaration.h"
#include "mozilla/css/Declaration.h"
#include "nsIDocument.h"
#include "nsIDOMMutationEvent.h"
#include "nsICSSStyleRule.h"
#include "mozilla/css/Loader.h"
#include "nsIURI.h"
#include "nsINameSpaceManager.h"
#include "nsStyleConsts.h"
#include "nsContentUtils.h"
#include "nsIContent.h"
#include "nsIPrincipal.h"
#include "nsNodeUtils.h"

namespace css = mozilla::css;

nsDOMCSSAttributeDeclaration::nsDOMCSSAttributeDeclaration(nsIContent *aContent
#ifdef MOZ_SMIL
                                                           , PRBool aIsSMILOverride
#endif 
                                                           )
  : mContent(aContent)
#ifdef MOZ_SMIL
  , mIsSMILOverride(aIsSMILOverride)
#endif 
{
  MOZ_COUNT_CTOR(nsDOMCSSAttributeDeclaration);

  NS_ASSERTION(aContent && aContent->IsElement(),
               "Inline style for non-element content?");
}

nsDOMCSSAttributeDeclaration::~nsDOMCSSAttributeDeclaration()
{
  MOZ_COUNT_DTOR(nsDOMCSSAttributeDeclaration);
}

NS_IMPL_CYCLE_COLLECTION_1(nsDOMCSSAttributeDeclaration, mContent)

NS_INTERFACE_MAP_BEGIN(nsDOMCSSAttributeDeclaration)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRIES_CYCLE_COLLECTION(nsDOMCSSAttributeDeclaration)
NS_IMPL_QUERY_TAIL_INHERITING(nsDOMCSSDeclaration)

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsDOMCSSAttributeDeclaration)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsDOMCSSAttributeDeclaration)

nsresult
nsDOMCSSAttributeDeclaration::SetCSSDeclaration(css::Declaration* aDecl)
{
  NS_ASSERTION(mContent, "Must have content node to set the decl!");
  nsICSSStyleRule* oldRule =
#ifdef MOZ_SMIL
    mIsSMILOverride ? mContent->GetSMILOverrideStyleRule() :
#endif 
    mContent->GetInlineStyleRule();
  NS_ASSERTION(oldRule, "content must have rule");

  nsCOMPtr<nsICSSStyleRule> newRule =
    oldRule->DeclarationChanged(aDecl, PR_FALSE);
  if (!newRule) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  return
#ifdef MOZ_SMIL
    mIsSMILOverride ? mContent->SetSMILOverrideStyleRule(newRule, PR_TRUE) :
#endif 
    mContent->SetInlineStyleRule(newRule, PR_TRUE);
}

nsIDocument*
nsDOMCSSAttributeDeclaration::DocToUpdate()
{
  
  
  
  
#ifdef MOZ_SMIL
  if (!mIsSMILOverride)
#endif
  {
    nsNodeUtils::AttributeWillChange(mContent, kNameSpaceID_None,
                                     nsGkAtoms::style,
                                     nsIDOMMutationEvent::MODIFICATION);
  }
  
  
  
  return mContent->GetOwnerDoc();
}

css::Declaration*
nsDOMCSSAttributeDeclaration::GetCSSDeclaration(PRBool aAllocate)
{
  if (!mContent)
    return nsnull;

  nsICSSStyleRule* cssRule;
#ifdef MOZ_SMIL
  if (mIsSMILOverride)
    cssRule = mContent->GetSMILOverrideStyleRule();
  else
#endif 
    cssRule = mContent->GetInlineStyleRule();

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
    rv = mContent->SetSMILOverrideStyleRule(newRule, PR_FALSE);
  else
#endif 
    rv = mContent->SetInlineStyleRule(newRule, PR_FALSE);

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
  NS_ASSERTION(mContent, "Something is severely broken -- there should be an nsIContent here!");
  
  *aSheetURI = nsnull;
  *aBaseURI = nsnull;
  *aSheetPrincipal = nsnull;
  *aCSSLoader = nsnull;

  nsIDocument* doc = mContent->GetOwnerDoc();
  if (!doc) {
    
    return NS_ERROR_NOT_AVAILABLE;
  }

  nsCOMPtr<nsIURI> baseURI = mContent->GetBaseURI();
  nsCOMPtr<nsIURI> sheetURI = doc->GetDocumentURI();

  NS_ADDREF(*aCSSLoader = doc->CSSLoader());

  baseURI.swap(*aBaseURI);
  sheetURI.swap(*aSheetURI);
  NS_ADDREF(*aSheetPrincipal = mContent->NodePrincipal());

  return NS_OK;
}

NS_IMETHODIMP
nsDOMCSSAttributeDeclaration::GetParentRule(nsIDOMCSSRule **aParent)
{
  NS_ENSURE_ARG_POINTER(aParent);

  *aParent = nsnull;
  return NS_OK;
}

