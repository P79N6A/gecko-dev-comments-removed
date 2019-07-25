






































#include "mozilla/Util.h"

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
#include "nsContentUtils.h"

using namespace mozilla;
using namespace mozilla::dom;

typedef nsSVGElement nsSVGScriptElementBase;

class nsSVGScriptElement : public nsSVGScriptElementBase,
                           public nsIDOMSVGScriptElement, 
                           public nsIDOMSVGURIReference,
                           public nsScriptElement
{
protected:
  friend nsresult NS_NewSVGScriptElement(nsIContent **aResult,
                                         already_AddRefed<nsINodeInfo> aNodeInfo,
                                         FromParser aFromParser);
  nsSVGScriptElement(already_AddRefed<nsINodeInfo> aNodeInfo,
                     FromParser aFromParser);
  
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
  
  
  virtual bool HasScriptContent();

  
  virtual void DidChangeString(PRUint8 aAttrEnum);

  
  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              bool aCompileEventHandlers);

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  virtual nsXPCClassInfo* GetClassInfo();
protected:
  virtual StringAttributesInfo GetStringInfo();

  enum { HREF };
  nsSVGString mStringAttributes[1];
  static StringInfo sStringInfo[1];
};

nsSVGElement::StringInfo nsSVGScriptElement::sStringInfo[1] =
{
  { &nsGkAtoms::href, kNameSpaceID_XLink, false }
};

NS_IMPL_NS_NEW_SVG_ELEMENT_CHECK_PARSER(Script)




NS_IMPL_ADDREF_INHERITED(nsSVGScriptElement,nsSVGScriptElementBase)
NS_IMPL_RELEASE_INHERITED(nsSVGScriptElement,nsSVGScriptElementBase)

DOMCI_NODE_DATA(SVGScriptElement, nsSVGScriptElement)

NS_INTERFACE_TABLE_HEAD(nsSVGScriptElement)
  NS_NODE_INTERFACE_TABLE8(nsSVGScriptElement, nsIDOMNode, nsIDOMElement,
                           nsIDOMSVGElement, nsIDOMSVGScriptElement,
                           nsIDOMSVGURIReference, nsIScriptLoaderObserver,
                           nsIScriptElement, nsIMutationObserver)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(SVGScriptElement)
NS_INTERFACE_MAP_END_INHERITING(nsSVGScriptElementBase)




nsSVGScriptElement::nsSVGScriptElement(already_AddRefed<nsINodeInfo> aNodeInfo,
                                       FromParser aFromParser)
  : nsSVGScriptElementBase(aNodeInfo)
  , nsScriptElement(aFromParser)
{
  AddMutationObserver(this);
}




nsresult
nsSVGScriptElement::Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const
{
  *aResult = nsnull;

  nsCOMPtr<nsINodeInfo> ni = aNodeInfo;
  nsSVGScriptElement* it = new nsSVGScriptElement(ni.forget(), NOT_FROM_PARSER);

  nsCOMPtr<nsINode> kungFuDeathGrip = it;
  nsresult rv = it->Init();
  rv |= CopyInnerTo(it);
  NS_ENSURE_SUCCESS(rv, rv);

  
  it->mAlreadyStarted = mAlreadyStarted;
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
  return SetAttr(kNameSpaceID_None, nsGkAtoms::type, aType, true); 
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
  nsContentUtils::GetNodeTextContent(this, false, text);
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

  if (mStringAttributes[HREF].IsExplicitlySet()) {
    
    
    nsAutoString src;
    mStringAttributes[HREF].GetAnimValue(src, this);

    nsCOMPtr<nsIURI> baseURI = GetBaseURI();
    NS_NewURI(getter_AddRefs(mUri), src, nsnull, baseURI);
    
    mExternal = true;
  }
  
  mFrozen = true;
}




bool
nsSVGScriptElement::HasScriptContent()
{
  return (mFrozen ? mExternal : mStringAttributes[HREF].IsExplicitlySet()) ||
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
                              ArrayLength(sStringInfo));
}




nsresult
nsSVGScriptElement::BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                               nsIContent* aBindingParent,
                               bool aCompileEventHandlers)
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

