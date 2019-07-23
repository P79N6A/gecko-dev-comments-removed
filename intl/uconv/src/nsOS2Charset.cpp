







































#include "nsIPlatformCharset.h"
#include "nsGREResProperties.h"
#include "pratom.h"
#define INCL_WIN
#include <os2.h>
#include "nsUConvDll.h"
#include "nsIOS2Locale.h"
#include "nsCOMPtr.h"
#include "nsReadableUtils.h"
#include "nsLocaleCID.h"
#include "nsIComponentManager.h"
#include "nsITimelineService.h"
#include "nsPlatformCharset.h"

static nsGREResProperties *gInfo = nsnull;
static PRInt32 gCnt= 0;

NS_IMPL_ISUPPORTS1(nsPlatformCharset, nsIPlatformCharset)

nsPlatformCharset::nsPlatformCharset()
{
  NS_TIMELINE_START_TIMER("nsPlatformCharset()");

  UINT acp = ::WinQueryCp(HMQ_CURRENT);
  PRInt32 acpint = (PRInt32)(acp & 0x00FFFF);
  nsAutoString acpKey(NS_LITERAL_STRING("os2."));
  acpKey.AppendInt(acpint, 10);
  nsresult res = MapToCharset(acpKey, mCharset);

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
    nsGREResProperties *info =
        new nsGREResProperties(NS_LITERAL_CSTRING("os2charset.properties"));

    NS_ASSERTION(info , "cannot open properties file");
    NS_ENSURE_TRUE(info, NS_ERROR_FAILURE);
    gInfo = info;
  }
  return NS_OK;
}

nsresult
nsPlatformCharset::MapToCharset(nsAString& inANSICodePage, nsACString& outCharset)
{
  
  if (inANSICodePage.EqualsLiteral("os2.850")) {
    outCharset.AssignLiteral("IBM850");
    return NS_OK;
  } 

  if (inANSICodePage.EqualsLiteral("os2.932")) {
    outCharset.AssignLiteral("Shift_JIS");
    return NS_OK;
  } 

  
  nsresult rv = InitInfo();
  if (NS_FAILED(rv)) {
    outCharset.AssignLiteral("IBM850");
    return rv;
  }

  nsAutoString charset;
  rv = gInfo->Get(inANSICodePage, charset);
  if (NS_FAILED(rv)) {
    outCharset.AssignLiteral("IBM850");
    return rv;
  }

  LossyCopyUTF16toASCII(charset, outCharset);
  return NS_OK;
}

NS_IMETHODIMP 
nsPlatformCharset::GetCharset(nsPlatformCharsetSel selector,
                              nsACString& oResult)
{
  if ((selector == kPlatformCharsetSel_4xBookmarkFile) || (selector == kPlatformCharsetSel_4xPrefsJS)) {
    if ((mCharset.Find("IBM850", IGNORE_CASE) != -1) || (mCharset.Find("IBM437", IGNORE_CASE) != -1)) 
      oResult.AssignLiteral("ISO-8859-1");
    else if (mCharset.Find("IBM852", IGNORE_CASE) != -1)
      oResult.AssignLiteral("windows-1250");
    else if ((mCharset.Find("IBM855", IGNORE_CASE) != -1) || (mCharset.Find("IBM866", IGNORE_CASE) != -1))
      oResult.AssignLiteral("windows-1251");
    else if ((mCharset.Find("IBM869", IGNORE_CASE) != -1) || (mCharset.Find("IBM813", IGNORE_CASE) != -1))
      oResult.AssignLiteral("windows-1253");
    else if (mCharset.Find("IBM857", IGNORE_CASE) != -1)
      oResult.AssignLiteral("windows-1254");
    else
      oResult = mCharset;
  } else {
    oResult = mCharset;
  }
  return NS_OK;
}

NS_IMETHODIMP
nsPlatformCharset::GetDefaultCharsetForLocale(const nsAString& localeName, nsACString &oResult)
{
  nsCOMPtr<nsIOS2Locale>	os2Locale;
  ULONG						codepage;
  char						acp_name[6];

  
  
  
  nsresult rv;
  oResult.Truncate();

  os2Locale = do_CreateInstance(NS_OS2LOCALE_CONTRACTID, &rv);
  if (NS_FAILED(rv)) { return rv; }

  rv = os2Locale->GetPlatformLocale(localeName, &codepage);
  if (NS_FAILED(rv)) { return rv; }

  nsAutoString os2_key(NS_LITERAL_STRING("os2."));
  os2_key.AppendInt((PRUint32)codepage);

  return MapToCharset(os2_key, oResult);

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
