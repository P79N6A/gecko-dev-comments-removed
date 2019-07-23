




































#include <Script.h>
#include <TextCommon.h>
#include "nsIPlatformCharset.h"
#include "pratom.h"
#include "nsGREResProperties.h"
#include "nsUConvDll.h"
#include "nsCOMPtr.h"
#include "nsIComponentManager.h"
#include "nsIMacLocale.h"
#include "nsLocaleCID.h"
#include "nsReadableUtils.h"
#include "nsPlatformCharset.h"
#include "nsEncoderDecoderUtils.h"

static nsGREResProperties *gInfo = nsnull;
static PRInt32 gCnt = 0;

NS_IMPL_ISUPPORTS1(nsPlatformCharset, nsIPlatformCharset)

nsPlatformCharset::nsPlatformCharset()
{
  PR_AtomicIncrement(&gCnt);
}
nsPlatformCharset::~nsPlatformCharset()
{
  PR_AtomicDecrement(&gCnt);
  if((0 == gCnt) && (nsnull != gInfo)) {
  	delete gInfo;
  	gInfo = nsnull;
  }
}

nsresult nsPlatformCharset::InitInfo()
{  
  
  if (gInfo == nsnull) {
    nsGREResProperties *info =
        new nsGREResProperties(NS_LITERAL_CSTRING("maccharset.properties"));
    NS_ASSERTION(info , "cannot open properties file");
    NS_ENSURE_TRUE(info, NS_ERROR_FAILURE);
    gInfo = info;
  }

  return NS_OK;
}

nsresult nsPlatformCharset::MapToCharset(short script, short region, nsACString& outCharset)
{
  switch (region) {
    case verUS:
    case verFrance:
    case verGermany:
      outCharset.AssignLiteral("x-mac-roman");
      return NS_OK;
    case verJapan:
      outCharset.AssignLiteral("Shift_JIS");
      return NS_OK;
  }

  
  nsresult rv = InitInfo();
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsAutoString key(NS_LITERAL_STRING("region."));
  key.AppendInt(region, 10);

  nsAutoString uCharset;
  rv = gInfo->Get(key, uCharset);
  if (NS_SUCCEEDED(rv))
    LossyCopyUTF16toASCII(uCharset, outCharset);
  else {
    key.AssignLiteral("script.");
    key.AppendInt(script, 10);
    rv = gInfo->Get(key, uCharset);
    
    if (NS_SUCCEEDED(rv))
      LossyCopyUTF16toASCII(uCharset, outCharset);
    else {
      outCharset.AssignLiteral("x-mac-roman");
    }
  }
  
  return NS_OK;
}

NS_IMETHODIMP 
nsPlatformCharset::GetCharset(nsPlatformCharsetSel selector, nsACString& oResult)
{
  nsresult rv;
  if (mCharset.IsEmpty()) {
    rv = MapToCharset(
           (short)(0x0000FFFF & ::GetScriptManagerVariable(smSysScript)),
           (short)(0x0000FFFF & ::GetScriptManagerVariable(smRegionCode)),
           mCharset);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  switch (selector) {
#ifdef XP_MACOSX  
    case kPlatformCharsetSel_FileName:
      oResult.AssignLiteral("UTF-8");
      break;
#endif
    case  kPlatformCharsetSel_KeyboardInput:
      rv = MapToCharset(
             (short) (0x0000FFFF & ::GetScriptManagerVariable(smKeyScript)),
             kTextRegionDontCare, oResult);
      NS_ENSURE_SUCCESS(rv, rv);
      break;
    default:
      oResult = mCharset;
      break;
  }

   return NS_OK;
}

NS_IMETHODIMP 
nsPlatformCharset::GetDefaultCharsetForLocale(const nsAString& localeName, nsACString &oResult)
{
  nsresult rv;
  nsCOMPtr<nsIMacLocale> pMacLocale;
  pMacLocale = do_CreateInstance(NS_MACLOCALE_CONTRACTID, &rv);
  if (NS_SUCCEEDED(rv)) {
    short script, language, region;
    rv = pMacLocale->GetPlatformLocale(localeName, &script, &language, &region);
    if (NS_SUCCEEDED(rv)) {
      if (NS_SUCCEEDED(MapToCharset(script, region, oResult))) {
        return NS_OK;
      }
    }
  }

  
  oResult.AssignLiteral("x-mac-roman");
  return NS_SUCCESS_USING_FALLBACK_LOCALE;
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
