



































#include "nsIDOMHTMLFieldSetElement.h"
#include "nsIDOMHTMLFormElement.h"
#include "nsIDOMEventTarget.h"
#include "nsGenericHTMLElement.h"
#include "nsStyleConsts.h"
#include "nsPresContext.h"
#include "nsIForm.h"
#include "nsIFormControl.h"


class nsHTMLFieldSetElement : public nsGenericHTMLFormElement,
                              public nsIDOMHTMLFieldSetElement
{
public:
  nsHTMLFieldSetElement(nsINodeInfo *aNodeInfo);
  virtual ~nsHTMLFieldSetElement();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE(nsGenericHTMLFormElement::)

  
  NS_FORWARD_NSIDOMELEMENT(nsGenericHTMLFormElement::)

  
  NS_FORWARD_NSIDOMHTMLELEMENT(nsGenericHTMLFormElement::)

  
  NS_DECL_NSIDOMHTMLFIELDSETELEMENT

  
  NS_IMETHOD_(PRInt32) GetType() const { return NS_FORM_FIELDSET; }
  NS_IMETHOD Reset();
  NS_IMETHOD SubmitNamesValues(nsIFormSubmission* aFormSubmission,
                               nsIContent* aSubmitElement);
  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;
};




NS_IMPL_NS_NEW_HTML_ELEMENT(FieldSet)


nsHTMLFieldSetElement::nsHTMLFieldSetElement(nsINodeInfo *aNodeInfo)
  : nsGenericHTMLFormElement(aNodeInfo)
{
}

nsHTMLFieldSetElement::~nsHTMLFieldSetElement()
{
}



NS_IMPL_ADDREF_INHERITED(nsHTMLFieldSetElement, nsGenericElement)
NS_IMPL_RELEASE_INHERITED(nsHTMLFieldSetElement, nsGenericElement)



NS_HTML_CONTENT_INTERFACE_TABLE_HEAD(nsHTMLFieldSetElement,
                                     nsGenericHTMLFormElement)
  NS_INTERFACE_TABLE_INHERITED1(nsHTMLFieldSetElement,
                                nsIDOMHTMLFieldSetElement)
NS_HTML_CONTENT_INTERFACE_TABLE_TAIL_CLASSINFO(HTMLFieldSetElement)





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
nsHTMLFieldSetElement::SubmitNamesValues(nsIFormSubmission* aFormSubmission,
                                         nsIContent* aSubmitElement)
{
  return NS_OK;
}
