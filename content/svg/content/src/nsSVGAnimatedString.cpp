





































#include "nsSVGValue.h"
#include "nsWeakReference.h"
#include "nsSVGAnimatedString.h"
#include "nsContentUtils.h"







NS_IMPL_ADDREF(nsSVGAnimatedString)
NS_IMPL_RELEASE(nsSVGAnimatedString)


NS_INTERFACE_MAP_BEGIN(nsSVGAnimatedString)
  NS_INTERFACE_MAP_ENTRY(nsISVGValue)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGAnimatedString)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(SVGAnimatedString)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsISVGValue)
NS_INTERFACE_MAP_END
  



NS_IMETHODIMP
nsSVGAnimatedString::SetValueString(const nsAString& aValue)
{
  WillModify();
  mBaseVal = aValue;
  DidModify();
  return NS_OK;
}

NS_IMETHODIMP
nsSVGAnimatedString::GetValueString(nsAString& aValue)
{
  aValue = mBaseVal;
  return NS_OK;
}






NS_IMETHODIMP
nsSVGAnimatedString::GetBaseVal(nsAString & aBaseVal)
{
  aBaseVal = mBaseVal;
  return NS_OK;
}
NS_IMETHODIMP
nsSVGAnimatedString::SetBaseVal(const nsAString & aBaseVal)
{
  SetValueString(aBaseVal);
  return NS_OK;
}


NS_IMETHODIMP
nsSVGAnimatedString::GetAnimVal(nsAString & aAnimVal)
{
  aAnimVal = mBaseVal;
  return NS_OK;
}


NS_IMETHODIMP
nsSVGAnimatedString::Clear()
{
  mBaseVal.Truncate();
  return NS_OK;
}





nsresult
NS_NewSVGAnimatedString(nsIDOMSVGAnimatedString** aResult)
{
  *aResult = nsnull;
  
  nsSVGAnimatedString* animatedString = new nsSVGAnimatedString();
  if(!animatedString) return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(animatedString);

  *aResult = (nsIDOMSVGAnimatedString*) animatedString;
  
  return NS_OK;
}
