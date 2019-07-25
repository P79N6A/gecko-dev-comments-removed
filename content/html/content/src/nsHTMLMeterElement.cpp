






































 
#include "nsIDOMHTMLMeterElement.h"
#include "nsGenericHTMLElement.h"
#include "nsAttrValue.h"
#include "nsEventStateManager.h"
#include "nsAlgorithm.h"


class nsHTMLMeterElement : public nsGenericHTMLFormElement,
                           public nsIDOMHTMLMeterElement
{
public:
  nsHTMLMeterElement(already_AddRefed<nsINodeInfo> aNodeInfo);
  virtual ~nsHTMLMeterElement();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE(nsGenericHTMLFormElement::)

  
  NS_FORWARD_NSIDOMELEMENT(nsGenericHTMLFormElement::)

  
  NS_FORWARD_NSIDOMHTMLELEMENT(nsGenericHTMLFormElement::)

  
  NS_DECL_NSIDOMHTMLMETERELEMENT

  
  NS_IMETHOD_(PRUint32) GetType() const { return NS_FORM_METER; }
  NS_IMETHOD Reset();
  NS_IMETHOD SubmitNamesValues(nsFormSubmission* aFormSubmission);

  nsresult Clone(nsINodeInfo* aNodeInfo, nsINode** aResult) const;

  bool ParseAttribute(PRInt32 aNamespaceID, nsIAtom* aAttribute,
                      const nsAString& aValue, nsAttrValue& aResult);

  virtual nsXPCClassInfo* GetClassInfo();

  virtual nsIDOMNode* AsDOMNode() { return this; }

protected:

  static const double kDefaultValue;
  static const double kDefaultMin;
  static const double kDefaultMax;
};

const double nsHTMLMeterElement::kDefaultValue =  0.0;
const double nsHTMLMeterElement::kDefaultMin   =  0.0;
const double nsHTMLMeterElement::kDefaultMax   =  1.0;

NS_IMPL_NS_NEW_HTML_ELEMENT(Meter)


nsHTMLMeterElement::nsHTMLMeterElement(already_AddRefed<nsINodeInfo> aNodeInfo)
  : nsGenericHTMLFormElement(aNodeInfo)
{
}

nsHTMLMeterElement::~nsHTMLMeterElement()
{
}

NS_IMPL_ADDREF_INHERITED(nsHTMLMeterElement, nsGenericElement)
NS_IMPL_RELEASE_INHERITED(nsHTMLMeterElement, nsGenericElement)

DOMCI_NODE_DATA(HTMLMeterElement, nsHTMLMeterElement)

NS_INTERFACE_TABLE_HEAD(nsHTMLMeterElement)
  NS_HTML_CONTENT_INTERFACE_TABLE1(nsHTMLMeterElement,
                                   nsIDOMHTMLMeterElement)
  NS_HTML_CONTENT_INTERFACE_TABLE_TO_MAP_SEGUE(nsHTMLMeterElement,
                                               nsGenericHTMLFormElement)
NS_HTML_CONTENT_INTERFACE_TABLE_TAIL_CLASSINFO(HTMLMeterElement)

NS_IMPL_ELEMENT_CLONE(nsHTMLMeterElement)


