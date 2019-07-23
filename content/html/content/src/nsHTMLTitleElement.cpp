




































#include "nsIDOMHTMLTitleElement.h"
#include "nsIDOMEventTarget.h"
#include "nsGenericHTMLElement.h"
#include "nsStyleConsts.h"
#include "nsPresContext.h"
#include "nsIDOMText.h"
#include "nsIDocument.h"
#include "nsIDOMHTMLDocument.h"
#include "nsContentUtils.h"
#include "nsPLDOMEvent.h"

class nsHTMLTitleElement : public nsGenericHTMLElement,
                           public nsIDOMHTMLTitleElement,
                           public nsStubMutationObserver
{
public:
  nsHTMLTitleElement(nsINodeInfo *aNodeInfo);
  virtual ~nsHTMLTitleElement();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE(nsGenericHTMLElement::)

  
  NS_FORWARD_NSIDOMELEMENT(nsGenericHTMLElement::)

  
  NS_FORWARD_NSIDOMHTMLELEMENT(nsGenericHTMLElement::)

  
  NS_DECL_NSIDOMHTMLTITLEELEMENT

  
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


NS_IMPL_NS_NEW_HTML_ELEMENT(Title)


nsHTMLTitleElement::nsHTMLTitleElement(nsINodeInfo *aNodeInfo)
  : nsGenericHTMLElement(aNodeInfo)
{
  AddMutationObserver(this);
}

nsHTMLTitleElement::~nsHTMLTitleElement()
{
}


NS_IMPL_ADDREF_INHERITED(nsHTMLTitleElement, nsGenericElement) 
NS_IMPL_RELEASE_INHERITED(nsHTMLTitleElement, nsGenericElement) 



NS_INTERFACE_TABLE_HEAD(nsHTMLTitleElement)
  NS_HTML_CONTENT_INTERFACE_TABLE2(nsHTMLTitleElement,
                                   nsIDOMHTMLTitleElement,
                                   nsIMutationObserver)
  NS_HTML_CONTENT_INTERFACE_TABLE_TO_MAP_SEGUE(nsHTMLTitleElement,
                                               nsGenericHTMLElement)
NS_HTML_CONTENT_INTERFACE_TABLE_TAIL_CLASSINFO(HTMLTitleElement)


NS_IMPL_ELEMENT_CLONE(nsHTMLTitleElement)


NS_IMETHODIMP 
nsHTMLTitleElement::GetText(nsAString& aTitle)
{
  nsContentUtils::GetNodeTextContent(this, PR_FALSE, aTitle);
  return NS_OK;
}

NS_IMETHODIMP 
nsHTMLTitleElement::SetText(const nsAString& aTitle)
{
  return nsContentUtils::SetNodeTextContent(this, aTitle, PR_TRUE);
}

void
nsHTMLTitleElement::CharacterDataChanged(nsIDocument *aDocument,
                                         nsIContent *aContent,
                                         CharacterDataChangeInfo *aInfo)
{
  SendTitleChangeEvent(PR_FALSE);
}

void
nsHTMLTitleElement::ContentAppended(nsIDocument *aDocument,
                                    nsIContent *aContainer,
                                    PRInt32 aNewIndexInContainer)
{
  SendTitleChangeEvent(PR_FALSE);
}

void
nsHTMLTitleElement::ContentInserted(nsIDocument *aDocument,
                                    nsIContent *aContainer,
                                    nsIContent *aChild,
                                    PRInt32 aIndexInContainer)
{
  SendTitleChangeEvent(PR_FALSE);
}

void
nsHTMLTitleElement::ContentRemoved(nsIDocument *aDocument,
                                   nsIContent *aContainer,
                                   nsIContent *aChild,
                                   PRInt32 aIndexInContainer)
{
  SendTitleChangeEvent(PR_FALSE);
}

nsresult
nsHTMLTitleElement::BindToTree(nsIDocument *aDocument,
                               nsIContent *aParent,
                               nsIContent *aBindingParent,
                               PRBool aCompileEventHandlers)
{
  
  nsresult rv = nsGenericHTMLElement::BindToTree(aDocument, aParent,
                                                 aBindingParent,
                                                 aCompileEventHandlers);
  NS_ENSURE_SUCCESS(rv, rv);

  SendTitleChangeEvent(PR_TRUE);

  return NS_OK;
}

void
nsHTMLTitleElement::UnbindFromTree(PRBool aDeep, PRBool aNullParent)
{
  SendTitleChangeEvent(PR_FALSE);

  
  nsGenericHTMLElement::UnbindFromTree(aDeep, aNullParent);
}

nsresult
nsHTMLTitleElement::DoneAddingChildren(PRBool aHaveNotified)
{
  if (!aHaveNotified) {
    SendTitleChangeEvent(PR_FALSE);
  }
  return NS_OK;
}

void
nsHTMLTitleElement::SendTitleChangeEvent(PRBool aBound)
{
  nsIDocument* doc = GetCurrentDoc();
  if (doc) {
    doc->NotifyPossibleTitleChange(aBound);
  }
}
