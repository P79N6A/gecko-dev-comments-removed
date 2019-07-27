











#include <stdio.h>

#include "nsSample.h"
#include "nsMemory.h"

#include "nsEmbedString.h"
#include "nsIClassInfoImpl.h"


nsSampleImpl::nsSampleImpl() : mValue(nullptr)
{
  mValue = (char*)nsMemory::Clone("initial value", 14);
}

nsSampleImpl::~nsSampleImpl()
{
  if (mValue) {
    nsMemory::Free(mValue);
  }
}















NS_IMPL_CLASSINFO(nsSampleImpl, nullptr, 0, NS_SAMPLE_CID)
NS_IMPL_ISUPPORTS_CI(nsSampleImpl, nsISample)





NS_IMETHODIMP
nsSampleImpl::GetValue(char** aValue)
{
  NS_PRECONDITION(aValue != nullptr, "null ptr");
  if (!aValue) {
    return NS_ERROR_NULL_POINTER;
  }

  if (mValue) {
    












    *aValue = (char*)nsMemory::Clone(mValue, strlen(mValue) + 1);
    if (!*aValue) {
      return NS_ERROR_NULL_POINTER;
    }
  } else {
    *aValue = nullptr;
  }
  return NS_OK;
}

NS_IMETHODIMP
nsSampleImpl::SetValue(const char* aValue)
{
  NS_PRECONDITION(aValue != nullptr, "null ptr");
  if (!aValue) {
    return NS_ERROR_NULL_POINTER;
  }

  if (mValue) {
    nsMemory::Free(mValue);
  }

  





  mValue = (char*)nsMemory::Clone(aValue, strlen(aValue) + 1);
  return NS_OK;
}

NS_IMETHODIMP
nsSampleImpl::Poke(const char* aValue)
{
  return SetValue((char*)aValue);
}


static void
GetStringValue(nsACString& aValue)
{
  NS_CStringSetData(aValue, "GetValue");
}

NS_IMETHODIMP
nsSampleImpl::WriteValue(const char* aPrefix)
{
  NS_PRECONDITION(aPrefix != nullptr, "null ptr");
  if (!aPrefix) {
    return NS_ERROR_NULL_POINTER;
  }

  printf("%s %s\n", aPrefix, mValue);

  
  nsEmbedString foopy;
  foopy.Append(char16_t('f'));
  foopy.Append(char16_t('o'));
  foopy.Append(char16_t('o'));
  foopy.Append(char16_t('p'));
  foopy.Append(char16_t('y'));

  const char16_t* f = foopy.get();
  uint32_t l = foopy.Length();
  printf("%c%c%c%c%c %d\n",
         char(f[0]), char(f[1]), char(f[2]), char(f[3]), char(f[4]), l);

  nsEmbedCString foopy2;
  GetStringValue(foopy2);

  
  const char* f2 = foopy2.get();
  uint32_t l2 = foopy2.Length();

  printf("%s %d\n", f2, l2);

  return NS_OK;
}
