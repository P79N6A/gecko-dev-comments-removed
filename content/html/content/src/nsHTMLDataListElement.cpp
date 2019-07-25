




































#include "nsIDOMHTMLDataListElement.h"
#include "nsGenericHTMLElement.h"
#include "nsIDOMEventTarget.h"
#include "nsGkAtoms.h"
#include "nsIDOMHTMLOptionElement.h"


class nsHTMLDataListElement : public nsGenericHTMLElement,
                              public nsIDOMHTMLDataListElement
{
public:
  nsHTMLDataListElement(already_AddRefed<nsINodeInfo> aNodeInfo);
  virtual ~nsHTMLDataListElement();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE(nsGenericHTMLElement::)

  
  NS_FORWARD_NSIDOMELEMENT(nsGenericHTMLElement::)

  
  NS_FORWARD_NSIDOMHTMLELEMENT(nsGenericHTMLElement::)

  
  NS_DECL_NSIDOMHTMLDATALISTELEMENT

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  
  static bool MatchOptions(nsIContent* aContent, PRInt32 aNamespaceID,
                             nsIAtom* aAtom, void* aData);

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(nsHTMLDataListElement,
                                           nsGenericHTMLElement)

  virtual nsXPCClassInfo* GetClassInfo();

protected:

  
  nsRefPtr<nsContentList> mOptions;
};


NS_IMPL_NS_NEW_HTML_ELEMENT(DataList)


nsHTMLDataListElement::nsHTMLDataListElement(already_AddRefed<nsINodeInfo> aNodeInfo)
  : nsGenericHTMLElement(aNodeInfo)
{
}

nsHTMLDataListElement::~nsHTMLDataListElement()
{
}


NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(nsHTMLDataListElement,
                                                nsGenericHTMLElement)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mOptions)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_CLASS(nsHTMLDataListElement)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(nsHTMLDataListElement,
                                                  nsGenericHTMLElement)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR_AMBIGUOUS(mOptions, nsIDOMNodeList)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_ADDREF_INHERITED(nsHTMLDataListElement, nsGenericElement)
NS_IMPL_RELEASE_INHERITED(nsHTMLDataListElement, nsGenericElement)

DOMCI_NODE_DATA(HTMLDataListElement, nsHTMLDataListElement)

NS_INTERFACE_TABLE_HEAD_CYCLE_COLLECTION_INHERITED(nsHTMLDataListElement)
  NS_HTML_CONTENT_INTERFACE_TABLE1(nsHTMLDataListElement, nsIDOMHTMLDataListElement)
  NS_HTML_CONTENT_INTERFACE_TABLE_TO_MAP_SEGUE(nsHTMLDataListElement,
                                               nsGenericHTMLElement)
NS_HTML_CONTENT_INTERFACE_TABLE_TAIL_CLASSINFO(HTMLDataListElement)


NS_IMPL_ELEMENT_CLONE(nsHTMLDataListElement)

bool
nsHTMLDataListElement::MatchOptions(nsIContent* aContent, PRInt32 aNamespaceID,
                                    nsIAtom* aAtom, void* aData)
{
  return aContent->NodeInfo()->Equals(nsGkAtoms::option, kNameSpaceID_XHTML) &&
         !aContent->HasAttr(kNameSpaceID_None, nsGkAtoms::disabled);
}

NS_IMETHODIMP
nsHTMLDataListElement::GetOptions(nsIDOMHTMLCollection** aOptions)
{
  if (!mOptions) {
    mOptions = new nsContentList(this, MatchOptions, nsnull, nsnull, PR_TRUE);
  }

  NS_ADDREF(*aOptions = mOptions);

  return NS_OK;
}

