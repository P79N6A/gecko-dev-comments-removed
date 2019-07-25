




#include "nscore.h"
#include "pratom.h"
#include "nsUUDll.h"
#include "nsISupports.h"
#include "nsCategoryImp.h"
#include "nsUnicodeProperties.h"

static nsCategoryImp gCategoryImp;

NS_IMPL_THREADSAFE_QUERY_INTERFACE1(nsCategoryImp, nsIUGenCategory)

NS_IMETHODIMP_(nsrefcnt) nsCategoryImp::AddRef(void)
{
  return nsrefcnt(1);
}

NS_IMETHODIMP_(nsrefcnt) nsCategoryImp::Release(void)
{
  return nsrefcnt(1);
}

nsCategoryImp* nsCategoryImp::GetInstance()
{
  return &gCategoryImp;
}

nsIUGenCategory::nsUGenCategory nsCategoryImp::Get(uint32_t aChar)
{
  return mozilla::unicode::GetGenCategory(aChar);
}
