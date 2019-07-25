





































#include "nsSVGAnimationElement.h"
#include "nsSVGSVGElement.h"
#include "nsSMILTimeContainer.h"
#include "nsSMILAnimationController.h"
#include "nsSMILAnimationFunction.h"
#include "nsISMILAttr.h"

using namespace mozilla::dom;




NS_IMPL_ADDREF_INHERITED(nsSVGAnimationElement, nsSVGAnimationElementBase)
NS_IMPL_RELEASE_INHERITED(nsSVGAnimationElement, nsSVGAnimationElementBase)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsSVGAnimationElement)
  NS_INTERFACE_MAP_ENTRY(nsISMILAnimationElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMElementTimeControl)
NS_INTERFACE_MAP_END_INHERITING(nsSVGAnimationElementBase)


NS_IMPL_CYCLE_COLLECTION_CLASS(nsSVGAnimationElement)
NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(nsSVGAnimationElement,
                                                nsSVGAnimationElementBase)
  tmp->mHrefTarget.Unlink();
  tmp->mTimedElement.Unlink();
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(nsSVGAnimationElement,
                                                  nsSVGAnimationElementBase)
  tmp->mHrefTarget.Traverse(&cb);
  tmp->mTimedElement.Traverse(&cb);
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END




#ifdef _MSC_VER



#pragma warning(push)
#pragma warning(disable:4355)
#endif
nsSVGAnimationElement::nsSVGAnimationElement(already_AddRefed<nsINodeInfo> aNodeInfo)
  : nsSVGAnimationElementBase(aNodeInfo),
    mHrefTarget(this)
#ifdef _MSC_VER
#pragma warning(pop)
#endif
{
}

nsresult
nsSVGAnimationElement::Init()
{
  nsresult rv = nsSVGAnimationElementBase::Init();
  NS_ENSURE_SUCCESS(rv, rv);

  mTimedElement.SetAnimationElement(this);
  AnimationFunction().SetAnimationElement(this);
  mTimedElement.SetTimeClient(&AnimationFunction());

  return NS_OK;
}




const Element&
nsSVGAnimationElement::AsElement() const
{
  return *this;
}

Element&
nsSVGAnimationElement::AsElement()
{
  return *this;
}

const nsAttrValue*
nsSVGAnimationElement::GetAnimAttr(nsIAtom* aName) const
{
  return mAttrsAndChildren.GetAttr(aName, kNameSpaceID_None);
}

PRBool
nsSVGAnimationElement::GetAnimAttr(nsIAtom* aAttName,
                                   nsAString& aResult) const
{
  return GetAttr(kNameSpaceID_None, aAttName, aResult);
}

PRBool
nsSVGAnimationElement::HasAnimAttr(nsIAtom* aAttName) const
{
  return HasAttr(kNameSpaceID_None, aAttName);
}

Element*
nsSVGAnimationElement::GetTargetElementContent()
{
  if (HasAttr(kNameSpaceID_XLink, nsGkAtoms::href)) {
    return mHrefTarget.get();
  }
  NS_ABORT_IF_FALSE(!mHrefTarget.get(),
                    "We shouldn't have an xlink:href target "
                    "if we don't have an xlink:href attribute");

  
  nsIContent* parent = GetFlattenedTreeParent();
  return parent && parent->IsElement() ? parent->AsElement() : nsnull;
}

PRBool
nsSVGAnimationElement::GetTargetAttributeName(PRInt32 *aNamespaceID,
                                              nsIAtom **aLocalName) const
{
  const nsAttrValue* nameAttr
    = mAttrsAndChildren.GetAttr(nsGkAtoms::attributeName);

  if (!nameAttr)
    return PR_FALSE;

  NS_ASSERTION(nameAttr->Type() == nsAttrValue::eAtom,
    "attributeName should have been parsed as an atom");

  return NS_SUCCEEDED(nsContentUtils::SplitQName(
                        this, nsDependentAtomString(nameAttr->GetAtomValue()),
                        aNamespaceID, aLocalName));
}

