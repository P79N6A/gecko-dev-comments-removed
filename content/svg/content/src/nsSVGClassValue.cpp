





































#include "nsSVGClassValue.h"
#include "nsContentUtils.h"







NS_IMPL_ADDREF(nsSVGClassValue)
NS_IMPL_RELEASE(nsSVGClassValue)

NS_INTERFACE_MAP_BEGIN(nsSVGClassValue)
  NS_INTERFACE_MAP_ENTRY(nsISVGValue)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGAnimatedString)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsISVGValue)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(SVGAnimatedString)
NS_INTERFACE_MAP_END
  



NS_IMETHODIMP
nsSVGClassValue::SetValueString(const nsAString& aValue)
{
  WillModify();
  mBaseVal.ParseAtomArray(aValue);
  DidModify();

  return NS_OK;
}

NS_IMETHODIMP
nsSVGClassValue::GetValueString(nsAString& aValue)
{
  mBaseVal.ToString(aValue);

  return NS_OK;
}





NS_IMETHODIMP
nsSVGClassValue::GetBaseVal(nsAString & aBaseVal)
{
  mBaseVal.ToString(aBaseVal);

  return NS_OK;
}
NS_IMETHODIMP
nsSVGClassValue::SetBaseVal(const nsAString & aBaseVal)
{
  SetValueString(aBaseVal);

  return NS_OK;
}


NS_IMETHODIMP
nsSVGClassValue::GetAnimVal(nsAString & aAnimVal)
{
  mBaseVal.ToString(aAnimVal);

  return NS_OK;
}
