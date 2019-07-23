



































#include "nsSVGUseElement.h"
#include "nsIDOMSVGGElement.h"
#include "nsGkAtoms.h"
#include "nsIDOMSVGAnimatedLength.h"
#include "nsIDOMSVGAnimatedString.h"
#include "nsSVGAnimatedString.h"
#include "nsIDOMDocument.h"
#include "nsIDOMSVGSVGElement.h"
#include "nsIDOMSVGSymbolElement.h"
#include "nsIDocument.h"
#include "nsIPresShell.h"




nsSVGElement::LengthInfo nsSVGUseElement::sLengthInfo[4] =
{
  { &nsGkAtoms::x, 0, nsIDOMSVGLength::SVG_LENGTHTYPE_NUMBER, nsSVGUtils::X },
  { &nsGkAtoms::y, 0, nsIDOMSVGLength::SVG_LENGTHTYPE_NUMBER, nsSVGUtils::Y },
  { &nsGkAtoms::width, 0, nsIDOMSVGLength::SVG_LENGTHTYPE_NUMBER, nsSVGUtils::X },
  { &nsGkAtoms::height, 0, nsIDOMSVGLength::SVG_LENGTHTYPE_NUMBER, nsSVGUtils::Y },
};

NS_IMPL_NS_NEW_SVG_ELEMENT(Use)




NS_IMPL_CYCLE_COLLECTION_CLASS(nsSVGUseElement)
NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(nsSVGUseElement,
                                                nsSVGUseElementBase)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mOriginal)
  tmp->DestroyAnonymousContent();
  tmp->RemoveListener();
NS_IMPL_CYCLE_COLLECTION_UNLINK_END
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(nsSVGUseElement,
                                                  nsSVGUseElementBase)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mOriginal)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mClone)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mSourceContent)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_ADDREF_INHERITED(nsSVGUseElement,nsSVGUseElementBase)
NS_IMPL_RELEASE_INHERITED(nsSVGUseElement,nsSVGUseElementBase)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(nsSVGUseElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMNode)
  NS_INTERFACE_MAP_ENTRY(nsIDOMElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGURIReference)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGUseElement)
  NS_INTERFACE_MAP_ENTRY(nsIMutationObserver)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(SVGUseElement)
  if (aIID.Equals(NS_GET_IID(nsSVGUseElement)))
    foundInterface = reinterpret_cast<nsISupports*>(this);
  else
NS_INTERFACE_MAP_END_INHERITING(nsSVGUseElementBase)




nsSVGUseElement::nsSVGUseElement(nsINodeInfo *aNodeInfo)
  : nsSVGUseElementBase(aNodeInfo)
{
}

nsSVGUseElement::~nsSVGUseElement()
{
  RemoveListener();
}

nsresult
nsSVGUseElement::Init()
{
  nsresult rv = nsSVGUseElementBase::Init();
  NS_ENSURE_SUCCESS(rv,rv);

  

  
  
  {
    rv = NS_NewSVGAnimatedString(getter_AddRefs(mHref));
    NS_ENSURE_SUCCESS(rv,rv);
    rv = AddMappedSVGValue(nsGkAtoms::href, mHref, kNameSpaceID_XLink);
    NS_ENSURE_SUCCESS(rv,rv);
  }

  return rv;
}




