





































#include "nsSVGImageElement.h"
#include "nsCOMPtr.h"
#include "nsIURI.h"
#include "nsNetUtil.h"
#include "imgIContainer.h"
#include "imgIDecoderObserver.h"
#include "gfxContext.h"
#include "mozilla/Preferences.h"

using namespace mozilla;

nsSVGElement::LengthInfo nsSVGImageElement::sLengthInfo[4] =
{
  { &nsGkAtoms::x, 0, nsIDOMSVGLength::SVG_LENGTHTYPE_NUMBER, nsSVGUtils::X },
  { &nsGkAtoms::y, 0, nsIDOMSVGLength::SVG_LENGTHTYPE_NUMBER, nsSVGUtils::Y },
  { &nsGkAtoms::width, 0, nsIDOMSVGLength::SVG_LENGTHTYPE_NUMBER, nsSVGUtils::X },
  { &nsGkAtoms::height, 0, nsIDOMSVGLength::SVG_LENGTHTYPE_NUMBER, nsSVGUtils::Y },
};

nsSVGElement::StringInfo nsSVGImageElement::sStringInfo[1] =
{
  { &nsGkAtoms::href, kNameSpaceID_XLink, PR_TRUE }
};

NS_IMPL_NS_NEW_SVG_ELEMENT(Image)




NS_IMPL_ADDREF_INHERITED(nsSVGImageElement,nsSVGImageElementBase)
NS_IMPL_RELEASE_INHERITED(nsSVGImageElement,nsSVGImageElementBase)

DOMCI_NODE_DATA(SVGImageElement, nsSVGImageElement)

NS_INTERFACE_TABLE_HEAD(nsSVGImageElement)
  NS_NODE_INTERFACE_TABLE7(nsSVGImageElement, nsIDOMNode, nsIDOMElement,
                           nsIDOMSVGElement, nsIDOMSVGImageElement,
                           nsIDOMSVGURIReference, imgIDecoderObserver,
                           nsIImageLoadingContent)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(SVGImageElement)
NS_INTERFACE_MAP_END_INHERITING(nsSVGImageElementBase)




nsSVGImageElement::nsSVGImageElement(already_AddRefed<nsINodeInfo> aNodeInfo)
  : nsSVGImageElementBase(aNodeInfo)
{
  
  AddStatesSilently(NS_EVENT_STATE_BROKEN);
}

nsSVGImageElement::~nsSVGImageElement()
{
  DestroyImageLoadingContent();
}





NS_IMPL_ELEMENT_CLONE_WITH_INIT(nsSVGImageElement)






NS_IMETHODIMP nsSVGImageElement::GetX(nsIDOMSVGAnimatedLength * *aX)
{
  return mLengthAttributes[X].ToDOMAnimatedLength(aX, this);
}


NS_IMETHODIMP nsSVGImageElement::GetY(nsIDOMSVGAnimatedLength * *aY)
{
  return mLengthAttributes[Y].ToDOMAnimatedLength(aY, this);
}


NS_IMETHODIMP nsSVGImageElement::GetWidth(nsIDOMSVGAnimatedLength * *aWidth)
{
  return mLengthAttributes[WIDTH].ToDOMAnimatedLength(aWidth, this);
}


NS_IMETHODIMP nsSVGImageElement::GetHeight(nsIDOMSVGAnimatedLength * *aHeight)
{
  return mLengthAttributes[HEIGHT].ToDOMAnimatedLength(aHeight, this);
}


NS_IMETHODIMP
nsSVGImageElement::GetPreserveAspectRatio(nsIDOMSVGAnimatedPreserveAspectRatio
                                          **aPreserveAspectRatio)
{
  return mPreserveAspectRatio.ToDOMAnimatedPreserveAspectRatio(aPreserveAspectRatio, this);
}





NS_IMETHODIMP
nsSVGImageElement::GetHref(nsIDOMSVGAnimatedString * *aHref)
{
  return mStringAttributes[HREF].ToDOMAnimatedString(aHref, this);
}



