






#include "nsDOMCSSAttrDeclaration.h"

#include "mozilla/css/Declaration.h"
#include "mozilla/css/StyleRule.h"
#include "mozilla/dom/Element.h"
#include "nsIDocument.h"
#include "nsIDOMMutationEvent.h"
#include "nsIURI.h"
#include "nsNodeUtils.h"
#include "nsWrapperCacheInlines.h"
#include "nsIFrame.h"
#include "ActiveLayerTracker.h"

using namespace mozilla;

nsDOMCSSAttributeDeclaration::nsDOMCSSAttributeDeclaration(dom::Element* aElement,
                                                           bool aIsSMILOverride)
  : mElement(aElement)
  , mIsSMILOverride(aIsSMILOverride)
{
  MOZ_COUNT_CTOR(nsDOMCSSAttributeDeclaration);

  NS_ASSERTION(aElement, "Inline style for a NULL element?");
}

nsDOMCSSAttributeDeclaration::~nsDOMCSSAttributeDeclaration()
{
  MOZ_COUNT_DTOR(nsDOMCSSAttributeDeclaration);
}

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE(nsDOMCSSAttributeDeclaration, mElement)




NS_IMPL_CYCLE_COLLECTION_CAN_SKIP_BEGIN(nsDOMCSSAttributeDeclaration)
  if (tmp->mElement && Element::CanSkip(tmp->mElement, true)) {
    if (tmp->PreservingWrapper()) {
      
      tmp->GetWrapper();
    }
    return true;
  }
  return tmp->IsBlack();
NS_IMPL_CYCLE_COLLECTION_CAN_SKIP_END

NS_IMPL_CYCLE_COLLECTION_CAN_SKIP_IN_CC_BEGIN(nsDOMCSSAttributeDeclaration)
  return tmp->IsBlack() ||
    (tmp->mElement && Element::CanSkipInCC(tmp->mElement));
NS_IMPL_CYCLE_COLLECTION_CAN_SKIP_IN_CC_END

NS_IMPL_CYCLE_COLLECTION_CAN_SKIP_THIS_BEGIN(nsDOMCSSAttributeDeclaration)
  return tmp->IsBlack() ||
    (tmp->mElement && Element::CanSkipThis(tmp->mElement));
NS_IMPL_CYCLE_COLLECTION_CAN_SKIP_THIS_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsDOMCSSAttributeDeclaration)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
NS_IMPL_QUERY_TAIL_INHERITING(nsDOMCSSDeclaration)

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsDOMCSSAttributeDeclaration)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsDOMCSSAttributeDeclaration)

nsresult
nsDOMCSSAttributeDeclaration::SetCSSDeclaration(css::Declaration* aDecl)
{
  NS_ASSERTION(mElement, "Must have Element to set the declaration!");
  css::StyleRule* oldRule =
    mIsSMILOverride ? mElement->GetSMILOverrideStyleRule() :
    mElement->GetInlineStyleRule();
  NS_ASSERTION(oldRule, "Element must have rule");

  nsRefPtr<css::StyleRule> newRule =
    oldRule->DeclarationChanged(aDecl, false);
  if (!newRule) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  return
    mIsSMILOverride ? mElement->SetSMILOverrideStyleRule(newRule, true) :
    mElement->SetInlineStyleRule(newRule, nullptr, true);
}

nsIDocument*
nsDOMCSSAttributeDeclaration::DocToUpdate()
{
  
  
  return mElement->OwnerDoc();
}

css::Declaration*
nsDOMCSSAttributeDeclaration::GetCSSDeclaration(Operation aOperation)
{
  if (!mElement)
    return nullptr;

  css::StyleRule* cssRule;
  if (mIsSMILOverride)
    cssRule = mElement->GetSMILOverrideStyleRule();
  else
    cssRule = mElement->GetInlineStyleRule();

  
  
  
  
  
  
  

  
  
  
  
  if (!mIsSMILOverride &&
      ((aOperation == eOperation_Modify) ||
       (aOperation == eOperation_RemoveProperty && cssRule))) {
    nsNodeUtils::AttributeWillChange(mElement, kNameSpaceID_None,
                                     nsGkAtoms::style,
                                     nsIDOMMutationEvent::MODIFICATION);
  }

  if (cssRule) {
    return cssRule->GetDeclaration();
  }

  if (aOperation != eOperation_Modify) {
    return nullptr;
  }

  
  css::Declaration *decl = new css::Declaration();
  decl->InitializeEmpty();
  nsRefPtr<css::StyleRule> newRule = new css::StyleRule(nullptr, decl, 0, 0);

  
  nsresult rv;
  if (mIsSMILOverride)
    rv = mElement->SetSMILOverrideStyleRule(newRule, false);
  else
    rv = mElement->SetInlineStyleRule(newRule, nullptr, false);

  if (NS_FAILED(rv)) {
    return nullptr; 
  }

  return decl;
}

void
nsDOMCSSAttributeDeclaration::GetCSSParsingEnvironment(CSSParsingEnvironment& aCSSParseEnv)
{
  NS_ASSERTION(mElement, "Something is severely broken -- there should be an Element here!");

  nsIDocument* doc = mElement->OwnerDoc();
  aCSSParseEnv.mSheetURI = doc->GetDocumentURI();
  aCSSParseEnv.mBaseURI = mElement->GetBaseURI();
  aCSSParseEnv.mPrincipal = mElement->NodePrincipal();
  aCSSParseEnv.mCSSLoader = doc->CSSLoader();
}

NS_IMETHODIMP
nsDOMCSSAttributeDeclaration::GetParentRule(nsIDOMCSSRule **aParent)
{
  NS_ENSURE_ARG_POINTER(aParent);

  *aParent = nullptr;
  return NS_OK;
}

 nsINode*
nsDOMCSSAttributeDeclaration::GetParentObject()
{
  return mElement;
}

NS_IMETHODIMP
nsDOMCSSAttributeDeclaration::SetPropertyValue(const nsCSSProperty aPropID,
                                               const nsAString& aValue)
{
  
  
  
  if (aPropID == eCSSProperty_opacity || aPropID == eCSSProperty_transform ||
      aPropID == eCSSProperty_left || aPropID == eCSSProperty_top ||
      aPropID == eCSSProperty_right || aPropID == eCSSProperty_bottom ||
      aPropID == eCSSProperty_margin_left || aPropID == eCSSProperty_margin_top ||
      aPropID == eCSSProperty_margin_right || aPropID == eCSSProperty_margin_bottom) {
    nsIFrame* frame = mElement->GetPrimaryFrame();
    if (frame) {
      ActiveLayerTracker::NotifyInlineStyleRuleModified(frame, aPropID);
    }
  }
  return nsDOMCSSDeclaration::SetPropertyValue(aPropID, aValue);
}
