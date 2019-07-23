






































#include "nsSVGElement.h"
#include "nsGkAtoms.h"
#include "nsIDOMSVGScriptElement.h"
#include "nsIDOMSVGURIReference.h"
#include "nsCOMPtr.h"
#include "nsSVGAnimatedString.h"
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
  virtual nsresult Init();
  
public:
  
  
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMSVGSCRIPTELEMENT
  NS_DECL_NSIDOMSVGURIREFERENCE

  
  
  NS_FORWARD_NSIDOMNODE(nsSVGScriptElementBase::)
  NS_FORWARD_NSIDOMELEMENT(nsSVGScriptElementBase::)
  NS_FORWARD_NSIDOMSVGELEMENT(nsSVGScriptElementBase::)

  
  virtual void GetScriptType(nsAString& type);
  virtual already_AddRefed<nsIURI> GetScriptURI();
  virtual void GetScriptText(nsAString& text);
  virtual void GetScriptCharset(nsAString& charset); 

  
  virtual PRBool HasScriptContent();

  
  NS_IMETHOD DidModifySVGObservable (nsISVGValue* observable,
                                     nsISVGValue::modificationType aModType);

  
  virtual nsresult DoneAddingChildren(PRBool aHaveNotified);
  virtual nsresult BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                              nsIContent* aBindingParent,
                              PRBool aCompileEventHandlers);

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;
    
protected:
  nsCOMPtr<nsIDOMSVGAnimatedString> mHref;
  PRUint32 mLineNumber;
  PRPackedBool mIsEvaluated;
  PRPackedBool mEvaluating;
};

NS_IMPL_NS_NEW_SVG_ELEMENT(Script)




NS_IMPL_ADDREF_INHERITED(nsSVGScriptElement,nsSVGScriptElementBase)
NS_IMPL_RELEASE_INHERITED(nsSVGScriptElement,nsSVGScriptElementBase)

NS_INTERFACE_MAP_BEGIN(nsSVGScriptElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMNode)
  NS_INTERFACE_MAP_ENTRY(nsIDOMElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGScriptElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGURIReference)
  NS_INTERFACE_MAP_ENTRY(nsIScriptLoaderObserver)
  NS_INTERFACE_MAP_ENTRY(nsIScriptElement)
  NS_INTERFACE_MAP_ENTRY(nsIMutationObserver)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(SVGScriptElement)
NS_INTERFACE_MAP_END_INHERITING(nsSVGScriptElementBase)




nsSVGScriptElement::nsSVGScriptElement(nsINodeInfo *aNodeInfo)
  : nsSVGScriptElementBase(aNodeInfo),
    mLineNumber(0),
    mIsEvaluated(PR_FALSE),
    mEvaluating(PR_FALSE)
{
  AddMutationObserver(this);
}

nsresult
nsSVGScriptElement::Init()
{
  nsresult rv;

  

  
  
  {
    rv = NS_NewSVGAnimatedString(getter_AddRefs(mHref));
    NS_ENSURE_SUCCESS(rv,rv);
    rv = AddMappedSVGValue(nsGkAtoms::href, mHref, kNameSpaceID_XLink);
    NS_ENSURE_SUCCESS(rv,rv);
  }

  return NS_OK;
}




NS_IMPL_ELEMENT_CLONE_WITH_INIT(nsSVGScriptElement)





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
  *aHref = mHref;
  NS_IF_ADDREF(*aHref);
  return NS_OK;
}




void
nsSVGScriptElement::GetScriptType(nsAString& type)
{
  GetType(type);
}




already_AddRefed<nsIURI>
nsSVGScriptElement::GetScriptURI()
{
  nsIURI *uri = nsnull;
  nsAutoString src;
  mHref->GetAnimVal(src);
  if (!src.IsEmpty()) {
    nsCOMPtr<nsIURI> baseURI = GetBaseURI();
    NS_NewURI(&uri, src, nsnull, baseURI);
  }
  return uri;
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




PRBool
nsSVGScriptElement::HasScriptContent()
{
  nsAutoString src;
  mHref->GetAnimVal(src);
  return !src.IsEmpty() || nsContentUtils::HasNonEmptyTextContent(this);
}




NS_IMETHODIMP
nsSVGScriptElement::DidModifySVGObservable(nsISVGValue* aObservable,
                                           nsISVGValue::modificationType aModType)
{
  nsresult rv = nsSVGScriptElementBase::DidModifySVGObservable(aObservable,
                                                               aModType);
  
  
  MaybeProcessScript();
  
  return rv;
}




nsresult
nsSVGScriptElement::DoneAddingChildren(PRBool aHaveNotified)
{
  mDoneAddingChildren = PR_TRUE;
  return MaybeProcessScript();
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
