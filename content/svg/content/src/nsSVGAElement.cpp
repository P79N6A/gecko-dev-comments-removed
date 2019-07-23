




































#include "nsSVGGraphicElement.h"
#include "nsIDOMSVGAElement.h"
#include "nsIDOMSVGURIReference.h"
#include "nsILink.h"
#include "nsSVGAnimatedString.h"
#include "nsCOMPtr.h"
#include "nsGkAtoms.h"

typedef nsSVGGraphicElement nsSVGAElementBase;

class nsSVGAElement : public nsSVGAElementBase,
                      public nsIDOMSVGAElement,
                      public nsIDOMSVGURIReference,
                      public nsILink
{
protected:
  friend nsresult NS_NewSVGAElement(nsIContent **aResult,
                                    nsINodeInfo *aNodeInfo);
  nsSVGAElement(nsINodeInfo *aNodeInfo);
  nsresult Init();

public:
  

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMSVGAELEMENT
  NS_DECL_NSIDOMSVGURIREFERENCE

  
  NS_FORWARD_NSIDOMNODE(nsSVGAElementBase::)
  NS_FORWARD_NSIDOMELEMENT(nsSVGAElementBase::)
  NS_FORWARD_NSIDOMSVGELEMENT(nsSVGAElementBase::)

  
  virtual nsresult PreHandleEvent(nsEventChainPreVisitor& aVisitor);
  virtual nsresult PostHandleEvent(nsEventChainPostVisitor& aVisitor);
  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  
  NS_IMETHOD GetLinkState(nsLinkState &aState);
  NS_IMETHOD SetLinkState(nsLinkState aState);
  NS_IMETHOD GetHrefURI(nsIURI** aURI);
  NS_IMETHOD LinkAdded() { return NS_OK; }
  NS_IMETHOD LinkRemoved() { return NS_OK; }

  
  virtual PRBool IsFocusable(PRInt32 *aTabIndex = nsnull);
  virtual PRBool IsLink(nsIURI** aURI) const;
  virtual void GetLinkTarget(nsAString& aTarget);

protected:

  nsCOMPtr<nsIDOMSVGAnimatedString> mHref;
  nsCOMPtr<nsIDOMSVGAnimatedString> mTarget;

  
  nsLinkState mLinkState;
};

NS_IMPL_NS_NEW_SVG_ELEMENT(A)





NS_IMPL_ADDREF_INHERITED(nsSVGAElement, nsSVGAElementBase)
NS_IMPL_RELEASE_INHERITED(nsSVGAElement, nsSVGAElementBase)

NS_INTERFACE_MAP_BEGIN(nsSVGAElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMNode)
  NS_INTERFACE_MAP_ENTRY(nsIDOMElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGAElement)
  NS_INTERFACE_MAP_ENTRY(nsILink)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(SVGAElement)
NS_INTERFACE_MAP_END_INHERITING(nsSVGAElementBase)





nsSVGAElement::nsSVGAElement(nsINodeInfo *aNodeInfo)
  : nsSVGAElementBase(aNodeInfo),
    mLinkState(eLinkState_Unknown)
{
}

