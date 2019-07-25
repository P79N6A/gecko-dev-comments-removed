



































#include "nsIDOMHTMLFieldSetElement.h"
#include "nsIDOMHTMLFormElement.h"
#include "nsIDOMEventTarget.h"
#include "nsGenericHTMLElement.h"
#include "nsStyleConsts.h"
#include "nsIForm.h"
#include "nsIFormControl.h"
#include "nsIConstraintValidation.h"
#include "nsEventDispatcher.h"


class nsHTMLFieldSetElement : public nsGenericHTMLFormElement,
                              public nsIDOMHTMLFieldSetElement,
                              public nsIConstraintValidation
{
public:
  using nsIConstraintValidation::GetValidationMessage;

  nsHTMLFieldSetElement(already_AddRefed<nsINodeInfo> aNodeInfo);
  virtual ~nsHTMLFieldSetElement();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE(nsGenericHTMLFormElement::)

  
  NS_FORWARD_NSIDOMELEMENT(nsGenericHTMLFormElement::)

  
  NS_FORWARD_NSIDOMHTMLELEMENT(nsGenericHTMLFormElement::)

  
  NS_DECL_NSIDOMHTMLFIELDSETELEMENT

  
  virtual nsresult PreHandleEvent(nsEventChainPreVisitor& aVisitor);
  virtual nsresult AfterSetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                                const nsAString* aValue, PRBool aNotify);

  
  NS_IMETHOD_(PRUint32) GetType() const { return NS_FORM_FIELDSET; }
  NS_IMETHOD Reset();
  NS_IMETHOD SubmitNamesValues(nsFormSubmission* aFormSubmission);
  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;
  virtual nsXPCClassInfo* GetClassInfo();

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(nsHTMLFieldSetElement,
                                           nsGenericHTMLFormElement)
private:

  
  static PRBool MatchListedElements(nsIContent* aContent, PRInt32 aNamespaceID,
                                    nsIAtom* aAtom, void* aData);

  
  nsRefPtr<nsContentList> mElements;
};




NS_IMPL_NS_NEW_HTML_ELEMENT(FieldSet)


nsHTMLFieldSetElement::nsHTMLFieldSetElement(already_AddRefed<nsINodeInfo> aNodeInfo)
  : nsGenericHTMLFormElement(aNodeInfo)
  , mElements(nsnull)
{
  
  SetBarredFromConstraintValidation(PR_TRUE);
}

nsHTMLFieldSetElement::~nsHTMLFieldSetElement()
{
}



NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(nsHTMLFieldSetElement)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mElements)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_CLASS(nsHTMLFieldSetElement)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(nsHTMLFieldSetElement,
                                                  nsGenericHTMLFormElement)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR_AMBIGUOUS(mElements, nsIDOMNodeList)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_ADDREF_INHERITED(nsHTMLFieldSetElement, nsGenericElement)
NS_IMPL_RELEASE_INHERITED(nsHTMLFieldSetElement, nsGenericElement)

DOMCI_NODE_DATA(HTMLFieldSetElement, nsHTMLFieldSetElement)


NS_INTERFACE_TABLE_HEAD(nsHTMLFieldSetElement)
  NS_HTML_CONTENT_INTERFACE_TABLE2(nsHTMLFieldSetElement,
                                   nsIDOMHTMLFieldSetElement,
                                   nsIConstraintValidation)
  NS_HTML_CONTENT_INTERFACE_TABLE_TO_MAP_SEGUE(nsHTMLFieldSetElement,
                                               nsGenericHTMLFormElement)
NS_HTML_CONTENT_INTERFACE_TABLE_TAIL_CLASSINFO(HTMLFieldSetElement)

NS_IMPL_ELEMENT_CLONE(nsHTMLFieldSetElement)


NS_IMPL_BOOL_ATTR(nsHTMLFieldSetElement, Disabled, disabled)
NS_IMPL_STRING_ATTR(nsHTMLFieldSetElement, Name, name)


NS_IMPL_NSICONSTRAINTVALIDATION(nsHTMLFieldSetElement)


nsresult
nsHTMLFieldSetElement::PreHandleEvent(nsEventChainPreVisitor& aVisitor)
{
  
  aVisitor.mCanHandle = PR_FALSE;
  if (IsDisabled()) {
    return NS_OK;
  }

  return nsGenericHTMLFormElement::PreHandleEvent(aVisitor);
}

nsresult
nsHTMLFieldSetElement::AfterSetAttr(PRInt32 aNameSpaceID, nsIAtom* aName,
                                    const nsAString* aValue, PRBool aNotify)
{
  if (aNameSpaceID == kNameSpaceID_None && aName == nsGkAtoms::disabled &&
      nsINode::GetFirstChild()) {
    if (!mElements) {
      mElements = new nsContentList(this, MatchListedElements, nsnull, nsnull,
                                    PR_TRUE);
    }

    PRUint32 length = mElements->Length(PR_TRUE);
    for (PRUint32 i=0; i<length; ++i) {
      static_cast<nsGenericHTMLFormElement*>(mElements->GetNodeAt(i))
        ->OnFieldSetDisabledChanged(0);
    }
  }

  return nsGenericHTMLFormElement::AfterSetAttr(aNameSpaceID, aName,
                                                aValue, aNotify);
}



NS_IMETHODIMP
nsHTMLFieldSetElement::GetForm(nsIDOMHTMLFormElement** aForm)
{
  return nsGenericHTMLFormElement::GetForm(aForm);
}

NS_IMETHODIMP
nsHTMLFieldSetElement::GetType(nsAString& aType)
{
  aType.AssignLiteral("fieldset");
  return NS_OK;
}


PRBool
nsHTMLFieldSetElement::MatchListedElements(nsIContent* aContent, PRInt32 aNamespaceID,
                                           nsIAtom* aAtom, void* aData)
{
  nsCOMPtr<nsIFormControl> formControl = do_QueryInterface(aContent);
  return formControl && formControl->GetType() != NS_FORM_LABEL;
}

NS_IMETHODIMP
nsHTMLFieldSetElement::GetElements(nsIDOMHTMLCollection** aElements)
{
  if (!mElements) {
    mElements = new nsContentList(this, MatchListedElements, nsnull, nsnull,
                                  PR_TRUE);
  }

  NS_ADDREF(*aElements = mElements);
  return NS_OK;
}



nsresult
nsHTMLFieldSetElement::Reset()
{
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLFieldSetElement::SubmitNamesValues(nsFormSubmission* aFormSubmission)
{
  return NS_OK;
}
