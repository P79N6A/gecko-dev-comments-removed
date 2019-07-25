




#include "mozilla/Util.h"

#include "nsSVGUseElement.h"
#include "nsIDOMSVGGElement.h"
#include "nsGkAtoms.h"
#include "nsSVGSVGElement.h"
#include "nsIDOMSVGSymbolElement.h"
#include "nsIDocument.h"
#include "nsIPresShell.h"
#include "mozilla/dom/Element.h"
#include "nsContentUtils.h"

using namespace mozilla;
using namespace mozilla::dom;




nsSVGElement::LengthInfo nsSVGUseElement::sLengthInfo[4] =
{
  { &nsGkAtoms::x, 0, nsIDOMSVGLength::SVG_LENGTHTYPE_NUMBER, nsSVGUtils::X },
  { &nsGkAtoms::y, 0, nsIDOMSVGLength::SVG_LENGTHTYPE_NUMBER, nsSVGUtils::Y },
  { &nsGkAtoms::width, 0, nsIDOMSVGLength::SVG_LENGTHTYPE_NUMBER, nsSVGUtils::X },
  { &nsGkAtoms::height, 0, nsIDOMSVGLength::SVG_LENGTHTYPE_NUMBER, nsSVGUtils::Y },
};

nsSVGElement::StringInfo nsSVGUseElement::sStringInfo[1] =
{
  { &nsGkAtoms::href, kNameSpaceID_XLink, true }
};

NS_IMPL_NS_NEW_SVG_ELEMENT(Use)




NS_IMPL_CYCLE_COLLECTION_CLASS(nsSVGUseElement)
NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(nsSVGUseElement,
                                                nsSVGUseElementBase)
  nsAutoScriptBlocker scriptBlocker;
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mOriginal)
  tmp->DestroyAnonymousContent();
  tmp->UnlinkSource();
NS_IMPL_CYCLE_COLLECTION_UNLINK_END
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(nsSVGUseElement,
                                                  nsSVGUseElementBase)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mOriginal)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mClone)
  tmp->mSource.Traverse(&cb);
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_ADDREF_INHERITED(nsSVGUseElement,nsSVGUseElementBase)
NS_IMPL_RELEASE_INHERITED(nsSVGUseElement,nsSVGUseElementBase)

DOMCI_NODE_DATA(SVGUseElement, nsSVGUseElement)

NS_INTERFACE_TABLE_HEAD_CYCLE_COLLECTION_INHERITED(nsSVGUseElement)
  NS_NODE_INTERFACE_TABLE7(nsSVGUseElement, nsIDOMNode, nsIDOMElement,
                           nsIDOMSVGElement, nsIDOMSVGTests,
                           nsIDOMSVGURIReference,
                           nsIDOMSVGUseElement, nsIMutationObserver)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(SVGUseElement)
  if (aIID.Equals(NS_GET_IID(nsSVGUseElement)))
    foundInterface = reinterpret_cast<nsISupports*>(this);
  else
NS_INTERFACE_MAP_END_INHERITING(nsSVGUseElementBase)




#ifdef _MSC_VER



#pragma warning(push)
#pragma warning(disable:4355)
#endif
nsSVGUseElement::nsSVGUseElement(already_AddRefed<nsINodeInfo> aNodeInfo)
  : nsSVGUseElementBase(aNodeInfo), mSource(this)
#ifdef _MSC_VER
#pragma warning(pop)
#endif
{
}

nsSVGUseElement::~nsSVGUseElement()
{
  UnlinkSource();
}




