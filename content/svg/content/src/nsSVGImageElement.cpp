





































#include "nsGkAtoms.h"
#include "nsSVGLength.h"
#include "nsSVGAnimatedString.h"
#include "nsCOMPtr.h"
#include "nsIURI.h"
#include "nsNetUtil.h"
#include "nsSVGAnimatedPreserveAspectRatio.h"
#include "nsSVGPreserveAspectRatio.h"
#include "imgIContainer.h"
#include "imgIDecoderObserver.h"
#include "nsSVGPathGeometryElement.h"
#include "nsIDOMSVGImageElement.h"
#include "nsIDOMSVGURIReference.h"
#include "nsImageLoadingContent.h"
#include "nsSVGLength2.h"
#include "gfxContext.h"

class nsIDOMSVGAnimatedString;
class nsIDOMSVGAnimatedPreserveAspectRatio;

typedef nsSVGPathGeometryElement nsSVGImageElementBase;

class nsSVGImageElement : public nsSVGImageElementBase,
                          public nsIDOMSVGImageElement,
                          public nsIDOMSVGURIReference,
                          public nsImageLoadingContent
{
protected:
  friend nsresult NS_NewSVGImageElement(nsIContent **aResult,
                                        nsINodeInfo *aNodeInfo);
  nsSVGImageElement(nsINodeInfo *aNodeInfo);
  virtual ~nsSVGImageElement();
  nsresult Init();

public:
  
  
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMSVGIMAGEELEMENT
  NS_DECL_NSIDOMSVGURIREFERENCE

  
  NS_FORWARD_NSIDOMNODE(nsSVGImageElementBase::)
  NS_FORWARD_NSIDOMELEMENT(nsSVGImageElementBase::)
  NS_FORWARD_NSIDOMSVGELEMENT(nsSVGImageElementBase::)

  
  NS_IMETHOD DidModifySVGObservable(nsISVGValue *observable,
                                    nsISVGValue::modificationType aModType);

  
  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              PRBool aCompileEventHandlers);

  virtual PRInt32 IntrinsicState() const;

  NS_IMETHODIMP_(PRBool) IsAttributeMapped(const nsIAtom* name) const;

  
  virtual void ConstructPath(gfxContext *aCtx);

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

protected:

  virtual LengthAttributesInfo GetLengthInfo();

  void GetSrc(nsAString& src);

  enum { X, Y, WIDTH, HEIGHT };
  nsSVGLength2 mLengthAttributes[4];
  static LengthInfo sLengthInfo[4];
  
  nsCOMPtr<nsIDOMSVGAnimatedString> mHref;
  nsCOMPtr<nsIDOMSVGAnimatedPreserveAspectRatio> mPreserveAspectRatio;
};

nsSVGElement::LengthInfo nsSVGImageElement::sLengthInfo[4] =
{
  { &nsGkAtoms::x, 0, nsIDOMSVGLength::SVG_LENGTHTYPE_NUMBER, nsSVGUtils::X },
  { &nsGkAtoms::y, 0, nsIDOMSVGLength::SVG_LENGTHTYPE_NUMBER, nsSVGUtils::Y },
  { &nsGkAtoms::width, 0, nsIDOMSVGLength::SVG_LENGTHTYPE_NUMBER, nsSVGUtils::X },
  { &nsGkAtoms::height, 0, nsIDOMSVGLength::SVG_LENGTHTYPE_NUMBER, nsSVGUtils::Y },
};

NS_IMPL_NS_NEW_SVG_ELEMENT(Image)




NS_IMPL_ADDREF_INHERITED(nsSVGImageElement,nsSVGImageElementBase)
NS_IMPL_RELEASE_INHERITED(nsSVGImageElement,nsSVGImageElementBase)

NS_INTERFACE_MAP_BEGIN(nsSVGImageElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMNode)
  NS_INTERFACE_MAP_ENTRY(nsIDOMElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGImageElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGURIReference)
  NS_INTERFACE_MAP_ENTRY(imgIDecoderObserver)
  NS_INTERFACE_MAP_ENTRY(nsIImageLoadingContent)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(SVGImageElement)