nsSMILTargetAttrType
nsSVGAnimationElement::GetTargetAttributeType() const
{
  nsIContent::AttrValuesArray typeValues[] = { &nsGkAtoms::css,
                                               &nsGkAtoms::XML,
                                               nsnull};
  nsSMILTargetAttrType smilTypes[] = { eSMILTargetAttrType_CSS,
                                       eSMILTargetAttrType_XML };
  PRInt32 index = FindAttrValueIn(kNameSpaceID_None,
                                  nsGkAtoms::attributeType,
                                  typeValues,
                                  eCaseMatters);
  return (index >= 0) ? smilTypes[index] : eSMILTargetAttrType_auto;
}

nsSMILTimedElement&
nsSVGAnimationElement::TimedElement()
{
  return mTimedElement;
}





NS_IMETHODIMP
nsSVGAnimationElement::GetTargetElement(nsIDOMSVGElement** aTarget)
{
  FlushAnimations();

  
  nsIContent* targetContent = GetTargetElementContent();

  nsCOMPtr<nsIDOMSVGElement> targetSVG = do_QueryInterface(targetContent);
  NS_IF_ADDREF(*aTarget = targetSVG);

  return NS_OK;
}


NS_IMETHODIMP
nsSVGAnimationElement::GetStartTime(float* retval)
{
  FlushAnimations();

  nsSMILTimeValue startTime = mTimedElement.GetStartTime();
  if (!startTime.IsResolved())
    return NS_ERROR_DOM_INVALID_STATE_ERR;

  *retval = float(double(startTime.GetMillis()) / PR_MSEC_PER_SEC);

  return NS_OK;
}


NS_IMETHODIMP
nsSVGAnimationElement::GetCurrentTime(float* retval)
{
  

  nsSMILTimeContainer* root = GetTimeContainer();
  if (root) {
    *retval = float(double(root->GetCurrentTime()) / PR_MSEC_PER_SEC);
  } else {
    *retval = 0.f;
  }
  return NS_OK;
}


NS_IMETHODIMP
nsSVGAnimationElement::GetSimpleDuration(float* retval)
{
  

  nsSMILTimeValue simpleDur = mTimedElement.GetSimpleDuration();
  if (!simpleDur.IsResolved()) {
    *retval = 0.f;
    return NS_ERROR_DOM_NOT_SUPPORTED_ERR;
  }

  *retval = float(double(simpleDur.GetMillis()) / PR_MSEC_PER_SEC);
  return NS_OK;
}




