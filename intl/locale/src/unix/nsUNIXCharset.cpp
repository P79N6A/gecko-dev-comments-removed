




































#include <locale.h>
#include "nsIPlatformCharset.h"
#include "pratom.h"
#include "nsUConvPropertySearch.h"
#include "nsCOMPtr.h"
#include "nsReadableUtils.h"
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
#include "prinit.h"
#include "nsUnicharUtils.h"

static const char* kUnixCharsets[][3] = {
#include "unixcharset.properties.h"
};

NS_IMPL_THREADSAFE_ISUPPORTS1(nsPlatformCharset, nsIPlatformCharset)

nsPlatformCharset::nsPlatformCharset()
{
}

nsresult
nsPlatformCharset::ConvertLocaleToCharsetUsingDeprecatedConfig(nsAString& locale, nsACString& oResult)
{
  if (!(locale.IsEmpty())) {
    nsCAutoString platformLocaleKey;
    
    platformLocaleKey.AssignLiteral("locale.");
    platformLocaleKey.Append(OSTYPE);
    platformLocaleKey.AppendLiteral(".");
    platformLocaleKey.AppendWithConversion(locale);

    nsresult res = nsUConvPropertySearch::SearchPropertyValue(kUnixCharsets,
        NS_ARRAY_LENGTH(kUnixCharsets), platformLocaleKey, oResult);
    if (NS_SUCCEEDED(res))  {
      return NS_OK;
    }
    nsCAutoString localeKey;
    localeKey.AssignLiteral("locale.all.");
    localeKey.AppendWithConversion(locale);
    res = nsUConvPropertySearch::SearchPropertyValue(kUnixCharsets,
        NS_ARRAY_LENGTH(kUnixCharsets), localeKey, oResult);
    if (NS_SUCCEEDED(res))  {
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
#else
  
  
  
  
  nsAutoString localeStr(localeName);
  nsresult res = ConvertLocaleToCharsetUsingDeprecatedConfig(localeStr, oResult);
  if (NS_SUCCEEDED(res))
    return res;

  NS_ERROR("unable to convert locale to charset using deprecated config");
  oResult.AssignLiteral("ISO-8859-1");
  return NS_SUCCESS_USING_FALLBACK_LOCALE;
#endif
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
