



































#include "nsSVGAnimatedBoolean.h"
#include "nsSVGValue.h"
#include "nsISVGValueUtils.h"
#include "nsDOMError.h"
#include "nsContentUtils.h"





class nsSVGAnimatedBoolean : public nsIDOMSVGAnimatedBoolean,
                             public nsSVGValue
{
protected:
  friend nsresult NS_NewSVGAnimatedBoolean(nsIDOMSVGAnimatedBoolean** result,
                                           PRBool aBaseVal);
  void Init(PRBool aBaseVal);

public:
  
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSIDOMSVGANIMATEDBOOLEAN

  
  NS_IMETHOD SetValueString(const nsAString& aValue);
  NS_IMETHOD GetValueString(nsAString& aValue);

protected:
  PRPackedBool mBaseVal;
};





void
nsSVGAnimatedBoolean::Init(PRBool aBaseVal)
{
  mBaseVal = aBaseVal;
}




NS_IMPL_ADDREF(nsSVGAnimatedBoolean)
NS_IMPL_RELEASE(nsSVGAnimatedBoolean)

NS_INTERFACE_MAP_BEGIN(nsSVGAnimatedBoolean)
  NS_INTERFACE_MAP_ENTRY(nsISVGValue)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGAnimatedBoolean)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(SVGAnimatedBoolean)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsISVGValue)
NS_INTERFACE_MAP_END




NS_IMETHODIMP
nsSVGAnimatedBoolean::SetValueString(const nsAString& aValue)
{
  nsresult rv = NS_OK;
  WillModify();

  if (aValue.EqualsLiteral("true"))
    mBaseVal = PR_TRUE;
  else if (aValue.EqualsLiteral("false"))
    mBaseVal = PR_FALSE;
  else
    rv = NS_ERROR_FAILURE;

  DidModify();
  return rv;
}

NS_IMETHODIMP
nsSVGAnimatedBoolean::GetValueString(nsAString& aValue)
{
  aValue.Assign(mBaseVal
                ? NS_LITERAL_STRING("true")
                : NS_LITERAL_STRING("false"));

  return NS_OK;
}





NS_IMETHODIMP
nsSVGAnimatedBoolean::GetBaseVal(PRBool *aBaseVal)
{
  *aBaseVal = mBaseVal;
  return NS_OK;
}


NS_IMETHODIMP
nsSVGAnimatedBoolean::SetBaseVal(PRBool aBaseVal)
{
  if (mBaseVal != aBaseVal &&
      (aBaseVal == PR_TRUE || aBaseVal == PR_FALSE)) {
    WillModify();
    mBaseVal = aBaseVal;
    DidModify();
  }
  return NS_OK;
}


NS_IMETHODIMP
nsSVGAnimatedBoolean::GetAnimVal(PRBool *aAnimVal)
{
  *aAnimVal = mBaseVal;
  return NS_OK;
}




nsresult
NS_NewSVGAnimatedBoolean(nsIDOMSVGAnimatedBoolean** aResult,
                         PRBool aBaseVal)
{
  *aResult = nsnull;

  nsSVGAnimatedBoolean* animatedBoolean = new nsSVGAnimatedBoolean();
  if (!animatedBoolean) return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(animatedBoolean);

  animatedBoolean->Init(aBaseVal);

  *aResult = (nsIDOMSVGAnimatedBoolean*) animatedBoolean;

  return NS_OK;
}
