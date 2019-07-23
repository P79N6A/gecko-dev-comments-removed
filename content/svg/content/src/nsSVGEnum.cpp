





































#include "nsSVGEnum.h"
#include "nsSVGValue.h"
#include "nsISVGValueUtils.h"
#include "nsWeakReference.h"
#include "nsIAtom.h"




class nsSVGEnum : public nsISVGEnum,
                  public nsSVGValue
{
protected:
  friend nsresult NS_NewSVGEnum(nsISVGEnum** result,
                                PRUint16 value,
                                nsSVGEnumMapping *mapping);
    
  friend nsresult NS_NewSVGEnum(nsISVGEnum** result,
                                const nsAString &value,
                                nsSVGEnumMapping *mapping);
  
  nsSVGEnum(PRUint16 value, nsSVGEnumMapping *mapping);
  nsSVGEnum(nsSVGEnumMapping *mapping);
  virtual ~nsSVGEnum();

public:
  
  NS_DECL_ISUPPORTS

  
  NS_IMETHOD GetIntegerValue(PRUint16 &value);
  NS_IMETHOD SetIntegerValue(PRUint16 value);
  
  
  NS_IMETHOD SetValueString(const nsAString& aValue);
  NS_IMETHOD GetValueString(nsAString& aValue);
  
#ifdef DEBUG_scooter
  void Print_mapping();
#endif
  
protected:
  PRUint16 mValue;
  nsSVGEnumMapping *mMapping;
};





#ifdef DEBUG_scooter
void nsSVGEnum::Print_mapping()
{
  nsSVGEnumMapping *tmp = mMapping;
  nsAutoString aStr;
  printf("Print_mapping: mMapping = 0x%x\n", tmp);
  while (tmp->key) {
    (*tmp->key)->ToString(aStr);
    printf ("Print_mapping: %s (%d)\n", NS_ConvertUTF16toUTF8(aStr).get(), tmp->val);
    tmp++;
  }
}
#endif

nsresult
NS_NewSVGEnum(nsISVGEnum** result,
              PRUint16 value,
              nsSVGEnumMapping *mapping)
{
  NS_ASSERTION(mapping, "no mapping");
  nsSVGEnum *pe = new nsSVGEnum(value, mapping);
  if (!pe) return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(pe);
  *result = pe;
  return NS_OK;
}

nsresult
NS_NewSVGEnum(nsISVGEnum** result,
              const nsAString &value,
              nsSVGEnumMapping *mapping)
{
  NS_ASSERTION(mapping, "no mapping");
  *result = nsnull;
  nsSVGEnum *pe = new nsSVGEnum(0, mapping);
  if (!pe) return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(pe);
  if (NS_FAILED(pe->SetValueString(value))) {
    NS_RELEASE(pe);
    return NS_ERROR_FAILURE;
  }
  *result = pe;
  return NS_OK;
}  


nsSVGEnum::nsSVGEnum(PRUint16 value,
                     nsSVGEnumMapping *mapping)
    : mValue(value), mMapping(mapping)
{
}

nsSVGEnum::~nsSVGEnum()
{
}




NS_IMPL_ADDREF(nsSVGEnum)
NS_IMPL_RELEASE(nsSVGEnum)

NS_INTERFACE_MAP_BEGIN(nsSVGEnum)
  NS_INTERFACE_MAP_ENTRY(nsISVGValue)
  NS_INTERFACE_MAP_ENTRY(nsISVGEnum)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsISVGValue)
NS_INTERFACE_MAP_END



NS_IMETHODIMP
nsSVGEnum::SetValueString(const nsAString& aValue)
{
  nsCOMPtr<nsIAtom> valAtom = do_GetAtom(aValue);

  nsSVGEnumMapping *tmp = mMapping;

  while (tmp->key) {
    if (valAtom == *(tmp->key)) {
      WillModify();
      mValue = tmp->val;
      DidModify();
      return NS_OK;
    }
    tmp++;
  }

  
  NS_WARNING("unknown enumeration key");
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsSVGEnum::GetValueString(nsAString& aValue)
{
  nsSVGEnumMapping *tmp = mMapping;

  while (tmp->key) {
    if (mValue == tmp->val) {
      (*tmp->key)->ToString(aValue);
      return NS_OK;
    }
    tmp++;
  }
  NS_ERROR("unknown enumeration value");
  return NS_ERROR_FAILURE;
}




NS_IMETHODIMP
nsSVGEnum::GetIntegerValue(PRUint16& aValue)
{
  aValue = mValue;
  return NS_OK;
}

NS_IMETHODIMP
nsSVGEnum::SetIntegerValue(PRUint16 aValue)
{
  WillModify();
  mValue = aValue;
  DidModify();
  return NS_OK;
}


