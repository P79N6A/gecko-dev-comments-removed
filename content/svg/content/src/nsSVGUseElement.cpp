



































#include "nsSVGUseElement.h"
#include "nsIDOMSVGGElement.h"
#include "nsGkAtoms.h"
#include "nsIDOMDocument.h"
#include "nsIDOMSVGSVGElement.h"
#include "nsIDOMSVGSymbolElement.h"
#include "nsIDocument.h"
#include "nsIPresShell.h"
#include "mozilla/dom/Element.h"

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
  { &nsGkAtoms::href, kNameSpaceID_XLink, PR_TRUE }
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
  NS_NODE_INTERFACE_TABLE6(nsSVGUseElement, nsIDOMNode, nsIDOMElement,
                           nsIDOMSVGElement, nsIDOMSVGURIReference,
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
  *aResult = nsnull;
  nsCOMPtr<nsINodeInfo> ni = aNodeInfo;
  nsSVGUseElement *it = new nsSVGUseElement(ni.forget());
  if (!it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  nsCOMPtr<nsINode> kungFuDeathGrip(it);
  nsresult rv = it->Init();
  rv |= CopyInnerTo(it);

  
  it->mOriginal = const_cast<nsSVGUseElement*>(this);

  if (NS_SUCCEEDED(rv)) {
    kungFuDeathGrip.swap(*aResult);
  }

  return rv;
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
                                  PRInt32 aNameSpaceID,
                                  nsIAtom* aAttribute,
                                  PRInt32 aModType)
{
  if (nsContentUtils::IsInSameAnonymousTree(this, aElement)) {
    TriggerReclone();
  }
}

void
nsSVGUseElement::ContentAppended(nsIDocument *aDocument,
                                 nsIContent *aContainer,
                                 nsIContent *aFirstNewContent,
                                 PRInt32 aNewIndexInContainer)
{
  if (nsContentUtils::IsInSameAnonymousTree(this, aContainer)) {
    TriggerReclone();
  }
}

void
nsSVGUseElement::ContentInserted(nsIDocument *aDocument,
                                 nsIContent *aContainer,
                                 nsIContent *aChild,
                                 PRInt32 aIndexInContainer)
{
  if (nsContentUtils::IsInSameAnonymousTree(this, aChild)) {
    TriggerReclone();
  }
}

void
nsSVGUseElement::ContentRemoved(nsIDocument *aDocument,
                                nsIContent *aContainer,
                                nsIContent *aChild,
                                PRInt32 aIndexInContainer,
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
  mClone = nsnull;

  if (mSource.get()) {
    mSource.get()->RemoveMutationObserver(this);
  }

  LookupHref();
  nsIContent* targetContent = mSource.get();
  if (!targetContent)
    return nsnull;

  
  
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
    return nsnull;

  
  if (!HasValidDimensions())
    return nsnull;

  

  
  if (nsContentUtils::ContentIsDescendantOf(this, targetContent))
    return nsnull;

  
  if (this->GetParent() && mOriginal) {
    for (nsCOMPtr<nsIContent> content = this->GetParent();
         content;
         content = content->GetParent()) {
      nsCOMPtr<nsIDOMSVGUseElement> useElement = do_QueryInterface(content);

      if (useElement) {
        nsRefPtr<nsSVGUseElement> useImpl;
        useElement->QueryInterface(NS_GET_IID(nsSVGUseElement),
                                   getter_AddRefs(useImpl));

        if (useImpl && useImpl->mOriginal == mOriginal)
          return nsnull;
      }
    }
  }

  nsCOMPtr<nsIDOMNode> newnode;
  nsCOMArray<nsINode> unused;
  nsNodeInfoManager* nodeInfoManager =
    targetContent->GetOwnerDoc() == GetOwnerDoc() ?
      nsnull : GetOwnerDoc()->NodeInfoManager();
  nsNodeUtils::Clone(targetContent, PR_TRUE, nodeInfoManager, unused,
                     getter_AddRefs(newnode));

  nsCOMPtr<nsIContent> newcontent = do_QueryInterface(newnode);

  if (!newcontent)
    return nsnull;

  nsCOMPtr<nsIDOMSVGSymbolElement> symbol     = do_QueryInterface(newcontent);
  nsCOMPtr<nsIDOMSVGSVGElement>    svg        = do_QueryInterface(newcontent);

  if (symbol) {
    nsIDocument *document = GetCurrentDoc();
    if (!document)
      return nsnull;

    nsNodeInfoManager *nodeInfoManager = document->NodeInfoManager();
    if (!nodeInfoManager)
      return nsnull;

    nsCOMPtr<nsINodeInfo> nodeInfo;
    nodeInfo = nodeInfoManager->GetNodeInfo(nsGkAtoms::svg, nsnull,
                                            kNameSpaceID_SVG,
                                            nsIDOMNode::ELEMENT_NODE);
    if (!nodeInfo)
      return nsnull;

    nsCOMPtr<nsIContent> svgNode;
    NS_NewSVGSVGElement(getter_AddRefs(svgNode), nodeInfo.forget(),
                        NOT_FROM_PARSER);

    if (!svgNode)
      return nsnull;
    
    
    const nsAttrName* name;
    PRUint32 i;
    for (i = 0; (name = newcontent->GetAttrNameAt(i)); i++) {
      nsAutoString value;
      PRInt32 nsID = name->NamespaceID();
      nsIAtom* lname = name->LocalName();

      newcontent->GetAttr(nsID, lname, value);
      svgNode->SetAttr(nsID, lname, name->GetPrefix(), value, PR_FALSE);
    }

    
    PRUint32 num = newcontent->GetChildCount();
    for (i = 0; i < num; i++) {
      nsCOMPtr<nsIContent> child = newcontent->GetChildAt(0);
      newcontent->RemoveChildAt(0, PR_FALSE);
      svgNode->InsertChildAt(child, i, PR_TRUE);
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
    return nsnull;
  nsCAutoString spec;
  baseURI->GetSpec(spec);
  newcontent->SetAttr(kNameSpaceID_XML, nsGkAtoms::base,
                      NS_ConvertUTF8toUTF16(spec), PR_FALSE);

  targetContent->AddMutationObserver(this);
  mClone = newcontent;
  return mClone;
}

void
nsSVGUseElement::DestroyAnonymousContent()
{
  nsContentUtils::DestroyAnonymousContent(&mClone);
}




PRBool nsSVGUseElement::HasValidDimensions()
{
  nsSVGSVGElement *ctx = GetCtx();

  return (!mLengthAttributes[WIDTH].IsExplicitlySet() ||
           mLengthAttributes[WIDTH].GetAnimValue(ctx) > 0) &&
         (!mLengthAttributes[HEIGHT].IsExplicitlySet() || 
           mLengthAttributes[HEIGHT].GetAnimValue(ctx) > 0);
}

void
nsSVGUseElement::SyncWidthHeight(PRUint8 aAttrEnum)
{
  if (HasValidDimensions() == !mClone) {
    TriggerReclone();
    return;
  }

  if (mClone && (aAttrEnum == WIDTH || aAttrEnum == HEIGHT)) {
    nsCOMPtr<nsIDOMSVGSymbolElement> symbol = do_QueryInterface(mClone);
    nsCOMPtr<nsIDOMSVGSVGElement>    svg    = do_QueryInterface(mClone);

    if (symbol || svg) {
      nsIAtom* aAttrName = aAttrEnum == WIDTH ?
        nsGkAtoms::width : nsGkAtoms::height;

      if (mLengthAttributes[aAttrEnum].IsExplicitlySet()) {
        static_cast<nsSVGElement*>(mClone.get())->
          SetLength(aAttrName, mLengthAttributes[aAttrEnum]);
      } else {
        
        
        
        TriggerReclone();
      }
    }
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
nsSVGUseElement::PrependLocalTransformTo(const gfxMatrix &aMatrix) const
{
  
  gfxMatrix matrix = nsSVGUseElementBase::PrependLocalTransformTo(aMatrix);

  
  float x, y;
  const_cast<nsSVGUseElement*>(this)->GetAnimatedLengthValues(&x, &y, nsnull);
  return matrix.PreMultiply(gfxMatrix().Translate(gfxPoint(x, y)));
}

void
nsSVGUseElement::DidChangeLength(PRUint8 aAttrEnum, PRBool aDoSetAttr)
{
  nsSVGUseElementBase::DidChangeLength(aAttrEnum, aDoSetAttr);

  SyncWidthHeight(aAttrEnum);
}

void
nsSVGUseElement::DidAnimateLength(PRUint8 aAttrEnum)
{
  nsSVGUseElementBase::DidAnimateLength(aAttrEnum);

  SyncWidthHeight(aAttrEnum);
}

nsSVGElement::LengthAttributesInfo
nsSVGUseElement::GetLengthInfo()
{
  return LengthAttributesInfo(mLengthAttributes, sLengthInfo,
                              NS_ARRAY_LENGTH(sLengthInfo));
}

void
nsSVGUseElement::DidChangeString(PRUint8 aAttrEnum)
{
  nsSVGUseElementBase::DidChangeString(aAttrEnum);

  if (aAttrEnum == HREF) {
    
    mOriginal = nsnull;
    UnlinkSource();
    TriggerReclone();
  }
}

void
nsSVGUseElement::DidAnimateString(PRUint8 aAttrEnum)
{
  if (aAttrEnum == HREF) {
    
    mOriginal = nsnull;
    UnlinkSource();
    TriggerReclone();
    return;
  }

  nsSVGUseElementBase::DidAnimateString(aAttrEnum);
}

nsSVGElement::StringAttributesInfo
nsSVGUseElement::GetStringInfo()
{
  return StringAttributesInfo(mStringAttributes, sStringInfo,
                              NS_ARRAY_LENGTH(sStringInfo));
}




NS_IMETHODIMP_(PRBool)
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

  return FindAttributeDependence(name, map, NS_ARRAY_LENGTH(map)) ||
    nsSVGUseElementBase::IsAttributeMapped(name);
}