nsresult
nsSVGAnimationElement::BindToTree(nsIDocument* aDocument,
                                  nsIContent* aParent,
                                  nsIContent* aBindingParent,
                                  PRBool aCompileEventHandlers)
{
  NS_ABORT_IF_FALSE(!mHrefTarget.get(),
                    "Shouldn't have href-target yet "
                    "(or it should've been cleared)");
  nsresult rv = nsSVGAnimationElementBase::BindToTree(aDocument, aParent,
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
nsSVGAnimationElement::UnbindFromTree(PRBool aDeep, PRBool aNullParent)
{
  nsIDocument *doc = GetOwnerDoc();
  if (doc) {
    nsSMILAnimationController *controller = doc->GetAnimationController();
    if (controller) {
      controller->UnregisterAnimationElement(this);
    }
  }

  mHrefTarget.Unlink();
  mTimedElement.DissolveReferences();

  AnimationNeedsResample();

  nsSVGAnimationElementBase::UnbindFromTree(aDeep, aNullParent);
}

PRBool
nsSVGAnimationElement::ParseAttribute(PRInt32 aNamespaceID,
                                      nsIAtom* aAttribute,
                                      const nsAString& aValue,
                                      nsAttrValue& aResult)
{
  if (aNamespaceID == kNameSpaceID_None) {
    
    if (aAttribute == nsGkAtoms::attributeName ||
        aAttribute == nsGkAtoms::attributeType) {
      aResult.ParseAtom(aValue);
      AnimationNeedsResample();
      return PR_TRUE;
    }

    nsresult rv = NS_ERROR_FAILURE;

    
    PRBool foundMatch =
      AnimationFunction().SetAttr(aAttribute, aValue, aResult, &rv);

    
    
    if (!foundMatch) {
      foundMatch =
        mTimedElement.SetAttr(aAttribute, aValue, aResult, this, &rv);
    }

    if (foundMatch) {
      AnimationNeedsResample();
      if (NS_FAILED(rv)) {
        ReportAttributeParseFailure(GetOwnerDoc(), aAttribute, aValue);
        return PR_FALSE;
      }
      return PR_TRUE;
    }
  }

  return nsSVGAnimationElementBase::ParseAttribute(aNamespaceID, aAttribute,
                                                   aValue, aResult);
}

nsresult
nsSVGAnimationElement::AfterSetAttr(PRInt32 aNamespaceID, nsIAtom* aName,
                                    const nsAString* aValue, PRBool aNotify)
{
  nsresult rv =
    nsSVGAnimationElementBase::AfterSetAttr(aNamespaceID, aName, aValue,
                                            aNotify);

  if (aNamespaceID != kNameSpaceID_XLink || aName != nsGkAtoms::href)
    return rv;

  if (!aValue) {
    mHrefTarget.Unlink();
    AnimationTargetChanged();
  } else if (IsInDoc()) {
    UpdateHrefTarget(this, *aValue);
  } 
    

  return rv;
}

nsresult
nsSVGAnimationElement::UnsetAttr(PRInt32 aNamespaceID,
                                 nsIAtom* aAttribute, PRBool aNotify)
{
  nsresult rv = nsSVGAnimationElementBase::UnsetAttr(aNamespaceID, aAttribute,
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

PRBool
nsSVGAnimationElement::IsNodeOfType(PRUint32 aFlags) const
{
  return !(aFlags & ~(eCONTENT | eSVG | eANIMATION));
}




nsSMILTimeContainer*
nsSVGAnimationElement::GetTimeContainer()
{
  nsSVGSVGElement *element = nsSVGUtils::GetOuterSVGElement(this);

  if (element) {
    return element->GetTimedDocumentRoot();
  }

  return nsnull;
}



NS_IMETHODIMP
nsSVGAnimationElement::BeginElement(void)
{
  return BeginElementAt(0.f);
}


NS_IMETHODIMP
nsSVGAnimationElement::BeginElementAt(float offset)
{
  NS_ENSURE_FINITE(offset, NS_ERROR_ILLEGAL_VALUE);

  
  FlushAnimations();

  
  
  nsresult rv = mTimedElement.BeginElementAt(offset);
  if (NS_FAILED(rv))
    return rv;

  AnimationNeedsResample();
  
  
  FlushAnimations();

  return NS_OK;
}


NS_IMETHODIMP
nsSVGAnimationElement::EndElement(void)
{
  return EndElementAt(0.f);
}


NS_IMETHODIMP
nsSVGAnimationElement::EndElementAt(float offset)
{
  NS_ENSURE_FINITE(offset, NS_ERROR_ILLEGAL_VALUE);

  
  FlushAnimations();

  nsresult rv = mTimedElement.EndElementAt(offset);
  if (NS_FAILED(rv))
    return rv;

  AnimationNeedsResample();
  
  FlushAnimations();
 
  return NS_OK;
}

PRBool
nsSVGAnimationElement::IsEventName(nsIAtom* aName)
{
  return nsContentUtils::IsEventAttributeName(aName, EventNameType_SMIL);
}

void
nsSVGAnimationElement::UpdateHrefTarget(nsIContent* aNodeForContext,
                                        const nsAString& aHrefStr)
{
  nsCOMPtr<nsIURI> targetURI;
  nsCOMPtr<nsIURI> baseURI = GetBaseURI();
  nsContentUtils::NewURIWithDocumentCharset(getter_AddRefs(targetURI),
                                            aHrefStr, GetOwnerDoc(), baseURI);
  mHrefTarget.Reset(aNodeForContext, targetURI);
  AnimationTargetChanged();
}

void
nsSVGAnimationElement::AnimationTargetChanged()
{
  mTimedElement.HandleTargetElementChange(GetTargetElementContent());
  AnimationNeedsResample();
}
