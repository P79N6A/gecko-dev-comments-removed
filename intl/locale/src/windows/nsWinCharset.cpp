




































#include "nsIPlatformCharset.h"
#include "nsUConvPropertySearch.h"
#include "pratom.h"
#include <windows.h>
#include "nsWin32Locale.h"
#include "nsCOMPtr.h"
#include "nsReadableUtils.h"
#include "nsLocaleCID.h"
#include "nsServiceManagerUtils.h"
#include "nsITimelineService.h"
#include "nsPlatformCharset.h"

static const char* kWinCharsets[][3] = {
#include "wincharset.properties.h"
};

NS_IMPL_ISUPPORTS1(nsPlatformCharset, nsIPlatformCharset)

nsPlatformCharset::nsPlatformCharset()
{
  NS_TIMELINE_START_TIMER("nsPlatformCharset()");

  nsAutoString acpKey(NS_LITERAL_STRING("acp."));
  acpKey.AppendInt(PRInt32(::GetACP() & 0x00FFFF), 10);
  MapToCharset(acpKey, mCharset);

  NS_TIMELINE_STOP_TIMER("nsPlatformCharset()");
  NS_TIMELINE_MARK_TIMER("nsPlatformCharset()");
}

nsPlatformCharset::~nsPlatformCharset()
{
}

nsresult
nsPlatformCharset::MapToCharset(nsAString& inANSICodePage, nsACString& outCharset)
{
  nsCAutoString key;
  LossyCopyUTF16toASCII(inANSICodePage, key);

  nsresult rv = nsUConvPropertySearch::SearchPropertyValue(kWinCharsets,
      NS_ARRAY_LENGTH(kWinCharsets), key, outCharset);
  if (NS_FAILED(rv)) {
    outCharset.AssignLiteral("windows-1252");
  }
  return rv;
}

NS_IMETHODIMP 
nsPlatformCharset::GetCharset(nsPlatformCharsetSel selector,
                              nsACString& oResult)
{
  oResult = mCharset;
  return NS_OK;
}

NS_IMETHODIMP
nsPlatformCharset::GetDefaultCharsetForLocale(const nsAString& localeName, nsACString& oResult)
{
  LCID                      localeAsLCID;

  
  
  
  nsresult rv;
  oResult.Truncate();

  rv = nsWin32Locale::GetPlatformLocale(localeName, &localeAsLCID);
  if (NS_FAILED(rv)) { return rv; }

  PRUnichar acp_name[6];
  if (GetLocaleInfoW(localeAsLCID, LOCALE_IDEFAULTANSICODEPAGE, acp_name,
                     NS_ARRAY_LENGTH(acp_name))==0) {
    return NS_ERROR_FAILURE; 
  }
  nsAutoString acp_key(NS_LITERAL_STRING("acp."));
  acp_key.Append(acp_name);

  return MapToCharset(acp_key, oResult);
}

NS_IMETHODIMP 
nsPlatformCharset::Init()
{
  return NS_OK;
}

nsresult 
nsPlatformCharset::MapToCharset(short script, short region, nsACString& outCharset)
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