NS_IMETHODIMP
nsHTMLMeterElement::Reset()
{
  
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLMeterElement::SubmitNamesValues(nsFormSubmission* aFormSubmission)
{
  
  return NS_OK;
}

bool
nsHTMLMeterElement::ParseAttribute(PRInt32 aNamespaceID, nsIAtom* aAttribute,
                                 const nsAString& aValue, nsAttrValue& aResult)
{
  if (aNamespaceID == kNameSpaceID_None) {
    if (aAttribute == nsGkAtoms::value || aAttribute == nsGkAtoms::max ||
        aAttribute == nsGkAtoms::min   || aAttribute == nsGkAtoms::low ||
        aAttribute == nsGkAtoms::high  || aAttribute == nsGkAtoms::optimum) {
      return aResult.ParseDoubleValue(aValue);
    }
  }

  return nsGenericHTMLFormElement::ParseAttribute(aNamespaceID, aAttribute,
                                                  aValue, aResult);
}

NS_IMETHODIMP
nsHTMLMeterElement::GetForm(nsIDOMHTMLFormElement** aForm)
{
  return nsGenericHTMLFormElement::GetForm(aForm);
}

NS_IMETHODIMP
nsHTMLMeterElement::GetMin(double* aValue)
{
  



  const nsAttrValue* attrMin = mAttrsAndChildren.GetAttr(nsGkAtoms::min);
  if (attrMin && attrMin->Type() == nsAttrValue::eDoubleValue) {
    *aValue = attrMin->GetDoubleValue();
    return NS_OK;
  }

  *aValue = kDefaultMin;
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLMeterElement::SetMin(double aValue)
{
  return SetDoubleAttr(nsGkAtoms::min, aValue);
}

NS_IMETHODIMP
nsHTMLMeterElement::GetMax(double* aValue)
{
  





  const nsAttrValue* attrMax = mAttrsAndChildren.GetAttr(nsGkAtoms::max);
  if (attrMax && attrMax->Type() == nsAttrValue::eDoubleValue) {
    *aValue = attrMax->GetDoubleValue();
  } else {
    *aValue = kDefaultMax;
  }

  double min;
  GetMin(&min);

  *aValue = NS_MAX(*aValue, min);

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLMeterElement::SetMax(double aValue)
{
  return SetDoubleAttr(nsGkAtoms::max, aValue);
}

NS_IMETHODIMP
nsHTMLMeterElement::GetValue(double* aValue)
{
  







  const nsAttrValue* attrValue = mAttrsAndChildren.GetAttr(nsGkAtoms::value);
  if (attrValue && attrValue->Type() == nsAttrValue::eDoubleValue) {
    *aValue = attrValue->GetDoubleValue();
  } else {
    *aValue = kDefaultValue;
  }

  double min;
  GetMin(&min);

  if (*aValue <= min) {
    *aValue = min;
    return NS_OK;
  }

  double max;
  GetMax(&max);

  *aValue = NS_MIN(*aValue, max);

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLMeterElement::SetValue(double aValue)
{
  return SetDoubleAttr(nsGkAtoms::value, aValue);
}

NS_IMETHODIMP
nsHTMLMeterElement::GetLow(double* aValue)
{
  








  double min;
  GetMin(&min);

  const nsAttrValue* attrLow = mAttrsAndChildren.GetAttr(nsGkAtoms::low);
  if (!attrLow || attrLow->Type() != nsAttrValue::eDoubleValue) {
    *aValue = min;
    return NS_OK;
  }

  *aValue = attrLow->GetDoubleValue();

  if (*aValue <= min) {
    *aValue = min;
    return NS_OK;
  }

  double max;
  GetMax(&max);

  *aValue = NS_MIN(*aValue, max);

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLMeterElement::SetLow(double aValue)
{
  return SetDoubleAttr(nsGkAtoms::low, aValue);
}

NS_IMETHODIMP
nsHTMLMeterElement::GetHigh(double* aValue)
{
  








  double max;
  GetMax(&max);

  const nsAttrValue* attrHigh = mAttrsAndChildren.GetAttr(nsGkAtoms::high);
  if (!attrHigh || attrHigh->Type() != nsAttrValue::eDoubleValue) {
    *aValue = max;
    return NS_OK;
  }

  *aValue = attrHigh->GetDoubleValue();

  if (*aValue >= max) {
    *aValue = max;
    return NS_OK;
  }

  double low;
  GetLow(&low);

  *aValue = NS_MAX(*aValue, low);

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLMeterElement::SetHigh(double aValue)
{
  return SetDoubleAttr(nsGkAtoms::high, aValue);
}

NS_IMETHODIMP
nsHTMLMeterElement::GetOptimum(double* aValue)
{
  










  double max;
  GetMax(&max);

  double min;
  GetMin(&min);

  const nsAttrValue* attrOptimum =
              mAttrsAndChildren.GetAttr(nsGkAtoms::optimum);
  if (!attrOptimum || attrOptimum->Type() != nsAttrValue::eDoubleValue) {
    *aValue = (min + max) / 2.0;
    return NS_OK;
  }

  *aValue = attrOptimum->GetDoubleValue();

  if (*aValue <= min) {
    *aValue = min;
    return NS_OK;
  }

  *aValue = NS_MIN(*aValue, max);

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLMeterElement::SetOptimum(double aValue)
{
  return SetDoubleAttr(nsGkAtoms::optimum, aValue);
}

