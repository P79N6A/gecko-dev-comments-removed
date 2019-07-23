









































#include "nsSVGAnimatedInteger.h"
#include "nsTextFormatter.h"
#include "prdtoa.h"
#include "nsSVGValue.h"
#include "nsISVGValueUtils.h"
#include "nsDOMError.h"
#include "nsContentUtils.h"




class nsSVGAnimatedInteger : public nsIDOMSVGAnimatedInteger,
                             public nsSVGValue
{
protected:
  friend nsresult NS_NewSVGAnimatedInteger(nsIDOMSVGAnimatedInteger** result,
                                          PRInt32 aBaseVal);
  nsSVGAnimatedInteger();
  ~nsSVGAnimatedInteger();
  void Init(PRInt32 aBaseVal);
  
public:
  
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSIDOMSVGANIMATEDINTEGER

  
  NS_IMETHOD SetValueString(const nsAString& aValue);
  NS_IMETHOD GetValueString(nsAString& aValue);

protected:
  PRInt32 mBaseVal;
};






nsSVGAnimatedInteger::nsSVGAnimatedInteger()
{
}

nsSVGAnimatedInteger::~nsSVGAnimatedInteger()
{
}

void
nsSVGAnimatedInteger::Init(PRInt32 aBaseVal)
{
  mBaseVal = aBaseVal;
}




NS_IMPL_ADDREF(nsSVGAnimatedInteger)
NS_IMPL_RELEASE(nsSVGAnimatedInteger)


NS_INTERFACE_MAP_BEGIN(nsSVGAnimatedInteger)
  NS_INTERFACE_MAP_ENTRY(nsISVGValue)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGAnimatedInteger)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(SVGAnimatedInteger)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsISVGValue)
NS_INTERFACE_MAP_END
  



NS_IMETHODIMP
nsSVGAnimatedInteger::SetValueString(const nsAString& aValue)
{
  nsresult rv = NS_OK;
  WillModify();
  
  char *str = ToNewCString(aValue);

  if (*str) {
    char *tmp = str;

    

    if (*tmp != '-' &&
        *tmp != '+' &&
        !isdigit(*tmp))
      rv = NS_ERROR_FAILURE;

    tmp++;

    while (*tmp) {
      if (!isdigit(*tmp)) {
        rv = NS_ERROR_FAILURE;
        break;
      }
      tmp++;
    }

    if (NS_SUCCEEDED(rv))
      sscanf(str, "%d", &mBaseVal);
  }
  nsMemory::Free(str);
  DidModify();
  return rv;
}

NS_IMETHODIMP
nsSVGAnimatedInteger::GetValueString(nsAString& aValue)
{
  PRUnichar buf[24];
  nsTextFormatter::snprintf(buf, sizeof(buf)/sizeof(PRUnichar),
                            NS_LITERAL_STRING("%d").get(),
                            mBaseVal);
  aValue.Assign(buf);
  
  return NS_OK;
}





NS_IMETHODIMP
nsSVGAnimatedInteger::GetBaseVal(PRInt32 *aBaseVal)
{
  *aBaseVal = mBaseVal;
  return NS_OK;
}


NS_IMETHODIMP
nsSVGAnimatedInteger::SetBaseVal(PRInt32 aBaseVal)
{
  WillModify();
  mBaseVal = aBaseVal;
  DidModify();
  return NS_OK;
}


NS_IMETHODIMP
nsSVGAnimatedInteger::GetAnimVal(PRInt32 *aAnimVal)
{
  *aAnimVal = mBaseVal;
  return NS_OK;
}




nsresult
NS_NewSVGAnimatedInteger(nsIDOMSVGAnimatedInteger** aResult,
                         PRInt32 aBaseVal)
{
  *aResult = nsnull;
  
  nsSVGAnimatedInteger* animatedNumber = new nsSVGAnimatedInteger();
  if (!animatedNumber) return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(animatedNumber);

  animatedNumber->Init(aBaseVal);
  
  *aResult = (nsIDOMSVGAnimatedInteger*) animatedNumber;
  
  return NS_OK;
}


