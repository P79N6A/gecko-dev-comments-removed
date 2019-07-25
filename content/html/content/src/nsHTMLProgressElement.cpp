




































#include "nsIDOMHTMLProgressElement.h"
#include "nsGenericHTMLElement.h"
#include "nsAttrValue.h"


class nsHTMLProgressElement : public nsGenericHTMLFormElement,
                              public nsIDOMHTMLProgressElement
{
public:
  nsHTMLProgressElement(already_AddRefed<nsINodeInfo> aNodeInfo);
  virtual ~nsHTMLProgressElement();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE(nsGenericHTMLFormElement::)

  
  NS_FORWARD_NSIDOMELEMENT(nsGenericHTMLFormElement::)

  
  NS_FORWARD_NSIDOMHTMLELEMENT(nsGenericHTMLFormElement::)

  
  NS_DECL_NSIDOMHTMLPROGRESSELEMENT

  
  NS_IMETHOD_(PRUint32) GetType() const { return NS_FORM_PROGRESS; }
  NS_IMETHOD Reset();
  NS_IMETHOD SubmitNamesValues(nsFormSubmission* aFormSubmission);

  nsresult Clone(nsINodeInfo* aNodeInfo, nsINode** aResult) const;

  PRBool ParseAttribute(PRInt32 aNamespaceID, nsIAtom* aAttribute,
                        const nsAString& aValue, nsAttrValue& aResult);

  virtual nsXPCClassInfo* GetClassInfo();

protected:
  






  bool IsIndeterminate() const;

  static const double kIndeterminatePosition;
  static const double kDefaultValue;
  static const double kDefaultMax;
};

const double nsHTMLProgressElement::kIndeterminatePosition = -1.0;
const double nsHTMLProgressElement::kDefaultValue          =  0.0;
const double nsHTMLProgressElement::kDefaultMax            =  1.0;

NS_IMPL_NS_NEW_HTML_ELEMENT(Progress)


nsHTMLProgressElement::nsHTMLProgressElement(already_AddRefed<nsINodeInfo> aNodeInfo)
  : nsGenericHTMLFormElement(aNodeInfo)
{
}

nsHTMLProgressElement::~nsHTMLProgressElement()
{
}

NS_IMPL_ADDREF_INHERITED(nsHTMLProgressElement, nsGenericElement)
NS_IMPL_RELEASE_INHERITED(nsHTMLProgressElement, nsGenericElement)

DOMCI_NODE_DATA(HTMLProgressElement, nsHTMLProgressElement)

NS_INTERFACE_TABLE_HEAD(nsHTMLProgressElement)
  NS_HTML_CONTENT_INTERFACE_TABLE1(nsHTMLProgressElement,
                                   nsIDOMHTMLProgressElement)
  NS_HTML_CONTENT_INTERFACE_TABLE_TO_MAP_SEGUE(nsHTMLProgressElement,
                                               nsGenericHTMLFormElement)
NS_HTML_CONTENT_INTERFACE_TABLE_TAIL_CLASSINFO(HTMLProgressElement)

NS_IMPL_ELEMENT_CLONE(nsHTMLProgressElement)


NS_IMETHODIMP
nsHTMLProgressElement::Reset()
{
  
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLProgressElement::SubmitNamesValues(nsFormSubmission* aFormSubmission)
{
  
  return NS_OK;
}

PRBool
nsHTMLProgressElement::ParseAttribute(PRInt32 aNamespaceID, nsIAtom* aAttribute,
                                      const nsAString& aValue, nsAttrValue& aResult)
{
  if (aNamespaceID == kNameSpaceID_None) {
    if (aAttribute == nsGkAtoms::value || aAttribute == nsGkAtoms::max) {
      return aResult.ParseDoubleValue(aValue);
    }
  }

  return nsGenericHTMLFormElement::ParseAttribute(aNamespaceID, aAttribute,
                                                  aValue, aResult);
}

NS_IMETHODIMP
nsHTMLProgressElement::GetForm(nsIDOMHTMLFormElement** aForm)
{
  return nsGenericHTMLFormElement::GetForm(aForm);
}

NS_IMETHODIMP
nsHTMLProgressElement::GetValue(double* aValue)
{
  const nsAttrValue* attrValue = mAttrsAndChildren.GetAttr(nsGkAtoms::value);
  if (!attrValue || attrValue->Type() != nsAttrValue::eDoubleValue ||
      attrValue->GetDoubleValue() < 0.0) {
    *aValue = kDefaultValue;
    return NS_OK;
  }

  *aValue = attrValue->GetDoubleValue();

  double max;
  GetMax(&max);

  *aValue = PR_MIN(*aValue, max);

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLProgressElement::SetValue(double aValue)
{
  return SetDoubleAttr(nsGkAtoms::value, aValue);
}

NS_IMETHODIMP
nsHTMLProgressElement::GetMax(double* aValue)
{
  const nsAttrValue* attrMax = mAttrsAndChildren.GetAttr(nsGkAtoms::max);
  if (attrMax && attrMax->Type() == nsAttrValue::eDoubleValue &&
      attrMax->GetDoubleValue() > 0.0) {
    *aValue = attrMax->GetDoubleValue();
  } else {
    *aValue = kDefaultMax;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLProgressElement::SetMax(double aValue)
{
  return SetDoubleAttr(nsGkAtoms::max, aValue);
}

NS_IMETHODIMP
nsHTMLProgressElement::GetPosition(double* aPosition)
{
  if (IsIndeterminate()) {
    *aPosition = kIndeterminatePosition;
    return NS_OK;
  }

  double value;
  double max;
  GetValue(&value);
  GetMax(&max);

  *aPosition = value / max;

  return NS_OK;
}

bool
nsHTMLProgressElement::IsIndeterminate() const
{
  const nsAttrValue* attrValue = mAttrsAndChildren.GetAttr(nsGkAtoms::value);
  return !attrValue || attrValue->Type() != nsAttrValue::eDoubleValue;
}

