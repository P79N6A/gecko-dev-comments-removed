




#include <locale.h>

#include "mozilla/ArrayUtils.h"

#include "nsIPlatformCharset.h"
#include "nsUConvPropertySearch.h"
#include "nsCOMPtr.h"
#include "nsReadableUtils.h"
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
#include "mozilla/dom/EncodingUtils.h"

using mozilla::dom::EncodingUtils;
using namespace mozilla;

static const char* kUnixCharsets[][3] = {
#include "unixcharset.properties.h"
};

NS_IMPL_ISUPPORTS(nsPlatformCharset, nsIPlatformCharset)

nsPlatformCharset::nsPlatformCharset()
{
}

static nsresult
ConvertLocaleToCharsetUsingDeprecatedConfig(const nsACString& locale,
                                            nsACString& oResult)
{
  if (!(locale.IsEmpty())) {
    nsAutoCString localeKey;
    localeKey.AssignLiteral("locale.all.");
    localeKey.Append(locale);
    if (NS_SUCCEEDED(nsUConvPropertySearch::SearchPropertyValue(kUnixCharsets,
        ArrayLength(kUnixCharsets), localeKey, oResult))) {
      return NS_OK;
    }
  }
  NS_ERROR("unable to convert locale to charset using deprecated config");
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
  
  
  
  
  NS_LossyConvertUTF16toASCII localeStr(localeName);
  return ConvertLocaleToCharsetUsingDeprecatedConfig(localeStr, oResult);
#endif
}

nsresult
nsPlatformCharset::InitGetCharset(nsACString &oString)
{
  char* nl_langinfo_codeset = nullptr;
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

  
  
  
  char* locale = setlocale(LC_CTYPE, nullptr);
  nsAutoCString localeStr;
  localeStr.Assign(locale);
  return ConvertLocaleToCharsetUsingDeprecatedConfig(localeStr, oString);
}

NS_IMETHODIMP 
nsPlatformCharset::Init()
{
  
  
  
  
  char* locale = setlocale(LC_CTYPE, nullptr);
  NS_ASSERTION(locale, "cannot setlocale");
  if (locale) {
    CopyASCIItoUTF16(locale, mLocale); 
  } else {
    mLocale.AssignLiteral("en_US");
  }

  
  return InitGetCharset(mCharset);
}

nsresult
nsPlatformCharset::VerifyCharset(nsCString &aCharset)
{
  
  if (aCharset.EqualsLiteral("UTF-8")) {
    return NS_OK;
  }

  nsAutoCString encoding;
  if (!EncodingUtils::FindEncodingForLabelNoReplacement(aCharset, encoding)) {
    return NS_ERROR_UCONV_NOCONV;
  }

  aCharset.Assign(encoding);
  return NS_OK;
}
