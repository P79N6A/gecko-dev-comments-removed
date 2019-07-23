






































#include "nsSVGElement.h"
#include "nsGkAtoms.h"
#include "nsIDOMSVGScriptElement.h"
#include "nsIDOMSVGURIReference.h"
#include "nsCOMPtr.h"
#include "nsSVGString.h"
#include "nsIDocument.h"
#include "nsIURI.h"
#include "nsNetUtil.h"
#include "nsScriptElement.h"
#include "nsIDOMText.h"

typedef nsSVGElement nsSVGScriptElementBase;

class nsSVGScriptElement : public nsSVGScriptElementBase,
                           public nsIDOMSVGScriptElement, 
                           public nsIDOMSVGURIReference,
                           public nsScriptElement
{
protected:
  friend nsresult NS_NewSVGScriptElement(nsIContent **aResult,
                                         nsINodeInfo *aNodeInfo);
  nsSVGScriptElement(nsINodeInfo *aNodeInfo);
  
public:
  
  
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMSVGSCRIPTELEMENT
  NS_DECL_NSIDOMSVGURIREFERENCE

  
  
  NS_FORWARD_NSIDOMNODE(nsSVGScriptElementBase::)
  NS_FORWARD_NSIDOMELEMENT(nsSVGScriptElementBase::)
  NS_FORWARD_NSIDOMSVGELEMENT(nsSVGScriptElementBase::)

  
  virtual void GetScriptType(nsAString& type);
  virtual void GetScriptText(nsAString& text);
  virtual void GetScriptCharset(nsAString& charset);
  virtual void FreezeUriAsyncDefer();
  
  
  virtual PRBool HasScriptContent();

  
  virtual void DidChangeString(PRUint8 aAttrEnum);

  
  virtual nsresult DoneAddingChildren(PRBool aHaveNotified);
  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              PRBool aCompileEventHandlers);

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;
    
protected:
  virtual StringAttributesInfo GetStringInfo();

  enum { HREF };
  nsSVGString mStringAttributes[1];
  static StringInfo sStringInfo[1];
};

nsSVGElement::StringInfo nsSVGScriptElement::sStringInfo[1] =
{
  { &nsGkAtoms::href, kNameSpaceID_XLink }
};

NS_IMPL_NS_NEW_SVG_ELEMENT(Script)




NS_IMPL_ADDREF_INHERITED(nsSVGScriptElement,nsSVGScriptElementBase)
NS_IMPL_RELEASE_INHERITED(nsSVGScriptElement,nsSVGScriptElementBase)

NS_INTERFACE_TABLE_HEAD(nsSVGScriptElement)
  NS_NODE_INTERFACE_TABLE8(nsSVGScriptElement, nsIDOMNode, nsIDOMElement,
                           nsIDOMSVGElement, nsIDOMSVGScriptElement,
                           nsIDOMSVGURIReference, nsIScriptLoaderObserver,
                           nsIScriptElement, nsIMutationObserver)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(SVGScriptElement)
NS_INTERFACE_MAP_END_INHERITING(nsSVGScriptElementBase)




nsSVGScriptElement::nsSVGScriptElement(nsINodeInfo *aNodeInfo)
  : nsSVGScriptElementBase(aNodeInfo)
{
  AddMutationObserver(this);
}




nsresult
nsSVGScriptElement::Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const
{
  *aResult = nsnull;

  nsSVGScriptElement* it = new nsSVGScriptElement(aNodeInfo);
  if (!it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  nsCOMPtr<nsINode> kungFuDeathGrip = it;
  nsresult rv = it->Init();
  rv |= CopyInnerTo(it);
  NS_ENSURE_SUCCESS(rv, rv);

  
  it->mIsEvaluated = mIsEvaluated;
  it->mLineNumber = mLineNumber;
  it->mMalformed = mMalformed;

  kungFuDeathGrip.swap(*aResult);

  return NS_OK;
}





NS_IMETHODIMP
nsSVGScriptElement::GetType(nsAString & aType)
{
  GetAttr(kNameSpaceID_None, nsGkAtoms::type, aType);

  return NS_OK;
}
NS_IMETHODIMP
nsSVGScriptElement::SetType(const nsAString & aType)
{
  NS_ERROR("write me!");
  return NS_ERROR_NOT_IMPLEMENTED;
}





NS_IMETHODIMP
nsSVGScriptElement::GetHref(nsIDOMSVGAnimatedString * *aHref)
{
  return mStringAttributes[HREF].ToDOMAnimatedString(aHref, this);
}




void
nsSVGScriptElement::GetScriptType(nsAString& type)
{
  GetType(type);
}

void
nsSVGScriptElement::GetScriptText(nsAString& text)
{
  nsContentUtils::GetNodeTextContent(this, PR_FALSE, text);
}

void
nsSVGScriptElement::GetScriptCharset(nsAString& charset)
{
  charset.Truncate();
}

void
nsSVGScriptElement::FreezeUriAsyncDefer()
{
  if (mFrozen) {
    return;
  }

  
  
  nsAutoString src;
  mStringAttributes[HREF].GetAnimValue(src, this);
  
  if (!src.IsEmpty()) {
    nsCOMPtr<nsIURI> baseURI = GetBaseURI();
    NS_NewURI(getter_AddRefs(mUri), src, nsnull, baseURI);
  }
  
  mFrozen = PR_TRUE;
}




PRBool
nsSVGScriptElement::HasScriptContent()
{
  nsAutoString src;
  mStringAttributes[HREF].GetAnimValue(src, this);
  
  return (mFrozen ? !!mUri : !src.IsEmpty()) ||
         nsContentUtils::HasNonEmptyTextContent(this);
}




void
nsSVGScriptElement::DidChangeString(PRUint8 aAttrEnum)
{
  nsSVGScriptElementBase::DidChangeString(aAttrEnum);

  if (aAttrEnum == HREF) {
    MaybeProcessScript();
  }
}

nsSVGElement::StringAttributesInfo
nsSVGScriptElement::GetStringInfo()
{
  return StringAttributesInfo(mStringAttributes, sStringInfo,
                              NS_ARRAY_LENGTH(sStringInfo));
}




nsresult
nsSVGScriptElement::DoneAddingChildren(PRBool aHaveNotified)
{
  mDoneAddingChildren = PR_TRUE;
  nsresult rv = MaybeProcessScript();
  if (!mIsEvaluated) {
    
    
    mFrozen = PR_FALSE;
    mUri = nsnull;
  }
  return rv;
}

nsresult
nsSVGScriptElement::BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                               nsIContent* aBindingParent,
                               PRBool aCompileEventHandlers)
{
  nsresult rv = nsSVGScriptElementBase::BindToTree(aDocument, aParent,
                                                   aBindingParent,
                                                   aCompileEventHandlers);
  NS_ENSURE_SUCCESS(rv, rv);

  if (aDocument) {
    MaybeProcessScript();
  }

  return NS_OK;
}