nsresult
nsSVGUseElement::Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const
{
  *aResult = nsnull;

  nsSVGUseElement *it = new (aNodeInfo) nsSVGUseElement(aNodeInfo);
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
  *aHref = mHref;
  NS_IF_ADDREF(*aHref);
  return NS_OK;
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




NS_IMETHODIMP
nsSVGUseElement::DidModifySVGObservable(nsISVGValue* aObservable,
                                        nsISVGValue::modificationType aModType)
{
  nsCOMPtr<nsIDOMSVGAnimatedString> s = do_QueryInterface(aObservable);

  if (s && mHref == s) {
    
    mOriginal = nsnull;

    TriggerReclone();
  }

  return nsSVGUseElementBase::DidModifySVGObservable(aObservable, aModType);
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
nsSVGUseElement::AttributeChanged(nsIDocument *aDocument,
                                  nsIContent *aContent,
                                  PRInt32 aNameSpaceID,
                                  nsIAtom *aAttribute,
                                  PRInt32 aModType,
                                  PRUint32 aStateMask)
{
  if (nsContentUtils::IsInSameAnonymousTree(this, aContent)) {
    TriggerReclone();
  }
}

void
nsSVGUseElement::ContentAppended(nsIDocument *aDocument,
                                 nsIContent *aContainer,
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
                                PRInt32 aIndexInContainer)
{
  if (nsContentUtils::IsInSameAnonymousTree(this, aChild)) {
    TriggerReclone();
  }
}

void
nsSVGUseElement::NodeWillBeDestroyed(const nsINode *aNode)
{
  RemoveListener();
}



nsIContent*
nsSVGUseElement::CreateAnonymousContent()
{
#ifdef DEBUG_tor
  nsAutoString href;
  mHref->GetAnimVal(href);
  fprintf(stderr, "<svg:use> reclone of \"%s\"\n", ToNewCString(href));
#endif

  mClone = nsnull;

  nsCOMPtr<nsIContent> targetContent = LookupHref();

  if (!targetContent)
    return nsnull;

  PRBool needAddObserver = PR_FALSE;
  if (mSourceContent != targetContent) {
    RemoveListener();
    needAddObserver = PR_TRUE;
  }

  
  
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
  nsNodeUtils::Clone(targetContent, PR_TRUE, nsnull, unused,
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
    nodeInfoManager->GetNodeInfo(nsGkAtoms::svg, nsnull, kNameSpaceID_SVG,
                                 getter_AddRefs(nodeInfo));
    if (!nodeInfo)
      return nsnull;

    nsCOMPtr<nsIContent> svgNode;
    NS_NewSVGSVGElement(getter_AddRefs(svgNode), nodeInfo);

    if (!svgNode)
      return nsnull;
    
    if (newcontent->HasAttr(kNameSpaceID_None, nsGkAtoms::viewBox)) {
      nsAutoString viewbox;
      newcontent->GetAttr(kNameSpaceID_None, nsGkAtoms::viewBox, viewbox);
      svgNode->SetAttr(kNameSpaceID_None, nsGkAtoms::viewBox, viewbox, PR_FALSE);
    }

    
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
    if (HasAttr(kNameSpaceID_None, nsGkAtoms::width)) {
      nsAutoString width;
      GetAttr(kNameSpaceID_None, nsGkAtoms::width, width);
      newcontent->SetAttr(kNameSpaceID_None, nsGkAtoms::width, width, PR_FALSE);
    }

    if (HasAttr(kNameSpaceID_None, nsGkAtoms::height)) {
      nsAutoString height;
      GetAttr(kNameSpaceID_None, nsGkAtoms::height, height);
      newcontent->SetAttr(kNameSpaceID_None, nsGkAtoms::height, height, PR_FALSE);
    }
  }

  if (needAddObserver) {
    targetContent->AddMutationObserver(this);
  }
  mSourceContent = targetContent;
  mClone = newcontent;
  return mClone;
}

void
nsSVGUseElement::DestroyAnonymousContent()
{
  nsContentUtils::DestroyAnonymousContent(&mClone);
}




void
nsSVGUseElement::SyncWidthHeight(PRUint8 aAttrEnum)
{
  if (mClone && (aAttrEnum == WIDTH || aAttrEnum == HEIGHT)) {
    nsCOMPtr<nsIDOMSVGSymbolElement> symbol = do_QueryInterface(mClone);
    nsCOMPtr<nsIDOMSVGSVGElement>    svg    = do_QueryInterface(mClone);

    if (symbol || svg) {
      if (aAttrEnum == WIDTH) {
        nsAutoString width;
        GetAttr(kNameSpaceID_None, nsGkAtoms::width, width);
        mClone->SetAttr(kNameSpaceID_None, nsGkAtoms::width, width, PR_FALSE);
      } else if (aAttrEnum == HEIGHT) {
        nsAutoString height;
        GetAttr(kNameSpaceID_None, nsGkAtoms::height, height);
        mClone->SetAttr(kNameSpaceID_None, nsGkAtoms::height, height, PR_FALSE);
      }
    }
  }
}

nsIContent *
nsSVGUseElement::LookupHref()
{
  nsAutoString href;
  mHref->GetAnimVal(href);
  if (href.IsEmpty())
    return nsnull;

  nsCOMPtr<nsIURI> targetURI, baseURI = GetBaseURI();
  nsContentUtils::NewURIWithDocumentCharset(getter_AddRefs(targetURI), href,
                                            GetCurrentDoc(), baseURI);

  return nsContentUtils::GetReferencedElement(targetURI, this);
}

void
nsSVGUseElement::TriggerReclone()
{
  nsIDocument *doc = GetCurrentDoc();
  if (!doc) return;
  nsIPresShell *presShell = doc->GetPrimaryShell();
  if (!presShell) return;
  presShell->RecreateFramesFor(this);
}

void
nsSVGUseElement::RemoveListener()
{
  if (mSourceContent) {
    mSourceContent->RemoveMutationObserver(this);
    mSourceContent = nsnull;
  }
}




void
nsSVGUseElement::DidChangeLength(PRUint8 aAttrEnum, PRBool aDoSetAttr)
{
  nsSVGUseElementBase::DidChangeLength(aAttrEnum, aDoSetAttr);

  SyncWidthHeight(aAttrEnum);
}

nsSVGElement::LengthAttributesInfo
nsSVGUseElement::GetLengthInfo()
{
  return LengthAttributesInfo(mLengthAttributes, sLengthInfo,
                              NS_ARRAY_LENGTH(sLengthInfo));
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