nsresult
nsSVGUseElement::Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const
{
  *aResult = nullptr;
  nsCOMPtr<nsINodeInfo> ni = aNodeInfo;
  nsSVGUseElement *it = new nsSVGUseElement(ni.forget());
  if (!it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  nsCOMPtr<nsINode> kungFuDeathGrip(it);
  nsresult rv1 = it->Init();
  nsresult rv2 = const_cast<nsSVGUseElement*>(this)->CopyInnerTo(it);

  
  it->mOriginal = const_cast<nsSVGUseElement*>(this);

  if (NS_SUCCEEDED(rv1) && NS_SUCCEEDED(rv2)) {
    kungFuDeathGrip.swap(*aResult);
  }

  return NS_FAILED(rv1) ? rv1 : rv2;
}





  NS_IMETHODIMP nsSVGUseElement::GetHref(nsIDOMSVGAnimatedString * *aHref)
{
  return mStringAttributes[HREF].ToDOMAnimatedString(aHref, this);
}





NS_IMETHODIMP nsSVGUseElement::GetX(nsIDOMSVGAnimatedLength * *aX)
{
  return mLengthAttributes[X].ToDOMAnimatedLength(aX, this);
}


NS_IMETHODIMP nsSVGUseElement::GetY(nsIDOMSVGAnimatedLength * *aY)
{
  return mLengthAttributes[Y].ToDOMAnimatedLength(aY, this);
}


NS_IMETHODIMP nsSVGUseElement::GetWidth(nsIDOMSVGAnimatedLength * *aWidth)
{
  return mLengthAttributes[WIDTH].ToDOMAnimatedLength(aWidth, this);
}


NS_IMETHODIMP nsSVGUseElement::GetHeight(nsIDOMSVGAnimatedLength * *aHeight)
{
  return mLengthAttributes[HEIGHT].ToDOMAnimatedLength(aHeight, this);
}




void
nsSVGUseElement::CharacterDataChanged(nsIDocument *aDocument,
                                      nsIContent *aContent,
                                      CharacterDataChangeInfo* aInfo)
{
  if (nsContentUtils::IsInSameAnonymousTree(this, aContent)) {
    TriggerReclone();
  }
}

void
nsSVGUseElement::AttributeChanged(nsIDocument* aDocument,
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
nsSVGUseElement::ContentAppended(nsIDocument *aDocument,
                                 nsIContent *aContainer,
                                 nsIContent *aFirstNewContent,
                                 int32_t aNewIndexInContainer)
{
  if (nsContentUtils::IsInSameAnonymousTree(this, aContainer)) {
    TriggerReclone();
  }
}

void
nsSVGUseElement::ContentInserted(nsIDocument *aDocument,
                                 nsIContent *aContainer,
                                 nsIContent *aChild,
                                 int32_t aIndexInContainer)
{
  if (nsContentUtils::IsInSameAnonymousTree(this, aChild)) {
    TriggerReclone();
  }
}

void
nsSVGUseElement::ContentRemoved(nsIDocument *aDocument,
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
nsSVGUseElement::NodeWillBeDestroyed(const nsINode *aNode)
{
  nsCOMPtr<nsIMutationObserver> kungFuDeathGrip(this);
  UnlinkSource();
}



nsIContent*
nsSVGUseElement::CreateAnonymousContent()
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
        nsRefPtr<nsSVGUseElement> useImpl;
        useElement->QueryInterface(NS_GET_IID(nsSVGUseElement),
                                   getter_AddRefs(useImpl));

        if (useImpl && useImpl->mOriginal == mOriginal)
          return nullptr;
      }
    }
  }

  nsCOMPtr<nsIDOMNode> newnode;
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

    if (mLengthAttributes[WIDTH].IsExplicitlySet())
      newElement->SetLength(nsGkAtoms::width, mLengthAttributes[WIDTH]);
    if (mLengthAttributes[HEIGHT].IsExplicitlySet())
      newElement->SetLength(nsGkAtoms::height, mLengthAttributes[HEIGHT]);
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
nsSVGUseElement::DestroyAnonymousContent()
{
  nsContentUtils::DestroyAnonymousContent(&mClone);
}

bool
nsSVGUseElement::OurWidthAndHeightAreUsed() const
{
  if (mClone) {
    nsCOMPtr<nsIDOMSVGSVGElement>    svg    = do_QueryInterface(mClone);
    nsCOMPtr<nsIDOMSVGSymbolElement> symbol = do_QueryInterface(mClone);
    return svg || symbol;
  }
  return false;
}




void
nsSVGUseElement::SyncWidthOrHeight(nsIAtom* aName)
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
    uint32_t index = *sLengthInfo[WIDTH].mName == aName ? WIDTH : HEIGHT;

    if (mLengthAttributes[index].IsExplicitlySet()) {
      target->SetLength(aName, mLengthAttributes[index]);
      return;
    }
    if (svg) {
      
      
      
      TriggerReclone();
      return;
    }
    
    
    nsSVGLength2 length;
    length.Init(nsSVGUtils::XY, 0xff,
                100, nsIDOMSVGLength::SVG_LENGTHTYPE_PERCENTAGE);
    target->SetLength(aName, length);
    return;
  }
}

void
nsSVGUseElement::LookupHref()
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
nsSVGUseElement::TriggerReclone()
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
nsSVGUseElement::UnlinkSource()
{
  if (mSource.get()) {
    mSource.get()->RemoveMutationObserver(this);
  }
  mSource.Unlink();
}




 gfxMatrix
nsSVGUseElement::PrependLocalTransformsTo(const gfxMatrix &aMatrix,
                                          TransformTypes aWhich) const
{
  NS_ABORT_IF_FALSE(aWhich != eChildToUserSpace || aMatrix.IsIdentity(),
                    "Skipping eUserSpaceToParent transforms makes no sense");

  
  gfxMatrix fromUserSpace =
    nsSVGUseElementBase::PrependLocalTransformsTo(aMatrix, aWhich);
  if (aWhich == eUserSpaceToParent) {
    return fromUserSpace;
  }
  
  float x, y;
  const_cast<nsSVGUseElement*>(this)->GetAnimatedLengthValues(&x, &y, nullptr);
  gfxMatrix toUserSpace = gfxMatrix().Translate(gfxPoint(x, y));
  if (aWhich == eChildToUserSpace) {
    return toUserSpace;
  }
  NS_ABORT_IF_FALSE(aWhich == eAllTransforms, "Unknown TransformTypes");
  return toUserSpace * fromUserSpace;
}

 bool
nsSVGUseElement::HasValidDimensions() const
{
  return (!mLengthAttributes[WIDTH].IsExplicitlySet() ||
           mLengthAttributes[WIDTH].GetAnimValInSpecifiedUnits() > 0) &&
         (!mLengthAttributes[HEIGHT].IsExplicitlySet() || 
           mLengthAttributes[HEIGHT].GetAnimValInSpecifiedUnits() > 0);
}

nsSVGElement::LengthAttributesInfo
nsSVGUseElement::GetLengthInfo()
{
  return LengthAttributesInfo(mLengthAttributes, sLengthInfo,
                              ArrayLength(sLengthInfo));
}

nsSVGElement::StringAttributesInfo
nsSVGUseElement::GetStringInfo()
{
  return StringAttributesInfo(mStringAttributes, sStringInfo,
                              ArrayLength(sStringInfo));
}




NS_IMETHODIMP_(bool)
nsSVGUseElement::IsAttributeMapped(const nsIAtom* name) const
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
    nsSVGUseElementBase::IsAttributeMapped(name);
}

