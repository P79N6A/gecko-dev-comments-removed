




#include "mozilla/Util.h"

#include "mozilla/dom/SVGAnimatedLength.h"
#include "mozilla/dom/SVGUseElement.h"
#include "mozilla/dom/SVGUseElementBinding.h"
#include "nsIDOMSVGGElement.h"
#include "nsGkAtoms.h"
#include "mozilla/dom/SVGSVGElement.h"
#include "nsIDOMSVGSymbolElement.h"
#include "nsIDocument.h"
#include "nsIPresShell.h"
#include "mozilla/dom/Element.h"
#include "nsContentUtils.h"

DOMCI_NODE_DATA(SVGUseElement, mozilla::dom::SVGUseElement)

NS_IMPL_NS_NEW_NAMESPACED_SVG_ELEMENT(Use)

namespace mozilla {
namespace dom {

JSObject*
SVGUseElement::WrapNode(JSContext *aCx, JSObject *aScope, bool *aTriedToWrap)
{
  return SVGUseElementBinding::Wrap(aCx, aScope, this, aTriedToWrap);
}




nsSVGElement::LengthInfo SVGUseElement::sLengthInfo[4] =
{
  { &nsGkAtoms::x, 0, nsIDOMSVGLength::SVG_LENGTHTYPE_NUMBER, SVGContentUtils::X },
  { &nsGkAtoms::y, 0, nsIDOMSVGLength::SVG_LENGTHTYPE_NUMBER, SVGContentUtils::Y },
  { &nsGkAtoms::width, 0, nsIDOMSVGLength::SVG_LENGTHTYPE_NUMBER, SVGContentUtils::X },
  { &nsGkAtoms::height, 0, nsIDOMSVGLength::SVG_LENGTHTYPE_NUMBER, SVGContentUtils::Y },
};

nsSVGElement::StringInfo SVGUseElement::sStringInfo[1] =
{
  { &nsGkAtoms::href, kNameSpaceID_XLink, true }
};




NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(SVGUseElement,
                                                SVGUseElementBase)
  nsAutoScriptBlocker scriptBlocker;
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mOriginal)
  tmp->DestroyAnonymousContent();
  tmp->UnlinkSource();
NS_IMPL_CYCLE_COLLECTION_UNLINK_END
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(SVGUseElement,
                                                  SVGUseElementBase)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mOriginal)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mClone)
  tmp->mSource.Traverse(&cb);
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_ADDREF_INHERITED(SVGUseElement,SVGUseElementBase)
NS_IMPL_RELEASE_INHERITED(SVGUseElement,SVGUseElementBase)

NS_INTERFACE_TABLE_HEAD_CYCLE_COLLECTION_INHERITED(SVGUseElement)
  NS_NODE_INTERFACE_TABLE6(SVGUseElement, nsIDOMNode, nsIDOMElement,
                           nsIDOMSVGElement,
                           nsIDOMSVGURIReference,
                           nsIDOMSVGUseElement, nsIMutationObserver)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(SVGUseElement)
  if (aIID.Equals(NS_GET_IID(mozilla::dom::SVGUseElement)))
    foundInterface = reinterpret_cast<nsISupports*>(this);
  else
NS_INTERFACE_MAP_END_INHERITING(SVGUseElementBase)




#ifdef _MSC_VER



#pragma warning(push)
#pragma warning(disable:4355)
#endif
SVGUseElement::SVGUseElement(already_AddRefed<nsINodeInfo> aNodeInfo)
  : SVGUseElementBase(aNodeInfo), mSource(this)
#ifdef _MSC_VER
#pragma warning(pop)
#endif
{
  SetIsDOMBinding();
}

SVGUseElement::~SVGUseElement()
{
  UnlinkSource();
}




