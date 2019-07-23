



































#include "nsIPlatformCharset.h"
#include "nsReadableUtils.h"
#include "nsPlatformCharset.h"







NS_IMPL_THREADSAFE_ISUPPORTS1(nsPlatformCharset, nsIPlatformCharset)

nsPlatformCharset::nsPlatformCharset()
{
  mCharset.AssignLiteral("UTF-8");
}

nsPlatformCharset::~nsPlatformCharset()
{
}

NS_IMETHODIMP
nsPlatformCharset::GetCharset(nsPlatformCharsetSel selector, nsACString& aResult)
{
  aResult = mCharset;
  return NS_OK;
}

NS_IMETHODIMP
nsPlatformCharset::GetDefaultCharsetForLocale(const nsAString& localeName, nsACString& aResult)
{
  aResult = mCharset;
  return NS_OK;
}

NS_IMETHODIMP
nsPlatformCharset::Init()
{
  return NS_OK;
}
nsresult
nsPlatformCharset::MapToCharset(short script, short region, nsACString& aCharset)
{
  return NS_OK;
}

nsresult
nsPlatformCharset::MapToCharset(nsAString& inANSICodePage, nsACString& aCharset)
{
  return NS_OK;
}

nsresult
nsPlatformCharset::InitGetCharset(nsACString &aString)
{
  aString = mCharset;
  return NS_OK;
}

nsresult
nsPlatformCharset::ConvertLocaleToCharsetUsingDeprecatedConfig(nsAString& locale, nsACString& aResult)
{
  aResult = mCharset;
  return NS_OK;
}

nsresult
nsPlatformCharset::VerifyCharset(nsCString &aCharset)
{
  aCharset = mCharset;
  return NS_OK;
}

nsresult
nsPlatformCharset::InitInfo()
{
  return NS_OK;
}
