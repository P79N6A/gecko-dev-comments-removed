




#include "nscore.h"
#include "nsISupports.h"
#include "nsCategoryImp.h"
#include "nsUnicodeProperties.h"

NS_IMPL_QUERY_INTERFACE(nsCategoryImp, nsIUGenCategory)

NS_IMETHODIMP_(MozExternalRefCountType) nsCategoryImp::AddRef(void)
{
  return MozExternalRefCountType(1);
}

NS_IMETHODIMP_(MozExternalRefCountType) nsCategoryImp::Release(void)
{
  return MozExternalRefCountType(1);
}

nsCategoryImp* nsCategoryImp::GetInstance()
{
  static nsCategoryImp categoryImp;
  return &categoryImp;
}

nsIUGenCategory::nsUGenCategory nsCategoryImp::Get(uint32_t aChar)
{
  return mozilla::unicode::GetGenCategory(aChar);
}