nsresult
nsSVGImageElement::LoadSVGImage(PRBool aForce, PRBool aNotify)
{
  
  nsCOMPtr<nsIURI> baseURI = GetBaseURI();

  nsAutoString href;
  mStringAttributes[HREF].GetAnimValue(href, this);
  href.Trim(" \t\n\r");

  if (baseURI && !href.IsEmpty())
    NS_MakeAbsoluteURI(href, href, baseURI);

  return LoadImage(href, aForce, aNotify);
}




nsresult
nsSVGImageElement::AfterSetAttr(PRInt32 aNamespaceID, nsIAtom* aName,
                                const nsAString* aValue, PRBool aNotify)
{
  if (aNamespaceID == kNameSpaceID_XLink && aName == nsGkAtoms::href) {
    
    
    if (Preferences::GetBool("dom.disable_image_src_set") &&
        !nsContentUtils::IsCallerChrome()) {
      return NS_OK;
    }

    if (aValue) {
      LoadSVGImage(PR_TRUE, aNotify);
    } else {
      CancelImageRequests(aNotify);
    }
  }
  return nsSVGImageElementBase::AfterSetAttr(aNamespaceID, aName,
                                             aValue, aNotify);
}

void
nsSVGImageElement::MaybeLoadSVGImage()
{
  if (HasAttr(kNameSpaceID_XLink, nsGkAtoms::href) &&
      (NS_FAILED(LoadSVGImage(PR_FALSE, PR_TRUE)) ||
       !LoadingEnabled())) {
    CancelImageRequests(PR_TRUE);
  }
}

nsresult
nsSVGImageElement::BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              PRBool aCompileEventHandlers)
{
  nsresult rv = nsSVGImageElementBase::BindToTree(aDocument, aParent,
                                                  aBindingParent,
                                                  aCompileEventHandlers);
  NS_ENSURE_SUCCESS(rv, rv);

  if (HasAttr(kNameSpaceID_XLink, nsGkAtoms::href)) {
    
    
    ClearBrokenState();
    RemoveStatesSilently(NS_EVENT_STATE_BROKEN);
    nsContentUtils::AddScriptRunner(
      NS_NewRunnableMethod(this, &nsSVGImageElement::MaybeLoadSVGImage));
  }

  return rv;
}

nsEventStates
nsSVGImageElement::IntrinsicState() const
{
  return nsSVGImageElementBase::IntrinsicState() |
    nsImageLoadingContent::ImageState();
}

NS_IMETHODIMP_(PRBool)
nsSVGImageElement::IsAttributeMapped(const nsIAtom* name) const
{
  static const MappedAttributeEntry* const map[] = {
    sViewportsMap,
  };
  
  return FindAttributeDependence(name, map, NS_ARRAY_LENGTH(map)) ||
    nsSVGImageElementBase::IsAttributeMapped(name);
}






void
nsSVGImageElement::ConstructPath(gfxContext *aCtx)
{
  float x, y, width, height;

  GetAnimatedLengthValues(&x, &y, &width, &height, nsnull);

  if (width <= 0 || height <= 0)
    return;

  aCtx->Rectangle(gfxRect(x, y, width, height));
}




nsSVGElement::LengthAttributesInfo
nsSVGImageElement::GetLengthInfo()
{
  return LengthAttributesInfo(mLengthAttributes, sLengthInfo,
                              NS_ARRAY_LENGTH(sLengthInfo));
}

SVGAnimatedPreserveAspectRatio *
nsSVGImageElement::GetPreserveAspectRatio()
{
  return &mPreserveAspectRatio;
}

nsSVGElement::StringAttributesInfo
nsSVGImageElement::GetStringInfo()
{
  return StringAttributesInfo(mStringAttributes, sStringInfo,
                              NS_ARRAY_LENGTH(sStringInfo));
}

void
nsSVGImageElement::DidAnimateString(PRUint8 aAttrEnum)
{
  if (aAttrEnum == HREF) {
    LoadSVGImage(PR_TRUE, PR_FALSE);
    return;
  }

  nsSVGImageElementBase::DidAnimateString(aAttrEnum);
}

nsresult
nsSVGImageElement::CopyInnerTo(nsGenericElement* aDest) const
{
  if (aDest->GetOwnerDoc()->IsStaticDocument()) {
    CreateStaticImageClone(static_cast<nsSVGImageElement*>(aDest));
  }
  return nsSVGImageElementBase::CopyInnerTo(aDest);
}
