




































#include "nsIDOMHTMLOutputElement.h"
#include "nsGenericHTMLElement.h"
#include "nsFormSubmission.h"
#include "nsDOMSettableTokenList.h"
#include "nsStubMutationObserver.h"


class nsHTMLOutputElement : public nsGenericHTMLFormElement,
                            public nsIDOMHTMLOutputElement,
                            public nsStubMutationObserver
{
public:
  nsHTMLOutputElement(already_AddRefed<nsINodeInfo> aNodeInfo);
  virtual ~nsHTMLOutputElement();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE(nsGenericHTMLFormElement::)

  
  NS_FORWARD_NSIDOMELEMENT(nsGenericHTMLFormElement::)

  
  NS_FORWARD_NSIDOMHTMLELEMENT(nsGenericHTMLFormElement::)

  
  NS_DECL_NSIDOMHTMLOUTPUTELEMENT

  
  NS_IMETHOD_(PRUint32) GetType() const { return NS_FORM_OUTPUT; }
  NS_IMETHOD Reset();
  NS_IMETHOD SubmitNamesValues(nsFormSubmission* aFormSubmission,
                               nsIContent* aSubmitElement);

  nsresult Clone(nsINodeInfo* aNodeInfo, nsINode** aResult) const;

  PRBool ParseAttribute(PRInt32 aNamespaceID, nsIAtom* aAttribute,
                        const nsAString& aValue, nsAttrValue& aResult);

  
  
  void DescendantsChanged();

  
  NS_DECL_NSIMUTATIONOBSERVER_CHARACTERDATACHANGED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTAPPENDED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTINSERTED
  NS_DECL_NSIMUTATIONOBSERVER_CONTENTREMOVED

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED_NO_UNLINK(nsHTMLOutputElement,
                                                     nsGenericHTMLFormElement)

  virtual nsXPCClassInfo* GetClassInfo();
protected:
  enum ValueModeFlag {
    eModeDefault,
    eModeValue
  };

  ValueModeFlag                     mValueModeFlag;
  nsString                          mDefaultValue;
  nsRefPtr<nsDOMSettableTokenList>  mTokenList;
};


NS_IMPL_NS_NEW_HTML_ELEMENT(Output)


nsHTMLOutputElement::nsHTMLOutputElement(already_AddRefed<nsINodeInfo> aNodeInfo)
  : nsGenericHTMLFormElement(aNodeInfo)
  , mValueModeFlag(eModeDefault)
{
  AddMutationObserver(this);
}

nsHTMLOutputElement::~nsHTMLOutputElement()
{
  if (mTokenList) {
    mTokenList->DropReference();
  }
}


NS_IMPL_ADDREF_INHERITED(nsHTMLOutputElement, nsGenericElement)
NS_IMPL_RELEASE_INHERITED(nsHTMLOutputElement, nsGenericElement)

DOMCI_NODE_DATA(HTMLOutputElement, nsHTMLOutputElement)

NS_INTERFACE_TABLE_HEAD(nsHTMLOutputElement)
  NS_HTML_CONTENT_INTERFACE_TABLE2(nsHTMLOutputElement,
                                   nsIDOMHTMLOutputElement,
                                   nsIMutationObserver)
  NS_HTML_CONTENT_INTERFACE_TABLE_TO_MAP_SEGUE(nsHTMLOutputElement,
                                               nsGenericHTMLFormElement)
NS_HTML_CONTENT_INTERFACE_TABLE_TAIL_CLASSINFO(HTMLOutputElement)

NS_IMPL_ELEMENT_CLONE(nsHTMLOutputElement)


NS_IMPL_STRING_ATTR(nsHTMLOutputElement, Name, name)

NS_IMETHODIMP
nsHTMLOutputElement::Reset()
{
  mValueModeFlag = eModeDefault;
  nsresult rv = nsContentUtils::SetNodeTextContent(this, mDefaultValue,
                                                   PR_TRUE);
  return rv;
}

NS_IMETHODIMP
nsHTMLOutputElement::SubmitNamesValues(nsFormSubmission* aFormSubmission,
                                       nsIContent* aSubmitElement)
{
  
  return NS_OK;
}

PRBool
nsHTMLOutputElement::ParseAttribute(PRInt32 aNamespaceID, nsIAtom* aAttribute,
                                    const nsAString& aValue, nsAttrValue& aResult)
{
  if (aNamespaceID == kNameSpaceID_None) {
    if (aAttribute == nsGkAtoms::_for) {
      aResult.ParseAtomArray(aValue);
      return PR_TRUE;
    }
  }

  return nsGenericHTMLFormElement::ParseAttribute(aNamespaceID, aAttribute,
                                                  aValue, aResult);
}

NS_IMETHODIMP
nsHTMLOutputElement::GetForm(nsIDOMHTMLFormElement** aForm)
{
  return nsGenericHTMLFormElement::GetForm(aForm);
}

NS_IMETHODIMP
nsHTMLOutputElement::GetType(nsAString& aType)
{
  aType.AssignLiteral("output");
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLOutputElement::GetValue(nsAString& aValue)
{
  nsContentUtils::GetNodeTextContent(this, PR_TRUE, aValue);
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLOutputElement::SetValue(const nsAString& aValue)
{
  mValueModeFlag = eModeValue;
  return nsContentUtils::SetNodeTextContent(this, aValue, PR_TRUE);
}

NS_IMETHODIMP
nsHTMLOutputElement::GetDefaultValue(nsAString& aDefaultValue)
{
  aDefaultValue = mDefaultValue;
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLOutputElement::SetDefaultValue(const nsAString& aDefaultValue)
{
  mDefaultValue = aDefaultValue;
  if (mValueModeFlag == eModeDefault) {
    return nsContentUtils::SetNodeTextContent(this, mDefaultValue, PR_TRUE);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLOutputElement::GetHtmlFor(nsIDOMDOMSettableTokenList** aResult)
{
  if (!mTokenList) {
    mTokenList = new nsDOMSettableTokenList(this, nsGkAtoms::_for);
  }

  NS_ADDREF(*aResult = mTokenList);

  return NS_OK;
}

void nsHTMLOutputElement::DescendantsChanged()
{
  if (mValueModeFlag == eModeDefault) {
    nsContentUtils::GetNodeTextContent(this, PR_TRUE, mDefaultValue);
  }
}



void nsHTMLOutputElement::CharacterDataChanged(nsIDocument* aDocument,
                                               nsIContent* aContent,
                                               CharacterDataChangeInfo* aInfo)
{
  DescendantsChanged();
}

void nsHTMLOutputElement::ContentAppended(nsIDocument* aDocument,
                                          nsIContent* aContainer,
                                          nsIContent* aFirstNewContent,
                                          PRInt32 aNewIndexInContainer)
{
  DescendantsChanged();
}

void nsHTMLOutputElement::ContentInserted(nsIDocument* aDocument,
                                          nsIContent* aContainer,
                                          nsIContent* aChild,
                                          PRInt32 aIndexInContainer)
{
  DescendantsChanged();
}

void nsHTMLOutputElement::ContentRemoved(nsIDocument* aDocument,
                                         nsIContent* aContainer,
                                         nsIContent* aChild,
                                         PRInt32 aIndexInContainer,
                                         nsIContent* aPreviousSibling)
{
  DescendantsChanged();
}

