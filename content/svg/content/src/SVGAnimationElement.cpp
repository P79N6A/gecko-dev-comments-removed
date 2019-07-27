




#include "mozilla/dom/SVGAnimationElement.h"
#include "mozilla/dom/SVGSVGElement.h"
#include "nsSMILTimeContainer.h"
#include "nsSMILAnimationController.h"
#include "nsSMILAnimationFunction.h"
#include "nsContentUtils.h"
#include "nsIURI.h"
#include "prtime.h"

namespace mozilla {
namespace dom {




NS_IMPL_ADDREF_INHERITED(SVGAnimationElement, SVGAnimationElementBase)
NS_IMPL_RELEASE_INHERITED(SVGAnimationElement, SVGAnimationElementBase)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(SVGAnimationElement)
  NS_INTERFACE_MAP_ENTRY(mozilla::dom::SVGTests)
NS_INTERFACE_MAP_END_INHERITING(SVGAnimationElementBase)

NS_IMPL_CYCLE_COLLECTION_INHERITED(SVGAnimationElement,
                                   SVGAnimationElementBase,
                                   mHrefTarget, mTimedElement)




SVGAnimationElement::SVGAnimationElement(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo)
  : SVGAnimationElementBase(aNodeInfo),
    mHrefTarget(MOZ_THIS_IN_INITIALIZER_LIST())
{
}

SVGAnimationElement::~SVGAnimationElement()
{
}

nsresult
SVGAnimationElement::Init()
{
  nsresult rv = SVGAnimationElementBase::Init();
  NS_ENSURE_SUCCESS(rv, rv);

  mTimedElement.SetAnimationElement(this);
  AnimationFunction().SetAnimationElement(this);
  mTimedElement.SetTimeClient(&AnimationFunction());

  return NS_OK;
}



const nsAttrValue*
SVGAnimationElement::GetAnimAttr(nsIAtom* aName) const
{
  return mAttrsAndChildren.GetAttr(aName, kNameSpaceID_None);
}

bool
SVGAnimationElement::GetAnimAttr(nsIAtom* aAttName,
                                 nsAString& aResult) const
{
  return GetAttr(kNameSpaceID_None, aAttName, aResult);
}

bool
SVGAnimationElement::HasAnimAttr(nsIAtom* aAttName) const
{
  return HasAttr(kNameSpaceID_None, aAttName);
}

Element*
SVGAnimationElement::GetTargetElementContent()
{
  if (HasAttr(kNameSpaceID_XLink, nsGkAtoms::href)) {
    return mHrefTarget.get();
  }
  NS_ABORT_IF_FALSE(!mHrefTarget.get(),
                    "We shouldn't have an xlink:href target "
                    "if we don't have an xlink:href attribute");

  
  nsIContent* parent = GetFlattenedTreeParent();
  return parent && parent->IsElement() ? parent->AsElement() : nullptr;
}

bool
SVGAnimationElement::GetTargetAttributeName(int32_t *aNamespaceID,
                                            nsIAtom **aLocalName) const
{
  const nsAttrValue* nameAttr
    = mAttrsAndChildren.GetAttr(nsGkAtoms::attributeName);

  if (!nameAttr)
    return false;

  NS_ASSERTION(nameAttr->Type() == nsAttrValue::eAtom,
    "attributeName should have been parsed as an atom");

  return NS_SUCCEEDED(nsContentUtils::SplitQName(
                        this, nsDependentAtomString(nameAttr->GetAtomValue()),
                        aNamespaceID, aLocalName));
}

nsSMILTargetAttrType
SVGAnimationElement::GetTargetAttributeType() const
{
  nsIContent::AttrValuesArray typeValues[] = { &nsGkAtoms::css,
                                               &nsGkAtoms::XML,
                                               nullptr};
  nsSMILTargetAttrType smilTypes[] = { eSMILTargetAttrType_CSS,
                                       eSMILTargetAttrType_XML };
  int32_t index = FindAttrValueIn(kNameSpaceID_None,
                                  nsGkAtoms::attributeType,
                                  typeValues,
                                  eCaseMatters);
  return (index >= 0) ? smilTypes[index] : eSMILTargetAttrType_auto;
}

nsSMILTimedElement&
SVGAnimationElement::TimedElement()
{
  return mTimedElement;
}

nsSVGElement*
SVGAnimationElement::GetTargetElement()
{
  FlushAnimations();

  
  nsIContent* target = GetTargetElementContent();

  return (target && target->IsSVG()) ? static_cast<nsSVGElement*>(target) : nullptr;
}

float
SVGAnimationElement::GetStartTime(ErrorResult& rv)
{
  FlushAnimations();

  nsSMILTimeValue startTime = mTimedElement.GetStartTime();
  if (!startTime.IsDefinite()) {
    rv.Throw(NS_ERROR_DOM_INVALID_STATE_ERR);
    return 0.f;
  }

  return float(double(startTime.GetMillis()) / PR_MSEC_PER_SEC);
}

float
SVGAnimationElement::GetCurrentTime()
{
  

  nsSMILTimeContainer* root = GetTimeContainer();
  if (root) {
    return float(double(root->GetCurrentTime()) / PR_MSEC_PER_SEC);
  }

  return 0.0f;
}

float
SVGAnimationElement::GetSimpleDuration(ErrorResult& rv)
{
  

  nsSMILTimeValue simpleDur = mTimedElement.GetSimpleDuration();
  if (!simpleDur.IsDefinite()) {
    rv.Throw(NS_ERROR_DOM_NOT_SUPPORTED_ERR);
    return 0.f;
  }

  return float(double(simpleDur.GetMillis()) / PR_MSEC_PER_SEC);
}




nsresult
SVGAnimationElement::BindToTree(nsIDocument* aDocument,
                                nsIContent* aParent,
                                nsIContent* aBindingParent,
                                bool aCompileEventHandlers)
{
  NS_ABORT_IF_FALSE(!mHrefTarget.get(),
                    "Shouldn't have href-target yet "
                    "(or it should've been cleared)");
  nsresult rv = SVGAnimationElementBase::BindToTree(aDocument, aParent,
                                                    aBindingParent,
                                                    aCompileEventHandlers);
  NS_ENSURE_SUCCESS(rv,rv);

  
  if (!GetCtx()) {
    
    
    
    
    return NS_OK;
  }

  
  if (aDocument) {
    nsSMILAnimationController *controller = aDocument->GetAnimationController();
    if (controller) {
      controller->RegisterAnimationElement(this);
    }
    const nsAttrValue* href = mAttrsAndChildren.GetAttr(nsGkAtoms::href,
                                                        kNameSpaceID_XLink);
    if (href) {
      nsAutoString hrefStr;
      href->ToString(hrefStr);

      
      
      
      UpdateHrefTarget(aParent, hrefStr);
    }

    mTimedElement.BindToTree(aParent);
  }

  AnimationNeedsResample();

  return NS_OK;
}

void
SVGAnimationElement::UnbindFromTree(bool aDeep, bool aNullParent)
{
  nsSMILAnimationController *controller = OwnerDoc()->GetAnimationController();
  if (controller) {
    controller->UnregisterAnimationElement(this);
  }

  mHrefTarget.Unlink();
  mTimedElement.DissolveReferences();

  AnimationNeedsResample();

  SVGAnimationElementBase::UnbindFromTree(aDeep, aNullParent);
}

bool
SVGAnimationElement::ParseAttribute(int32_t aNamespaceID,
                                    nsIAtom* aAttribute,
                                    const nsAString& aValue,
                                    nsAttrValue& aResult)
{
  if (aNamespaceID == kNameSpaceID_None) {
    
    if (aAttribute == nsGkAtoms::attributeName ||
        aAttribute == nsGkAtoms::attributeType) {
      aResult.ParseAtom(aValue);
      AnimationNeedsResample();
      return true;
    }

    nsresult rv = NS_ERROR_FAILURE;

    
    bool foundMatch =
      AnimationFunction().SetAttr(aAttribute, aValue, aResult, &rv);

    
    
    if (!foundMatch) {
      foundMatch =
        mTimedElement.SetAttr(aAttribute, aValue, aResult, this, &rv);
    }

    if (foundMatch) {
      AnimationNeedsResample();
      if (NS_FAILED(rv)) {
        ReportAttributeParseFailure(OwnerDoc(), aAttribute, aValue);
        return false;
      }
      return true;
    }
  }

  return SVGAnimationElementBase::ParseAttribute(aNamespaceID, aAttribute,
                                                 aValue, aResult);
}

nsresult
SVGAnimationElement::AfterSetAttr(int32_t aNamespaceID, nsIAtom* aName,
                                  const nsAttrValue* aValue, bool aNotify)
{
  nsresult rv =
    SVGAnimationElementBase::AfterSetAttr(aNamespaceID, aName, aValue,
                                          aNotify);

  if (SVGTests::IsConditionalProcessingAttribute(aName)) {
    bool isDisabled = !SVGTests::PassesConditionalProcessingTests();
    if (mTimedElement.SetIsDisabled(isDisabled)) {
      AnimationNeedsResample();
    }
  }

  if (aNamespaceID != kNameSpaceID_XLink || aName != nsGkAtoms::href)
    return rv;

  if (!aValue) {
    mHrefTarget.Unlink();
    AnimationTargetChanged();
  } else if (IsInDoc()) {
    NS_ABORT_IF_FALSE(aValue->Type() == nsAttrValue::eString,
                      "Expected href attribute to be string type");
    UpdateHrefTarget(this, aValue->GetStringValue());
  } 
    

  return rv;
}

nsresult
SVGAnimationElement::UnsetAttr(int32_t aNamespaceID,
                               nsIAtom* aAttribute, bool aNotify)
{
  nsresult rv = SVGAnimationElementBase::UnsetAttr(aNamespaceID, aAttribute,
                                                   aNotify);
  NS_ENSURE_SUCCESS(rv,rv);

  if (aNamespaceID == kNameSpaceID_None) {
    if (AnimationFunction().UnsetAttr(aAttribute) ||
        mTimedElement.UnsetAttr(aAttribute)) {
      AnimationNeedsResample();
    }
  }

  return NS_OK;
}

bool
SVGAnimationElement::IsNodeOfType(uint32_t aFlags) const
{
  return !(aFlags & ~(eCONTENT | eANIMATION));
}




void
SVGAnimationElement::ActivateByHyperlink()
{
  FlushAnimations();

  
  
  
  nsSMILTimeValue seekTime = mTimedElement.GetHyperlinkTime();
  if (seekTime.IsDefinite()) {
    nsSMILTimeContainer* timeContainer = GetTimeContainer();
    if (timeContainer) {
      timeContainer->SetCurrentTime(seekTime.GetMillis());
      AnimationNeedsResample();
      
      
      FlushAnimations();
    }
    
    
  } else {
    ErrorResult rv;
    BeginElement(rv);
  }
}




nsSMILTimeContainer*
SVGAnimationElement::GetTimeContainer()
{
  SVGSVGElement *element = SVGContentUtils::GetOuterSVGElement(this);

  if (element) {
    return element->GetTimedDocumentRoot();
  }

  return nullptr;
}

void
SVGAnimationElement::BeginElementAt(float offset, ErrorResult& rv)
{
  
  FlushAnimations();

  
  
  rv = mTimedElement.BeginElementAt(offset);
  if (rv.Failed())
    return;

  AnimationNeedsResample();
  
  
  FlushAnimations();
}

void
SVGAnimationElement::EndElementAt(float offset, ErrorResult& rv)
{
  
  FlushAnimations();

  rv = mTimedElement.EndElementAt(offset);
  if (rv.Failed())
    return;

  AnimationNeedsResample();
  
  FlushAnimations();
}

bool
SVGAnimationElement::IsEventAttributeName(nsIAtom* aName)
{
  return nsContentUtils::IsEventAttributeName(aName, EventNameType_SMIL);
}

void
SVGAnimationElement::UpdateHrefTarget(nsIContent* aNodeForContext,
                                      const nsAString& aHrefStr)
{
  nsCOMPtr<nsIURI> targetURI;
  nsCOMPtr<nsIURI> baseURI = GetBaseURI();
  nsContentUtils::NewURIWithDocumentCharset(getter_AddRefs(targetURI),
                                            aHrefStr, OwnerDoc(), baseURI);
  mHrefTarget.Reset(aNodeForContext, targetURI);
  AnimationTargetChanged();
}

void
SVGAnimationElement::AnimationTargetChanged()
{
  mTimedElement.HandleTargetElementChange(GetTargetElementContent());
  AnimationNeedsResample();
}

} 
} 
