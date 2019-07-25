



































#include "nsIDOMHTMLFieldSetElement.h"
#include "nsIDOMHTMLFormElement.h"
#include "nsIDOMEventTarget.h"
#include "nsGenericHTMLElement.h"
#include "nsStyleConsts.h"
#include "nsIForm.h"
#include "nsIFormControl.h"
#include "nsConstraintValidation.h"


class nsHTMLFieldSetElement : public nsGenericHTMLFormElement,
                              public nsIDOMHTMLFieldSetElement,
                              public nsConstraintValidation
{
public:
  nsHTMLFieldSetElement(already_AddRefed<nsINodeInfo> aNodeInfo);
  virtual ~nsHTMLFieldSetElement();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE(nsGenericHTMLFormElement::)

  
  NS_FORWARD_NSIDOMELEMENT(nsGenericHTMLFormElement::)

  
  NS_FORWARD_NSIDOMHTMLELEMENT(nsGenericHTMLFormElement::)

  
  NS_DECL_NSIDOMHTMLFIELDSETELEMENT

  
  NS_IMETHOD_(PRUint32) GetType() const { return NS_FORM_FIELDSET; }
  NS_IMETHOD Reset();
  NS_IMETHOD SubmitNamesValues(nsFormSubmission* aFormSubmission,
                               nsIContent* aSubmitElement);
  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;
  virtual nsXPCClassInfo* GetClassInfo();

  
  PRBool IsBarredFromConstraintValidation() { return PR_TRUE; };
};




NS_IMPL_NS_NEW_HTML_ELEMENT(FieldSet)


nsHTMLFieldSetElement::nsHTMLFieldSetElement(already_AddRefed<nsINodeInfo> aNodeInfo)
  : nsGenericHTMLFormElement(aNodeInfo)
{
}

nsHTMLFieldSetElement::~nsHTMLFieldSetElement()
{
}



NS_IMPL_ADDREF_INHERITED(nsHTMLFieldSetElement, nsGenericElement)
NS_IMPL_RELEASE_INHERITED(nsHTMLFieldSetElement, nsGenericElement)

DOMCI_NODE_DATA(HTMLFieldSetElement, nsHTMLFieldSetElement)


NS_INTERFACE_TABLE_HEAD(nsHTMLFieldSetElement)
  NS_HTML_CONTENT_INTERFACE_TABLE1(nsHTMLFieldSetElement,
                                   nsIDOMHTMLFieldSetElement)
  NS_HTML_CONTENT_INTERFACE_TABLE_TO_MAP_SEGUE(nsHTMLFieldSetElement,
                                               nsGenericHTMLFormElement)
NS_HTML_CONTENT_INTERFACE_TABLE_TAIL_CLASSINFO(HTMLFieldSetElement)





NS_IMPL_NSCONSTRAINTVALIDATION(nsHTMLFieldSetElement)

NS_IMPL_ELEMENT_CLONE(nsHTMLFieldSetElement)




NS_IMETHODIMP
nsHTMLFieldSetElement::GetForm(nsIDOMHTMLFormElement** aForm)
{
  return nsGenericHTMLFormElement::GetForm(aForm);
}



nsresult
nsHTMLFieldSetElement::Reset()
{
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLFieldSetElement::SubmitNamesValues(nsFormSubmission* aFormSubmission,
                                         nsIContent* aSubmitElement)
{
  return NS_OK;
}