nsresult
SVGUseElement::Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const
{
  *aResult = nullptr;
  nsCOMPtr<nsINodeInfo> ni = aNodeInfo;
  SVGUseElement *it = new SVGUseElement(ni.forget());
  if (!it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  nsCOMPtr<nsINode> kungFuDeathGrip(it);
  nsresult rv1 = it->Init();
  nsresult rv2 = const_cast<SVGUseElement*>(this)->CopyInnerTo(it);

  
  it->mOriginal = const_cast<SVGUseElement*>(this);

  if (NS_SUCCEEDED(rv1) && NS_SUCCEEDED(rv2)) {
    kungFuDeathGrip.swap(*aResult);
  }

  return NS_FAILED(rv1) ? rv1 : rv2;
}





  NS_IMETHODIMP SVGUseElement::GetHref(nsIDOMSVGAnimatedString * *aHref)
{
  *aHref = Href().get();
  return NS_OK;
}

already_AddRefed<nsIDOMSVGAnimatedString>
SVGUseElement::Href()
{
  nsCOMPtr<nsIDOMSVGAnimatedString> href;
  mStringAttributes[HREF].ToDOMAnimatedString(getter_AddRefs(href), this);
  return href.forget();
}





NS_IMETHODIMP SVGUseElement::GetX(nsIDOMSVGAnimatedLength * *aX)
{
  *aX = X().get();
  return NS_OK;
}

already_AddRefed<SVGAnimatedLength>
SVGUseElement::X()
{
  return mLengthAttributes[ATTR_X].ToDOMAnimatedLength(this);
}


NS_IMETHODIMP SVGUseElement::GetY(nsIDOMSVGAnimatedLength * *aY)
{
  *aY = Y().get();
  return NS_OK;
}

already_AddRefed<SVGAnimatedLength>
SVGUseElement::Y()
{
  return mLengthAttributes[ATTR_Y].ToDOMAnimatedLength(this);
}


NS_IMETHODIMP SVGUseElement::GetWidth(nsIDOMSVGAnimatedLength * *aWidth)
{
  *aWidth = Width().get();
  return NS_OK;
}

already_AddRefed<SVGAnimatedLength>
SVGUseElement::Width()
{
  return mLengthAttributes[ATTR_WIDTH].ToDOMAnimatedLength(this);
}


NS_IMETHODIMP SVGUseElement::GetHeight(nsIDOMSVGAnimatedLength * *aHeight)
{
  *aHeight = Height().get();
  return NS_OK;
}

already_AddRefed<SVGAnimatedLength>
SVGUseElement::Height()
{
  return mLengthAttributes[ATTR_HEIGHT].ToDOMAnimatedLength(this);
}




void
SVGUseElement::CharacterDataChanged(nsIDocument *aDocument,
                                    nsIContent *aContent,
                                    CharacterDataChangeInfo* aInfo)
{
  if (nsContentUtils::IsInSameAnonymousTree(this, aContent)) {
    TriggerReclone();
  }
}

void
SVGUseElement::AttributeChanged(nsIDocument* aDocument,
                                Element* aElement,
                                int32_t aNameSpaceID,
                                nsIAtom* aAttribute,
                                int32_t aModType)
{
  if (nsContentUtils::IsInSameAnonymousTree(this, aElement)) {
    TriggerReclone();
  }
}

void
SVGUseElement::ContentAppended(nsIDocument *aDocument,
                               nsIContent *aContainer,
                               nsIContent *aFirstNewContent,
                               int32_t aNewIndexInContainer)
{
  if (nsContentUtils::IsInSameAnonymousTree(this, aContainer)) {
    TriggerReclone();
  }
}

void
SVGUseElement::ContentInserted(nsIDocument *aDocument,
                               nsIContent *aContainer,
                               nsIContent *aChild,
                               int32_t aIndexInContainer)
{
  if (nsContentUtils::IsInSameAnonymousTree(this, aChild)) {
    TriggerReclone();
  }
}

void
SVGUseElement::ContentRemoved(nsIDocument *aDocument,
                              nsIContent *aContainer,
                              nsIContent *aChild,
                              int32_t aIndexInContainer,
                              nsIContent *aPreviousSibling)
{
  if (nsContentUtils::IsInSameAnonymousTree(this, aChild)) {
    TriggerReclone();
  }
}

void
SVGUseElement::NodeWillBeDestroyed(const nsINode *aNode)
{
  nsCOMPtr<nsIMutationObserver> kungFuDeathGrip(this);
  UnlinkSource();
}



nsIContent*
SVGUseElement::CreateAnonymousContent()
{
  mClone = nullptr;

  if (mSource.get()) {
    mSource.get()->RemoveMutationObserver(this);
  }

  LookupHref();
  nsIContent* targetContent = mSource.get();
  if (!targetContent || !targetContent->IsSVG())
    return nullptr;

  
  
  nsIAtom *tag = targetContent->Tag();
  if (tag != nsGkAtoms::svg &&
      tag != nsGkAtoms::symbol &&
      tag != nsGkAtoms::g &&
      tag != nsGkAtoms::path &&
      tag != nsGkAtoms::text &&
      tag != nsGkAtoms::rect &&
      tag != nsGkAtoms::circle &&
      tag != nsGkAtoms::ellipse &&
      tag != nsGkAtoms::line &&
      tag != nsGkAtoms::polyline &&
      tag != nsGkAtoms::polygon &&
      tag != nsGkAtoms::image &&
      tag != nsGkAtoms::use)
    return nullptr;

  

  
  if (nsContentUtils::ContentIsDescendantOf(this, targetContent))
    return nullptr;

  
  if (GetParent() && mOriginal) {
    for (nsCOMPtr<nsIContent> content = GetParent();
         content;
         content = content->GetParent()) {
      nsCOMPtr<nsIDOMSVGUseElement> useElement = do_QueryInterface(content);

      if (useElement) {
        nsRefPtr<SVGUseElement> useImpl;
        useElement->QueryInterface(NS_GET_IID(mozilla::dom::SVGUseElement),
                                   getter_AddRefs(useImpl));

        if (useImpl && useImpl->mOriginal == mOriginal)
          return nullptr;
      }
    }
  }

  nsCOMPtr<nsINode> newnode;
  nsCOMArray<nsINode> unused;
  nsNodeInfoManager* nodeInfoManager =
    targetContent->OwnerDoc() == OwnerDoc() ?
      nullptr : OwnerDoc()->NodeInfoManager();
  nsNodeUtils::Clone(targetContent, true, nodeInfoManager, unused,
                     getter_AddRefs(newnode));

  nsCOMPtr<nsIContent> newcontent = do_QueryInterface(newnode);

  if (!newcontent)
    return nullptr;

  nsCOMPtr<nsIDOMSVGSymbolElement> symbol     = do_QueryInterface(newcontent);
  nsCOMPtr<nsIDOMSVGSVGElement>    svg        = do_QueryInterface(newcontent);

  if (symbol) {
    nsIDocument *document = GetCurrentDoc();
    if (!document)
      return nullptr;

    nsNodeInfoManager *nodeInfoManager = document->NodeInfoManager();
    if (!nodeInfoManager)
      return nullptr;

    nsCOMPtr<nsINodeInfo> nodeInfo;
    nodeInfo = nodeInfoManager->GetNodeInfo(nsGkAtoms::svg, nullptr,
                                            kNameSpaceID_SVG,
                                            nsIDOMNode::ELEMENT_NODE);
    if (!nodeInfo)
      return nullptr;

    nsCOMPtr<nsIContent> svgNode;
    NS_NewSVGSVGElement(getter_AddRefs(svgNode), nodeInfo.forget(),
                        NOT_FROM_PARSER);

    if (!svgNode)
      return nullptr;
    
    
    const nsAttrName* name;
    uint32_t i;
    for (i = 0; (name = newcontent->GetAttrNameAt(i)); i++) {
      nsAutoString value;
      int32_t nsID = name->NamespaceID();
      nsIAtom* lname = name->LocalName();

      newcontent->GetAttr(nsID, lname, value);
      svgNode->SetAttr(nsID, lname, name->GetPrefix(), value, false);
    }

    
    uint32_t num = newcontent->GetChildCount();
    for (i = 0; i < num; i++) {
      nsCOMPtr<nsIContent> child = newcontent->GetFirstChild();
      newcontent->RemoveChildAt(0, false);
      svgNode->InsertChildAt(child, i, true);
    }

    newcontent = svgNode;
  }

  if (symbol || svg) {
    nsSVGElement *newElement = static_cast<nsSVGElement*>(newcontent.get());

    if (mLengthAttributes[ATTR_WIDTH].IsExplicitlySet())
      newElement->SetLength(nsGkAtoms::width, mLengthAttributes[ATTR_WIDTH]);
    if (mLengthAttributes[ATTR_HEIGHT].IsExplicitlySet())
      newElement->SetLength(nsGkAtoms::height, mLengthAttributes[ATTR_HEIGHT]);
  }

  
  nsCOMPtr<nsIURI> baseURI = targetContent->GetBaseURI();
  if (!baseURI)
    return nullptr;
  newcontent->SetExplicitBaseURI(baseURI);

  targetContent->AddMutationObserver(this);
  mClone = newcontent;
  return mClone;
}

void
SVGUseElement::DestroyAnonymousContent()
{
  nsContentUtils::DestroyAnonymousContent(&mClone);
}

bool
SVGUseElement::OurWidthAndHeightAreUsed() const
{
  if (mClone) {
    nsCOMPtr<nsIDOMSVGSVGElement>    svg    = do_QueryInterface(mClone);
    nsCOMPtr<nsIDOMSVGSymbolElement> symbol = do_QueryInterface(mClone);
    return svg || symbol;
  }
  return false;
}




void
SVGUseElement::SyncWidthOrHeight(nsIAtom* aName)
{
  NS_ASSERTION(aName == nsGkAtoms::width || aName == nsGkAtoms::height,
               "The clue is in the function name");
  NS_ASSERTION(OurWidthAndHeightAreUsed(), "Don't call this");

  if (!mClone) {
    return;
  }

  nsCOMPtr<nsIDOMSVGSymbolElement> symbol = do_QueryInterface(mClone);
  nsCOMPtr<nsIDOMSVGSVGElement>    svg    = do_QueryInterface(mClone);

  if (symbol || svg) {
    nsSVGElement *target = static_cast<nsSVGElement*>(mClone.get());
    uint32_t index = *sLengthInfo[ATTR_WIDTH].mName == aName ? ATTR_WIDTH : ATTR_HEIGHT;

    if (mLengthAttributes[index].IsExplicitlySet()) {
      target->SetLength(aName, mLengthAttributes[index]);
      return;
    }
    if (svg) {
      
      
      
      TriggerReclone();
      return;
    }
    
    
    nsSVGLength2 length;
    length.Init(SVGContentUtils::XY, 0xff,
                100, nsIDOMSVGLength::SVG_LENGTHTYPE_PERCENTAGE);
    target->SetLength(aName, length);
    return;
  }
}

void
SVGUseElement::LookupHref()
{
  nsAutoString href;
  mStringAttributes[HREF].GetAnimValue(href, this);
  if (href.IsEmpty())
    return;

  nsCOMPtr<nsIURI> targetURI;
  nsCOMPtr<nsIURI> baseURI = mOriginal ? mOriginal->GetBaseURI() : GetBaseURI();
  nsContentUtils::NewURIWithDocumentCharset(getter_AddRefs(targetURI), href,
                                            GetCurrentDoc(), baseURI);

  mSource.Reset(this, targetURI);
}

void
SVGUseElement::TriggerReclone()
{
  nsIDocument *doc = GetCurrentDoc();
  if (!doc)
    return;
  nsIPresShell *presShell = doc->GetShell();
  if (!presShell)
    return;
  presShell->PostRecreateFramesFor(this);
}

void
SVGUseElement::UnlinkSource()
{
  if (mSource.get()) {
    mSource.get()->RemoveMutationObserver(this);
  }
  mSource.Unlink();
}




 gfxMatrix
SVGUseElement::PrependLocalTransformsTo(const gfxMatrix &aMatrix,
                                        TransformTypes aWhich) const
{
  NS_ABORT_IF_FALSE(aWhich != eChildToUserSpace || aMatrix.IsIdentity(),
                    "Skipping eUserSpaceToParent transforms makes no sense");

  
  gfxMatrix fromUserSpace =
    SVGUseElementBase::PrependLocalTransformsTo(aMatrix, aWhich);
  if (aWhich == eUserSpaceToParent) {
    return fromUserSpace;
  }
  
  float x, y;
  const_cast<SVGUseElement*>(this)->GetAnimatedLengthValues(&x, &y, nullptr);
  gfxMatrix toUserSpace = gfxMatrix().Translate(gfxPoint(x, y));
  if (aWhich == eChildToUserSpace) {
    return toUserSpace;
  }
  NS_ABORT_IF_FALSE(aWhich == eAllTransforms, "Unknown TransformTypes");
  return toUserSpace * fromUserSpace;
}

 bool
SVGUseElement::HasValidDimensions() const
{
  return (!mLengthAttributes[ATTR_WIDTH].IsExplicitlySet() ||
           mLengthAttributes[ATTR_WIDTH].GetAnimValInSpecifiedUnits() > 0) &&
         (!mLengthAttributes[ATTR_HEIGHT].IsExplicitlySet() ||
           mLengthAttributes[ATTR_HEIGHT].GetAnimValInSpecifiedUnits() > 0);
}

nsSVGElement::LengthAttributesInfo
SVGUseElement::GetLengthInfo()
{
  return LengthAttributesInfo(mLengthAttributes, sLengthInfo,
                              ArrayLength(sLengthInfo));
}

nsSVGElement::StringAttributesInfo
SVGUseElement::GetStringInfo()
{
  return StringAttributesInfo(mStringAttributes, sStringInfo,
                              ArrayLength(sStringInfo));
}




NS_IMETHODIMP_(bool)
SVGUseElement::IsAttributeMapped(const nsIAtom* name) const
{
  static const MappedAttributeEntry* const map[] = {
    sFEFloodMap,
    sFiltersMap,
    sFontSpecificationMap,
    sGradientStopMap,
    sLightingEffectsMap,
    sMarkersMap,
    sTextContentElementsMap,
    sViewportsMap
  };

  return FindAttributeDependence(name, map) ||
    SVGUseElementBase::IsAttributeMapped(name);
}

} 
} 
