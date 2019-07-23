




































#include <locale.h>
#include "nsIPlatformCharset.h"
#include "pratom.h"
#include "nsGREResProperties.h"
#include "nsCOMPtr.h"
#include "nsReadableUtils.h"
#include "nsLocaleCID.h"
#include "nsUConvDll.h"
#include "nsIComponentManager.h"
#include "nsIServiceManager.h"
#include "nsIUnicodeDecoder.h"
#include "nsIUnicodeEncoder.h"
#include "nsICharsetConverterManager.h"
#include "nsEncoderDecoderUtils.h"
#if HAVE_GNU_LIBC_VERSION_H
#include <gnu/libc-version.h>
#endif
#ifdef HAVE_NL_TYPES_H
#include <nl_types.h>
#endif
#if HAVE_LANGINFO_CODESET
#include <langinfo.h>
#endif
#include "nsPlatformCharset.h"
#include "nsAutoLock.h"
#include "prinit.h"
#include "nsUnicharUtils.h"

NS_IMPL_THREADSAFE_ISUPPORTS1(nsPlatformCharset, nsIPlatformCharset)

static nsGREResProperties *gNLInfo = nsnull;
static nsGREResProperties *gInfo_deprecated = nsnull;
static PRInt32 gCnt=0;


static PRLock  *gLock = nsnull;

static PRStatus InitLock(void)
{
  gLock = PR_NewLock();
  if (gLock)
    return PR_SUCCESS;
  return PR_FAILURE;
}

nsPlatformCharset::nsPlatformCharset()
{
  PR_AtomicIncrement(&gCnt);
  static PRCallOnceType once;
  PR_CallOnce(&once, InitLock);
  NS_ASSERTION(gLock, "Can't allocate a lock?!");
}

nsresult
nsPlatformCharset::ConvertLocaleToCharsetUsingDeprecatedConfig(nsAString& locale, nsACString& oResult)
{

  
  {
    nsAutoLock guard(gLock);
    if (!gInfo_deprecated) {
      nsGREResProperties *info =
          new nsGREResProperties(NS_LITERAL_CSTRING("unixcharset.properties"));
      NS_ASSERTION(info, "cannot create nsGREResProperties");
      gInfo_deprecated = info;
    }
  }

  if (gInfo_deprecated && !(locale.IsEmpty())) {
    nsAutoString platformLocaleKey;
    
    platformLocaleKey.AssignLiteral("locale.");
    platformLocaleKey.AppendWithConversion(OSTYPE);
    platformLocaleKey.AppendLiteral(".");
    platformLocaleKey.Append(locale);

    nsAutoString charset;
    nsresult res = gInfo_deprecated->Get(platformLocaleKey, charset);
    if (NS_SUCCEEDED(res))  {
      LossyCopyUTF16toASCII(charset, oResult);
      return NS_OK;
    }
    nsAutoString localeKey;
    localeKey.AssignLiteral("locale.all.");
    localeKey.Append(locale);
    res = gInfo_deprecated->Get(localeKey, charset);
    if (NS_SUCCEEDED(res))  {
      LossyCopyUTF16toASCII(charset, oResult);
      return NS_OK;
    }
   }
   NS_ERROR("unable to convert locale to charset using deprecated config");
   mCharset.AssignLiteral("ISO-8859-1");
   oResult.AssignLiteral("ISO-8859-1");
   return NS_SUCCESS_USING_FALLBACK_LOCALE;
}

nsPlatformCharset::~nsPlatformCharset()
{
  PR_AtomicDecrement(&gCnt);
  if (!gCnt) {
    if (gNLInfo) {
      delete gNLInfo;
      gNLInfo = nsnull;
      PR_DestroyLock(gLock);
      gLock = nsnull;
    }
    if (gInfo_deprecated) {
      delete gInfo_deprecated;
      gInfo_deprecated = nsnull;
    }
  }
}

NS_IMETHODIMP 
nsPlatformCharset::GetCharset(nsPlatformCharsetSel selector, nsACString& oResult)
{
  oResult = mCharset; 
  return NS_OK;
}

NS_IMETHODIMP 
nsPlatformCharset::GetDefaultCharsetForLocale(const nsAString& localeName, nsACString &oResult)
{
  
  
  
  
  if (mLocale.Equals(localeName) ||
    
    (mLocale.LowerCaseEqualsLiteral("en_us") && 
     localeName.LowerCaseEqualsLiteral("c"))) {
    oResult = mCharset;
    return NS_OK;
  }

#if HAVE_LANGINFO_CODESET
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  NS_WARNING("GetDefaultCharsetForLocale: need to add multi locale support");
#ifdef DEBUG_jungshik
  printf("localeName=%s mCharset=%s\n", NS_ConvertUTF16toUTF8(localeName).get(),
         mCharset.get());
#endif
  
  oResult = mCharset;
  return NS_SUCCESS_USING_FALLBACK_LOCALE;
#endif

  
  
  
  
  nsAutoString localeStr(localeName);
  nsresult res = ConvertLocaleToCharsetUsingDeprecatedConfig(localeStr, oResult);
  if (NS_SUCCEEDED(res))
    return res;

  NS_ERROR("unable to convert locale to charset using deprecated config");
  oResult.AssignLiteral("ISO-8859-1");
  return NS_SUCCESS_USING_FALLBACK_LOCALE;
}

