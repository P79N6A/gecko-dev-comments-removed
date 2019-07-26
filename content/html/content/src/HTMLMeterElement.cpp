




#include "HTMLMeterElement.h"
#include "mozilla/dom/HTMLMeterElementBinding.h"

NS_IMPL_NS_NEW_HTML_ELEMENT(Meter)

namespace mozilla {
namespace dom {

const double HTMLMeterElement::kDefaultValue =  0.0;
const double HTMLMeterElement::kDefaultMin   =  0.0;
const double HTMLMeterElement::kDefaultMax   =  1.0;


HTMLMeterElement::HTMLMeterElement(already_AddRefed<nsINodeInfo> aNodeInfo)
  : nsGenericHTMLElement(aNodeInfo)
{
  SetIsDOMBinding();
}

HTMLMeterElement::~HTMLMeterElement()
{
}

NS_IMPL_ADDREF_INHERITED(HTMLMeterElement, Element)
NS_IMPL_RELEASE_INHERITED(HTMLMeterElement, Element)


NS_INTERFACE_TABLE_HEAD(HTMLMeterElement)
  NS_HTML_CONTENT_INTERFACE_TABLE1(HTMLMeterElement,
                                   nsIDOMHTMLMeterElement)
  NS_HTML_CONTENT_INTERFACE_TABLE_TO_MAP_SEGUE(HTMLMeterElement,
                                               nsGenericHTMLElement)
NS_HTML_CONTENT_INTERFACE_MAP_END

NS_IMPL_ELEMENT_CLONE(HTMLMeterElement)


nsEventStates
HTMLMeterElement::IntrinsicState() const
{
  nsEventStates state = nsGenericHTMLElement::IntrinsicState();

  state |= GetOptimumState();

  return state;
}

bool
HTMLMeterElement::ParseAttribute(int32_t aNamespaceID, nsIAtom* aAttribute,
                                 const nsAString& aValue, nsAttrValue& aResult)
{
  if (aNamespaceID == kNameSpaceID_None) {
    if (aAttribute == nsGkAtoms::value || aAttribute == nsGkAtoms::max ||
        aAttribute == nsGkAtoms::min   || aAttribute == nsGkAtoms::low ||
        aAttribute == nsGkAtoms::high  || aAttribute == nsGkAtoms::optimum) {
      return aResult.ParseDoubleValue(aValue);
    }
  }

  return nsGenericHTMLElement::ParseAttribute(aNamespaceID, aAttribute,
                                                  aValue, aResult);
}






double
HTMLMeterElement::Min() const
{
  



  const nsAttrValue* attrMin = mAttrsAndChildren.GetAttr(nsGkAtoms::min);
  if (attrMin && attrMin->Type() == nsAttrValue::eDoubleValue) {
    return attrMin->GetDoubleValue();
  }
  return kDefaultMin;
}

double
HTMLMeterElement::Max() const
{
  





  double max;

  const nsAttrValue* attrMax = mAttrsAndChildren.GetAttr(nsGkAtoms::max);
  if (attrMax && attrMax->Type() == nsAttrValue::eDoubleValue) {
    max = attrMax->GetDoubleValue();
  } else {
    max = kDefaultMax;
  }

  return std::max(max, Min());
}

double
HTMLMeterElement::Value() const
{
  







  double value;

  const nsAttrValue* attrValue = mAttrsAndChildren.GetAttr(nsGkAtoms::value);
  if (attrValue && attrValue->Type() == nsAttrValue::eDoubleValue) {
    value = attrValue->GetDoubleValue();
  } else {
    value = kDefaultValue;
  }

  double min = Min();

  if (value <= min) {
    return min;
  }

  return std::min(value, Max());
}

double
HTMLMeterElement::Low() const
{
  








  double min = Min();

  const nsAttrValue* attrLow = mAttrsAndChildren.GetAttr(nsGkAtoms::low);
  if (!attrLow || attrLow->Type() != nsAttrValue::eDoubleValue) {
    return min;
  }

  double low = attrLow->GetDoubleValue();

  if (low <= min) {
    return min;
  }

  return std::min(low, Max());
}

double
HTMLMeterElement::High() const
{
  








  double max = Max();

  const nsAttrValue* attrHigh = mAttrsAndChildren.GetAttr(nsGkAtoms::high);
  if (!attrHigh || attrHigh->Type() != nsAttrValue::eDoubleValue) {
    return max;
  }

  double high = attrHigh->GetDoubleValue();

  if (high >= max) {
    return max;
  }

  return std::max(high, Low());
}

double
HTMLMeterElement::Optimum() const
{
  










  double max = Max();

  double min = Min();

  const nsAttrValue* attrOptimum =
              mAttrsAndChildren.GetAttr(nsGkAtoms::optimum);
  if (!attrOptimum || attrOptimum->Type() != nsAttrValue::eDoubleValue) {
    return (min + max) / 2.0;
  }

  double optimum = attrOptimum->GetDoubleValue();

  if (optimum <= min) {
    return min;
  }

  return std::min(optimum, max);
}





NS_IMETHODIMP
HTMLMeterElement::GetMin(double* aValue)
{
  *aValue = Min();
  return NS_OK;
}

NS_IMETHODIMP
HTMLMeterElement::SetMin(double aValue)
{
  return SetDoubleAttr(nsGkAtoms::min, aValue);
}

NS_IMETHODIMP
HTMLMeterElement::GetMax(double* aValue)
{
  *aValue = Max();
  return NS_OK;
}

NS_IMETHODIMP
HTMLMeterElement::SetMax(double aValue)
{
  return SetDoubleAttr(nsGkAtoms::max, aValue);
}

NS_IMETHODIMP
HTMLMeterElement::GetValue(double* aValue)
{
  *aValue = Value();
  return NS_OK;
}

NS_IMETHODIMP
HTMLMeterElement::SetValue(double aValue)
{
  return SetDoubleAttr(nsGkAtoms::value, aValue);
}

NS_IMETHODIMP
HTMLMeterElement::GetLow(double* aValue)
{
  *aValue = Low();
  return NS_OK;
}

NS_IMETHODIMP
HTMLMeterElement::SetLow(double aValue)
{
  return SetDoubleAttr(nsGkAtoms::low, aValue);
}

NS_IMETHODIMP
HTMLMeterElement::GetHigh(double* aValue)
{
  *aValue = High();
  return NS_OK;
}

NS_IMETHODIMP
HTMLMeterElement::SetHigh(double aValue)
{
  return SetDoubleAttr(nsGkAtoms::high, aValue);
}

NS_IMETHODIMP
HTMLMeterElement::GetOptimum(double* aValue)
{
  *aValue = Optimum();
  return NS_OK;
}

NS_IMETHODIMP
HTMLMeterElement::SetOptimum(double aValue)
{
  return SetDoubleAttr(nsGkAtoms::optimum, aValue);
}

nsEventStates
HTMLMeterElement::GetOptimumState() const
{
  









  double value = Value();
  double low = Low();
  double high = High();
  double optimum = Optimum();

  if (optimum < low) {
    if (value < low) {
      return NS_EVENT_STATE_OPTIMUM;
    }
    if (value <= high) {
      return NS_EVENT_STATE_SUB_OPTIMUM;
    }
    return NS_EVENT_STATE_SUB_SUB_OPTIMUM;
  }
  if (optimum > high) {
    if (value > high) {
      return NS_EVENT_STATE_OPTIMUM;
    }
    if (value >= low) {
      return NS_EVENT_STATE_SUB_OPTIMUM;
    }
    return NS_EVENT_STATE_SUB_SUB_OPTIMUM;
  }
  
  if (value >= low && value <= high) {
    return NS_EVENT_STATE_OPTIMUM;
  }
  return NS_EVENT_STATE_SUB_OPTIMUM;
}

JSObject*
HTMLMeterElement::WrapNode(JSContext* aCx, JSObject* aScope)
{
  return HTMLMeterElementBinding::Wrap(aCx, aScope, this);
}

} 
} 
