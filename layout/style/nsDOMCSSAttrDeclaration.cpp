







































#include "nsDOMCSSAttrDeclaration.h"

#include "mozilla/css/Declaration.h"
#include "mozilla/css/Loader.h"
#include "mozilla/css/StyleRule.h"
#include "mozilla/dom/Element.h"
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
  css::StyleRule* oldRule =
#ifdef MOZ_SMIL
    mIsSMILOverride ? mElement->GetSMILOverrideStyleRule() :
#endif 
    mElement->GetInlineStyleRule();
  NS_ASSERTION(oldRule, "Element must have rule");

  nsRefPtr<css::StyleRule> newRule =
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

  css::StyleRule* cssRule;
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
  nsRefPtr<css::StyleRule> newRule = new css::StyleRule(nsnull, decl);

  
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

void
nsDOMCSSAttributeDeclaration::GetCSSParsingEnvironment(CSSParsingEnvironment& aCSSParseEnv)
{
  NS_ASSERTION(mElement, "Something is severely broken -- there should be an Element here!");

  nsIDocument* doc = mElement->GetOwnerDoc();
  if (!doc) {
    
    aCSSParseEnv.mPrincipal = nsnull;
    return;
  }

  aCSSParseEnv.mSheetURI = doc->GetDocumentURI();
  aCSSParseEnv.mBaseURI = mElement->GetBaseURI();
  aCSSParseEnv.mPrincipal = mElement->NodePrincipal();
  aCSSParseEnv.mCSSLoader = doc->CSSLoader();
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