nsresult
nsPlatformCharset::InitGetCharset(nsACString &oString)
{
  char* nl_langinfo_codeset = nsnull;
  nsCString aCharset;
  nsresult res;

#if HAVE_LANGINFO_CODESET
  nl_langinfo_codeset = nl_langinfo(CODESET);
  NS_ASSERTION(nl_langinfo_codeset, "cannot get nl_langinfo(CODESET)");

  
  
  
  if (nl_langinfo_codeset) {
    aCharset.Assign(nl_langinfo_codeset);
    res = VerifyCharset(aCharset);
    if (NS_SUCCEEDED(res)) {
      oString = aCharset;
      return res;
    }
  }

  
  {
    nsAutoLock guard(gLock);

    if (!gNLInfo) {
      nsCAutoString propertyFile;
      
      propertyFile.AssignLiteral("unixcharset.");
      propertyFile.AppendLiteral(NS_STRINGIFY(OSARCH));
      propertyFile.AppendLiteral(".properties");
      nsGREResProperties *info = new nsGREResProperties(propertyFile);
      NS_ASSERTION(info, "cannot create nsGREResProperties");
      if (info) {
        PRBool didLoad = info->DidLoad();
        if (!didLoad) {
          delete info;
          info = nsnull;
        }
      }
      gNLInfo = info;
    }
  }

  
  
  
  if (gNLInfo && nl_langinfo_codeset) {
    nsAutoString localeKey;

#if HAVE_GNU_GET_LIBC_VERSION
    
    
    
    const char *glibc_version = gnu_get_libc_version();
    if ((glibc_version != nsnull) && (strlen(glibc_version))) {
      localeKey.AssignLiteral("nllic.");
      localeKey.AppendWithConversion(glibc_version);
      localeKey.AppendLiteral(".");
      localeKey.AppendWithConversion(nl_langinfo_codeset);
      nsAutoString uCharset;
      res = gNLInfo->Get(localeKey, uCharset);
      if (NS_SUCCEEDED(res)) {
        aCharset.AssignWithConversion(uCharset);
        res = VerifyCharset(aCharset);
        if (NS_SUCCEEDED(res)) {
          oString = aCharset;
          return res;
        }
      }
    }
#endif

    
    
    
    localeKey.AssignLiteral("nllic.");
    localeKey.AppendWithConversion(nl_langinfo_codeset);
    nsAutoString uCharset;
    res = gNLInfo->Get(localeKey, uCharset);
    if (NS_SUCCEEDED(res)) {
      aCharset.AssignWithConversion(uCharset);
      res = VerifyCharset(aCharset);
      if (NS_SUCCEEDED(res)) {
        oString = aCharset;
        return res;
      }
    }
  }

  NS_ERROR("unable to use nl_langinfo(CODESET)");
#endif

  
  
  
  char* locale = setlocale(LC_CTYPE, nsnull);
  nsAutoString localeStr;
  localeStr.AssignWithConversion(locale);
  res = ConvertLocaleToCharsetUsingDeprecatedConfig(localeStr, oString);
  if (NS_SUCCEEDED(res)) {
    return res; 
  }

  oString.Truncate();
  return res;
}

NS_IMETHODIMP 
nsPlatformCharset::Init()
{
  nsCAutoString charset;
  nsresult res = NS_OK;

  
  
  
  
  char* locale = setlocale(LC_CTYPE, nsnull);
  NS_ASSERTION(locale, "cannot setlocale");
  if (locale) {
    CopyASCIItoUTF16(locale, mLocale); 
  } else {
    mLocale.AssignLiteral("en_US");
  }

  res = InitGetCharset(charset);
  if (NS_SUCCEEDED(res)) {
    mCharset = charset;
    return res; 
  }

  
  NS_ERROR("unable to convert locale to charset using deprecated config");
  mCharset.AssignLiteral("ISO-8859-1");
  return NS_SUCCESS_USING_FALLBACK_LOCALE;
}

nsresult
nsPlatformCharset::VerifyCharset(nsCString &aCharset)
{
  nsresult res;
  
  
  
  nsCOMPtr <nsICharsetConverterManager>  charsetConverterManager;
  charsetConverterManager = do_GetService(NS_CHARSETCONVERTERMANAGER_CONTRACTID, &res);
  if (NS_FAILED(res))
    return res;

  
  
  
  nsCOMPtr <nsIUnicodeEncoder> enc;
  res = charsetConverterManager->GetUnicodeEncoder(aCharset.get(), getter_AddRefs(enc));
  if (NS_FAILED(res)) {
    NS_ERROR("failed to create encoder");
    return res;
  }

  
  
  
  nsCOMPtr <nsIUnicodeDecoder> dec;
  res = charsetConverterManager->GetUnicodeDecoder(aCharset.get(), getter_AddRefs(dec));
  if (NS_FAILED(res)) {
    NS_ERROR("failed to create decoder");
    return res;
  }

  
  
  

  nsCAutoString result;
  res = charsetConverterManager->GetCharsetAlias(aCharset.get(), result);
  if (NS_FAILED(res)) {
    return res;
  }

  
  
  

  aCharset.Assign(result);
  NS_ASSERTION(NS_SUCCEEDED(res), "failed to get preferred charset name, using non-preferred");
  return NS_OK;
}

nsresult 
nsPlatformCharset::InitInfo()
{  
  return NS_OK;
}
