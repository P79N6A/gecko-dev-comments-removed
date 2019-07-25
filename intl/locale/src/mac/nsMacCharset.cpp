




































#include <Carbon/Carbon.h>
#include "nsIPlatformCharset.h"
#include "pratom.h"
#include "nsCOMPtr.h"
#include "nsIServiceManager.h"
#include "nsLocaleCID.h"
#include "nsReadableUtils.h"
#include "nsPlatformCharset.h"
#include "nsEncoderDecoderUtils.h"

NS_IMPL_ISUPPORTS1(nsPlatformCharset, nsIPlatformCharset)

nsPlatformCharset::nsPlatformCharset()
{
}
nsPlatformCharset::~nsPlatformCharset()
{
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

NS_IMETHODIMP 
nsPlatformCharset::Init()
{
  return NS_OK;
}

nsresult 
nsPlatformCharset::MapToCharset(nsAString& inANSICodePage, nsACString& outCharset)
{
  return NS_OK;
}

nsresult
nsPlatformCharset::InitGetCharset(nsACString &oString)
{
  return NS_OK;
}

nsresult
nsPlatformCharset::ConvertLocaleToCharsetUsingDeprecatedConfig(nsAString& locale, nsACString& oResult)
{
  return NS_OK;
}

nsresult
nsPlatformCharset::VerifyCharset(nsCString &aCharset)
{
  return NS_OK;
}