NS_INTERFACE_MAP_END_INHERITING(nsSVGImageElementBase)




nsSVGImageElement::nsSVGImageElement(nsINodeInfo *aNodeInfo)
  : nsSVGImageElementBase(aNodeInfo)
{
}

nsSVGImageElement::~nsSVGImageElement()
{
  DestroyImageLoadingContent();
}

nsresult
nsSVGImageElement::Init()
{
  nsresult rv = nsSVGImageElementBase::Init();
  NS_ENSURE_SUCCESS(rv,rv);

  

  

  
  
  {
    rv = NS_NewSVGAnimatedString(getter_AddRefs(mHref));
    NS_ENSURE_SUCCESS(rv,rv);
    rv = AddMappedSVGValue(nsGkAtoms::href, mHref, kNameSpaceID_XLink);
    NS_ENSURE_SUCCESS(rv,rv);
  }

  
  {
    nsCOMPtr<nsIDOMSVGPreserveAspectRatio> preserveAspectRatio;
    rv = NS_NewSVGPreserveAspectRatio(getter_AddRefs(preserveAspectRatio));
    NS_ENSURE_SUCCESS(rv,rv);
    rv = NS_NewSVGAnimatedPreserveAspectRatio(
                                          getter_AddRefs(mPreserveAspectRatio),
                                          preserveAspectRatio);
    NS_ENSURE_SUCCESS(rv,rv);
    rv = AddMappedSVGValue(nsGkAtoms::preserveAspectRatio,
                           mPreserveAspectRatio);
    NS_ENSURE_SUCCESS(rv,rv);
  }

  return rv;
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
  *aPreserveAspectRatio = mPreserveAspectRatio;
  NS_IF_ADDREF(*aPreserveAspectRatio);
  return NS_OK;
}





NS_IMETHODIMP
nsSVGImageElement::GetHref(nsIDOMSVGAnimatedString * *aHref)
{
  *aHref = mHref;
  NS_IF_ADDREF(*aHref);
  return NS_OK;
}




nsSVGElement::LengthAttributesInfo
nsSVGImageElement::GetLengthInfo()
{
  return LengthAttributesInfo(mLengthAttributes, sLengthInfo,
                              NS_ARRAY_LENGTH(sLengthInfo));
}



void nsSVGImageElement::GetSrc(nsAString& src)
{
  

  nsCOMPtr<nsIURI> baseURI = GetBaseURI();

  nsAutoString relURIStr;
  mHref->GetAnimVal(relURIStr);
  relURIStr.Trim(" \t\n\r");

  if (baseURI && !relURIStr.IsEmpty()) 
    NS_MakeAbsoluteURI(src, relURIStr, baseURI);
  else
    src = relURIStr;
}




NS_IMETHODIMP
nsSVGImageElement::DidModifySVGObservable(nsISVGValue* aObservable,
                                          nsISVGValue::modificationType aModType)
{
  nsCOMPtr<nsIDOMSVGAnimatedString> s = do_QueryInterface(aObservable);

  if (s && mHref == s) {
    nsAutoString href;
    GetSrc(href);

#ifdef DEBUG_tor
    fprintf(stderr, "nsSVGImageElement - URI <%s>\n", ToNewCString(href));
#endif

    
    
    if (nsContentUtils::GetBoolPref("dom.disable_image_src_set") &&
        !nsContentUtils::IsCallerChrome()) {
      return NS_OK;
    }

    LoadImage(href, PR_TRUE, PR_TRUE);
  }

  return nsSVGImageElementBase::DidModifySVGObservable(aObservable, aModType);
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

  
  
  nsAutoString href;
  if (GetAttr(kNameSpaceID_XLink, nsGkAtoms::href, href)) {
    
    
    LoadImage(href, PR_FALSE, PR_FALSE);
  }

  return rv;
}

PRInt32
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

  if (width == 0 || height == 0)
    return;

  aCtx->Rectangle(gfxRect(x, y, width, height));
}
