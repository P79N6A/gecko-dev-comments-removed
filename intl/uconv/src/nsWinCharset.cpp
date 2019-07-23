




































#include "nsIPlatformCharset.h"
#include "nsGREResProperties.h"
#include "pratom.h"
#include <windows.h>
#include "nsUConvDll.h"
#include "nsIWin32Locale.h"
#include "nsCOMPtr.h"
#include "nsReadableUtils.h"
#include "nsLocaleCID.h"
#include "nsServiceManagerUtils.h"
#include "nsITimelineService.h"
#include "nsPlatformCharset.h"

static nsGREResProperties *gInfo = nsnull;
static PRInt32 gCnt= 0;

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
  PR_AtomicDecrement(&gCnt);
  if ((0 == gCnt) && (nsnull != gInfo)) {
    delete gInfo;
    gInfo = nsnull;
  }
}

nsresult 
nsPlatformCharset::InitInfo()
{  
  PR_AtomicIncrement(&gCnt); 

  if (gInfo == nsnull) {
    nsGREResProperties *info = new nsGREResProperties(NS_LITERAL_CSTRING("wincharset.properties"));

    NS_ASSERTION(info , "cannot open properties file");
    NS_ENSURE_TRUE(info, NS_ERROR_FAILURE);
    gInfo = info;
  }
  return NS_OK;
}

nsresult
nsPlatformCharset::MapToCharset(nsAString& inANSICodePage, nsACString& outCharset)
{
  
  if (inANSICodePage.EqualsLiteral("acp.1252")) {
    outCharset.AssignLiteral("windows-1252");
    return NS_OK;
  } 

  if (inANSICodePage.EqualsLiteral("acp.932")) {
    outCharset.AssignLiteral("Shift_JIS");
    return NS_OK;
  } 

  
  nsresult rv = InitInfo();
  if (NS_FAILED(rv)) {
    outCharset.AssignLiteral("windows-1252");
    return rv;
  }

  nsAutoString charset;
  rv = gInfo->Get(inANSICodePage, charset);
  if (NS_FAILED(rv)) {
    outCharset.AssignLiteral("windows-1252");
    return rv;
  }

  LossyCopyUTF16toASCII(charset, outCharset);
  return NS_OK;
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
  nsCOMPtr<nsIWin32Locale>	winLocale;
  LCID						localeAsLCID;
  char						acp_name[6];

  
  
  
  nsresult rv;
  oResult.Truncate();

  winLocale = do_GetService(NS_WIN32LOCALE_CONTRACTID, &rv);
  if (NS_FAILED(rv)) { return rv; }

  rv = winLocale->GetPlatformLocale(localeName, &localeAsLCID);
  if (NS_FAILED(rv)) { return rv; }

  if (GetLocaleInfo(localeAsLCID, LOCALE_IDEFAULTANSICODEPAGE, acp_name, sizeof(acp_name))==0) { 
    return NS_ERROR_FAILURE; 
  }
  nsAutoString acp_key(NS_LITERAL_STRING("acp."));
  acp_key.AppendWithConversion(acp_name);

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
