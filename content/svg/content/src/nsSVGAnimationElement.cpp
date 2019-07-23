





































#include "nsSVGAnimationElement.h"
#include "nsSVGSVGElement.h"
#include "nsSMILTimeContainer.h"
#include "nsSMILAnimationController.h"
#include "nsSMILAnimationFunction.h"
#include "nsISMILAttr.h"




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
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(nsSVGAnimationElement,
                                                  nsSVGAnimationElementBase)
  tmp->mHrefTarget.Traverse(&cb);
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END




nsSVGAnimationElement::nsSVGAnimationElement(nsINodeInfo *aNodeInfo)
  : nsSVGAnimationElementBase(aNodeInfo),
    mHrefTarget(this),
    mTimedDocumentRoot(nsnull)
{
}

nsresult
nsSVGAnimationElement::Init()
{
  nsresult rv = nsSVGAnimationElementBase::Init();
  NS_ENSURE_SUCCESS(rv, rv);

  AnimationFunction().SetAnimationElement(this);
  mTimedElement.SetTimeClient(&AnimationFunction());

  return NS_OK;
}




const nsIContent&
nsSVGAnimationElement::Content() const
{
  return *this;
}

nsIContent&
nsSVGAnimationElement::Content()
{
  return *this;
}

const nsAttrValue*
nsSVGAnimationElement::GetAnimAttr(nsIAtom* aName) const
{
  return mAttrsAndChildren.GetAttr(aName, kNameSpaceID_None);
}

nsIContent*
nsSVGAnimationElement::GetTargetElementContent()
{
  if (HasAttr(kNameSpaceID_XLink, nsGkAtoms::href)) {
    return mHrefTarget.get();
  }
  NS_ABORT_IF_FALSE(!mHrefTarget.get(),
                    "We shouldn't have an xlink:href target "
                    "if we don't have an xlink:href attribute");

  
  return nsSVGUtils::GetParentElement(this);
}

nsIAtom*
nsSVGAnimationElement::GetTargetAttributeName() const
{
  const nsAttrValue* nameAttr
    = mAttrsAndChildren.GetAttr(nsGkAtoms::attributeName);

  if (!nameAttr)
    return nsnull;

  NS_ASSERTION(nameAttr->Type() == nsAttrValue::eAtom,
    "attributeName should have been parsed as an atom");
  return nameAttr->GetAtomValue();
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

  
  nsCOMPtr<nsIDOMSVGSVGElement> ownerDOMSVG;
  rv = GetOwnerSVGElement(getter_AddRefs(ownerDOMSVG));

  if (NS_FAILED(rv) || !ownerDOMSVG)
    
    
    
    
    return NS_OK;

  mTimedDocumentRoot = GetTimeContainer();
  if (!mTimedDocumentRoot)
    
    
    
    return NS_OK;

  
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

  if (mTimedDocumentRoot) {
    mTimedDocumentRoot = nsnull;
  }

  mHrefTarget.Unlink();

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
      foundMatch = mTimedElement.SetAttr(aAttribute, aValue, aResult, &rv);
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

  PRBool returnVal =
    nsSVGAnimationElementBase::ParseAttribute(aNamespaceID, aAttribute,
                                              aValue, aResult);
  if (aNamespaceID == kNameSpaceID_XLink &&
      aAttribute == nsGkAtoms::href &&
      IsInDoc()) {
    
    
    UpdateHrefTarget(this, aValue);
  }
  return returnVal;
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
  } else if (aNamespaceID == kNameSpaceID_XLink) {
    if (aAttribute == nsGkAtoms::href) {
      mHrefTarget.Unlink();
    }
  }

  return NS_OK;
}




nsSMILTimeContainer*
nsSVGAnimationElement::GetTimeContainer()
{
  nsSMILTimeContainer *result = nsnull;
  nsCOMPtr<nsIDOMSVGSVGElement> ownerDOMSVG;

  nsresult rv = GetOwnerSVGElement(getter_AddRefs(ownerDOMSVG));

  if (NS_SUCCEEDED(rv) && ownerDOMSVG) {
    nsSVGSVGElement *ownerSVG =
      static_cast<nsSVGSVGElement*>(ownerDOMSVG.get());
    result = ownerSVG->GetTimedDocumentRoot();
  }

  return result;
}



NS_IMETHODIMP
nsSVGAnimationElement::BeginElement(void)
{
  return BeginElementAt(0.f);
}


NS_IMETHODIMP
nsSVGAnimationElement::BeginElementAt(float offset)
{
  nsresult rv = mTimedElement.BeginElementAt(offset, mTimedDocumentRoot);
  AnimationNeedsResample();

  return rv;
}


NS_IMETHODIMP
nsSVGAnimationElement::EndElement(void)
{
  return EndElementAt(0.f);
}


NS_IMETHODIMP
nsSVGAnimationElement::EndElementAt(float offset)
{
  nsresult rv = mTimedElement.EndElementAt(offset, mTimedDocumentRoot);
  AnimationNeedsResample();

  return rv;
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
}