nsresult
nsSVGAElement::Init()
{
  nsresult rv = nsSVGAElementBase::Init();
  NS_ENSURE_SUCCESS(rv,rv);

  

  
  
  {
    rv = NS_NewSVGAnimatedString(getter_AddRefs(mHref));
    NS_ENSURE_SUCCESS(rv, rv);
    rv = AddMappedSVGValue(nsGkAtoms::href, mHref, kNameSpaceID_XLink);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  

  
  {
    rv = NS_NewSVGAnimatedString(getter_AddRefs(mTarget));
    NS_ENSURE_SUCCESS(rv, rv);
    rv = AddMappedSVGValue(nsGkAtoms::target, mTarget);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}






NS_IMETHODIMP
nsSVGAElement::GetHref(nsIDOMSVGAnimatedString * *aHref)
{
  *aHref = mHref;
  NS_IF_ADDREF(*aHref);
  return NS_OK;
}





nsresult
nsSVGAElement::PreHandleEvent(nsEventChainPreVisitor& aVisitor)
{
  nsresult rv = nsGenericElement::PreHandleEvent(aVisitor);
  NS_ENSURE_SUCCESS(rv, rv);

  return PreHandleEventForLinks(aVisitor);
}

nsresult
nsSVGAElement::PostHandleEvent(nsEventChainPostVisitor& aVisitor)
{
  return PostHandleEventForLinks(aVisitor);
}

NS_IMPL_ELEMENT_CLONE_WITH_INIT(nsSVGAElement)






NS_IMETHODIMP
nsSVGAElement::GetTarget(nsIDOMSVGAnimatedString * *aTarget)
{
  *aTarget = mTarget;
  NS_IF_ADDREF(*aTarget);
  return NS_OK;
}





NS_IMETHODIMP
nsSVGAElement::GetLinkState(nsLinkState &aState)
{
  aState = mLinkState;
  return NS_OK;
}

NS_IMETHODIMP
nsSVGAElement::SetLinkState(nsLinkState aState)
{
  mLinkState = aState;
  return NS_OK;
}

NS_IMETHODIMP
nsSVGAElement::GetHrefURI(nsIURI** aURI)
{
  *aURI = nsnull;
  return NS_OK; 
}





PRBool
nsSVGAElement::IsFocusable(PRInt32 *aTabIndex)
{
  nsCOMPtr<nsIURI> uri;
  if (IsLink(getter_AddRefs(uri))) {
    if (aTabIndex) {
      *aTabIndex = ((sTabFocusModel & eTabFocus_linksMask) == 0 ? -1 : 0);
    }
    return PR_TRUE;
  }

  if (aTabIndex) {
    *aTabIndex = -1;
  }

  return PR_FALSE;
}

PRBool
nsSVGAElement::IsLink(nsIURI** aURI) const
{
  
  
  
  
  
  
  
  
  

  static nsIContent::AttrValuesArray sTypeVals[] =
    { &nsGkAtoms::_empty, &nsGkAtoms::simple, nsnull };

  static nsIContent::AttrValuesArray sShowVals[] =
    { &nsGkAtoms::_empty, &nsGkAtoms::_new, &nsGkAtoms::replace, nsnull };

  static nsIContent::AttrValuesArray sActuateVals[] =
    { &nsGkAtoms::_empty, &nsGkAtoms::onRequest, nsnull };

  
  const nsAttrValue* href = mAttrsAndChildren.GetAttr(nsGkAtoms::href,
                                                      kNameSpaceID_XLink);
  if (href &&
      FindAttrValueIn(kNameSpaceID_XLink, nsGkAtoms::type,
                      sTypeVals, eCaseMatters) !=
                      nsIContent::ATTR_VALUE_NO_MATCH &&
      FindAttrValueIn(kNameSpaceID_XLink, nsGkAtoms::show,
                      sShowVals, eCaseMatters) !=
                      nsIContent::ATTR_VALUE_NO_MATCH &&
      FindAttrValueIn(kNameSpaceID_XLink, nsGkAtoms::actuate,
                      sActuateVals, eCaseMatters) !=
                      nsIContent::ATTR_VALUE_NO_MATCH) {
    nsCOMPtr<nsIURI> baseURI = GetBaseURI();
    
    
    
    
    nsAutoString hrefStr;
    href->ToString(hrefStr);
    nsContentUtils::NewURIWithDocumentCharset(aURI, hrefStr,
                                              GetOwnerDoc(), baseURI);
    
    return !!*aURI;
  }

  *aURI = nsnull;
  return PR_FALSE;
}

void
nsSVGAElement::GetLinkTarget(nsAString& aTarget)
{
  GetAttr(kNameSpaceID_None, nsGkAtoms::target, aTarget);
  if (aTarget.IsEmpty()) {
    nsIDocument* ownerDoc = GetOwnerDoc();
    if (ownerDoc) {
      ownerDoc->GetBaseTarget(aTarget);
    }
  }
}
