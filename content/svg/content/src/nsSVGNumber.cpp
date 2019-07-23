





































#include "nsSVGNumber.h"
#include "nsTextFormatter.h"
#include "prdtoa.h"
#include "nsSVGValue.h"
#include "nsISVGValueUtils.h"
#include "nsContentUtils.h"




class nsSVGNumber : public nsIDOMSVGNumber,
                    public nsSVGValue
{
protected:
  friend nsresult NS_NewSVGNumber(nsIDOMSVGNumber** result,
                                  float val);
  nsSVGNumber(float val);
  
public:
  
  NS_DECL_ISUPPORTS

  
  NS_DECL_NSIDOMSVGNUMBER

  
  NS_IMETHOD SetValueString(const nsAString& aValue);
  NS_IMETHOD GetValueString(nsAString& aValue);
  
protected:
  float mValue;
};




nsresult
NS_NewSVGNumber(nsIDOMSVGNumber** result, float val)
{
  *result = new nsSVGNumber(val);
  if (!*result) return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(*result);
  return NS_OK;
}

nsSVGNumber::nsSVGNumber(float val)
    : mValue(val)
{
}




NS_IMPL_ADDREF(nsSVGNumber)
NS_IMPL_RELEASE(nsSVGNumber)

NS_INTERFACE_MAP_BEGIN(nsSVGNumber)
  NS_INTERFACE_MAP_ENTRY(nsISVGValue)
  NS_INTERFACE_MAP_ENTRY(nsIDOMSVGNumber)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(SVGNumber)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsISVGValue)
NS_INTERFACE_MAP_END




NS_IMETHODIMP
nsSVGNumber::GetValueString(nsAString& aValue)
{
  PRUnichar buf[24];
  nsTextFormatter::snprintf(buf, sizeof(buf)/sizeof(PRUnichar),
                            NS_LITERAL_STRING("%g").get(),
                            (double)mValue);
  aValue.Assign(buf);
  
  return NS_OK;
}

NS_IMETHODIMP
nsSVGNumber::SetValueString(const nsAString& aValue)
{
  nsresult rv = NS_OK;
  WillModify();
  
  NS_ConvertUTF16toUTF8 value(aValue);
  const char *str = value.get();

  if (*str) {
    char *rest;
    float val = float(PR_strtod(str, &rest));
    if (rest && rest!=str && NS_FloatIsFinite(val)) {
      if (*rest=='%') {
        rv = SetValue(val / 100.0f);
        rest++;
      } else {
        rv = SetValue(val);
      }
      
      while (*rest && isspace(*rest))
        ++rest;

      
      if (*rest != '\0') {
        rv = NS_ERROR_FAILURE;
        NS_ERROR("trailing data in number value");
      }
    } else {
      rv = NS_ERROR_FAILURE;
      
    }
  }
  DidModify();
  return rv;
}







NS_IMETHODIMP nsSVGNumber::GetValue(float *aValue)
{
  *aValue = mValue;
  return NS_OK;
}
NS_IMETHODIMP nsSVGNumber::SetValue(float aValue)
{
  NS_ENSURE_FINITE(aValue, NS_ERROR_ILLEGAL_VALUE);
  WillModify();
  mValue = aValue;
  DidModify();
  return NS_OK;
}
