




#include "nsIPlatformCharset.h"
#include "nsPlatformCharset.h"

NS_IMPL_ISUPPORTS(nsPlatformCharset, nsIPlatformCharset)

nsPlatformCharset::nsPlatformCharset()
{
}

nsPlatformCharset::~nsPlatformCharset()
{
}

NS_IMETHODIMP 
nsPlatformCharset::Init()
{
  return NS_OK;
}

NS_IMETHODIMP 
nsPlatformCharset::GetCharset(nsPlatformCharsetSel selector, nsACString& oResult)
{
  oResult.AssignLiteral("UTF-8");
  return NS_OK;
}

NS_IMETHODIMP 
nsPlatformCharset::GetDefaultCharsetForLocale(const nsAString& localeName, nsACString &oResult)
{
  oResult.AssignLiteral("UTF-8");
  return NS_OK;
}

nsresult
nsPlatformCharset::InitGetCharset(nsACString &oString)
{
  return NS_OK;
}

nsresult
nsPlatformCharset::VerifyCharset(nsCString &aCharset)
{
  return NS_OK;
}
