





































#include "nsSVGStylableElement.h"
#include "nsIDOMSVGTitleElement.h"

typedef nsSVGStylableElement nsSVGTitleElementBase;

class nsSVGTitleElement : public nsSVGTitleElementBase,
                          public nsIDOMSVGTitleElement,
                          public nsStubMutationObserver
{
protected:
  friend nsresult NS_NewSVGTitleElement(nsIContent **aResult,
                                        nsINodeInfo *aNodeInfo);
  nsSVGTitleElement(nsINodeInfo *aNodeInfo);
  nsresult Init();

public:
  

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMSVGTITLEELEMENT

  
  NS_FORWARD_NSIDOMNODE(nsSVGTitleElementBase::)
  NS_FORWARD_NSIDOMELEMENT(nsSVGTitleElementBase::)
  NS_FORWARD_NSIDOMSVGELEMENT(nsSVGTitleElementBase::)

  
  NS_DECL_NSIMUTATIONOBSERVER_CHARACTERDATACHANGED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTAPPENDED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTINSERTED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTREMOVED

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  virtual nsresult BindToTree(nsIDocument *aDocument, nsIContent *aParent,
                              nsIContent *aBindingParent,
                              PRBool aCompileEventHandlers);

  virtual void UnbindFromTree(PRBool aDeep = PR_TRUE,
                              PRBool aNullParent = PR_TRUE);

  virtual nsresult DoneAddingChildren(PRBool aHaveNotified);

private:
  void SendTitleChangeEvent(PRBool aBound);
};

NS_IMPL_NS_NEW_SVG_ELEMENT(Title)





NS_IMPL_ADDREF_INHERITED(nsSVGTitleElement, nsSVGTitleElementBase)
NS_IMPL_RELEASE_INHERITED(nsSVGTitleElement, nsSVGTitleElementBase)

NS_INTERFACE_TABLE_HEAD(nsSVGTitleElement)
  NS_NODE_INTERFACE_TABLE5(nsSVGTitleElement, nsIDOMNode, nsIDOMElement,
                           nsIDOMSVGElement, nsIDOMSVGTitleElement,
                           nsIMutationObserver)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(SVGTitleElement)
NS_INTERFACE_MAP_END_INHERITING(nsSVGTitleElementBase)





nsSVGTitleElement::nsSVGTitleElement(nsINodeInfo *aNodeInfo)
  : nsSVGTitleElementBase(aNodeInfo)
{
  AddMutationObserver(this);
}

nsresult
nsSVGTitleElement::Init()
{
  return nsSVGTitleElementBase::Init();
}

void
nsSVGTitleElement::CharacterDataChanged(nsIDocument *aDocument,
                                        nsIContent *aContent,
                                        CharacterDataChangeInfo *aInfo)
{
  SendTitleChangeEvent(PR_FALSE);
}

void
nsSVGTitleElement::ContentAppended(nsIDocument *aDocument,
                                   nsIContent *aContainer,
                                   PRInt32 aNewIndexInContainer)
{
  SendTitleChangeEvent(PR_FALSE);
}

void
nsSVGTitleElement::ContentInserted(nsIDocument *aDocument,
                                   nsIContent *aContainer,
                                   nsIContent *aChild,
                                   PRInt32 aIndexInContainer)
{
  SendTitleChangeEvent(PR_FALSE);
}

void
nsSVGTitleElement::ContentRemoved(nsIDocument *aDocument,
                                  nsIContent *aContainer,
                                  nsIContent *aChild,
                                  PRInt32 aIndexInContainer)
{
  SendTitleChangeEvent(PR_FALSE);
}

nsresult
nsSVGTitleElement::BindToTree(nsIDocument *aDocument,
                               nsIContent *aParent,
                               nsIContent *aBindingParent,
                               PRBool aCompileEventHandlers)
{
  
  nsresult rv = nsSVGTitleElementBase::BindToTree(aDocument, aParent,
                                                  aBindingParent,
                                                  aCompileEventHandlers);
  NS_ENSURE_SUCCESS(rv, rv);

  SendTitleChangeEvent(PR_TRUE);

  return NS_OK;
}

void
nsSVGTitleElement::UnbindFromTree(PRBool aDeep, PRBool aNullParent)
{
  SendTitleChangeEvent(PR_FALSE);

  
  nsSVGTitleElementBase::UnbindFromTree(aDeep, aNullParent);
}

nsresult
nsSVGTitleElement::DoneAddingChildren(PRBool aHaveNotified)
{
  if (!aHaveNotified) {
    SendTitleChangeEvent(PR_FALSE);
  }
  return NS_OK;
}

void
nsSVGTitleElement::SendTitleChangeEvent(PRBool aBound)
{
  nsIDocument* doc = GetCurrentDoc();
  if (doc) {
    doc->NotifyPossibleTitleChange(aBound);
  }
}




NS_IMPL_ELEMENT_CLONE_WITH_INIT(nsSVGTitleElement)
