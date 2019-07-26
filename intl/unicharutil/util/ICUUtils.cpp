



#ifdef MOZILLA_INTERNAL_API
#ifdef ENABLE_INTL_API

#include "ICUUtils.h"
#include "mozilla/Preferences.h"
#include "nsIContent.h"
#include "nsIDocument.h"
#include "nsIToolkitChromeRegistry.h"
#include "nsStringGlue.h"
#include "unicode/uloc.h"
#include "unicode/unum.h"

using namespace mozilla;






static bool gLocaleNumberGroupingEnabled;
static const char LOCALE_NUMBER_GROUPING_PREF_STR[] = "dom.forms.number.grouping";

static bool
LocaleNumberGroupingIsEnabled()
{
  static bool sInitialized = false;

  if (!sInitialized) {
    
    Preferences::AddBoolVarCache(&gLocaleNumberGroupingEnabled,
                                 LOCALE_NUMBER_GROUPING_PREF_STR,
                                 false);
    sInitialized = true;
  }

  return gLocaleNumberGroupingEnabled;
}

void
ICUUtils::LanguageTagIterForContent::GetNext(nsACString& aBCP47LangTag)
{
  if (mCurrentFallbackIndex < 0) {
    mCurrentFallbackIndex = 0;
    
    
    nsAutoString lang;
    mContent->GetLang(lang);
    if (!lang.IsEmpty()) {
      aBCP47LangTag = NS_ConvertUTF16toUTF8(lang);
      return;
    }
  }

  if (mCurrentFallbackIndex < 1) {
    mCurrentFallbackIndex = 1;
    
    
    nsIDocument* doc = mContent->OwnerDoc();
    nsAutoString lang;
    doc->GetContentLanguage(lang);
    if (!lang.IsEmpty()) {
      aBCP47LangTag = NS_ConvertUTF16toUTF8(lang);
      return;
    }
  }

  if (mCurrentFallbackIndex < 2) {
    mCurrentFallbackIndex = 2;
    
    nsCOMPtr<nsIToolkitChromeRegistry> cr =
      mozilla::services::GetToolkitChromeRegistryService();
    nsAutoCString uaLangTag;
    if (cr) {
      cr->GetSelectedLocale(NS_LITERAL_CSTRING("global"), uaLangTag);
    }
    if (!uaLangTag.IsEmpty()) {
      aBCP47LangTag = uaLangTag;
      return;
    }
  }

  
  

  aBCP47LangTag.Truncate(); 
}

 bool
ICUUtils::LocalizeNumber(double aValue,
                         LanguageTagIterForContent& aLangTags,
                         nsAString& aLocalizedValue)
{
  MOZ_ASSERT(aLangTags.IsAtStart(), "Don't call Next() before passing");

  static const int32_t kBufferSize = 256;

  UChar buffer[kBufferSize];

  nsAutoCString langTag;
  aLangTags.GetNext(langTag);
  while (!langTag.IsEmpty()) {
    UErrorCode status = U_ZERO_ERROR;
    AutoCloseUNumberFormat format(unum_open(UNUM_DECIMAL, nullptr, 0,
                                            langTag.get(), nullptr, &status));
    unum_setAttribute(format, UNUM_GROUPING_USED,
                      LocaleNumberGroupingIsEnabled());
    
    
    
    unum_setAttribute(format, UNUM_MAX_FRACTION_DIGITS, 16);
    int32_t length = unum_formatDouble(format, aValue, buffer, kBufferSize,
                                       nullptr, &status);
    NS_ASSERTION(length < kBufferSize &&
                 status != U_BUFFER_OVERFLOW_ERROR &&
                 status != U_STRING_NOT_TERMINATED_WARNING,
                 "Need a bigger buffer?!");
    if (U_SUCCESS(status)) {
      ICUUtils::AssignUCharArrayToString(buffer, length, aLocalizedValue);
      return true;
    }
    aLangTags.GetNext(langTag);
  }
  return false;
}

 double
ICUUtils::ParseNumber(nsAString& aValue,
                      LanguageTagIterForContent& aLangTags)
{
  MOZ_ASSERT(aLangTags.IsAtStart(), "Don't call Next() before passing");

  if (aValue.IsEmpty()) {
    return std::numeric_limits<float>::quiet_NaN();
  }

  uint32_t length = aValue.Length();

  nsAutoCString langTag;
  aLangTags.GetNext(langTag);
  while (!langTag.IsEmpty()) {
    UErrorCode status = U_ZERO_ERROR;
    AutoCloseUNumberFormat format(unum_open(UNUM_DECIMAL, nullptr, 0,
                                            langTag.get(), nullptr, &status));
    int32_t parsePos = 0;
    static_assert(sizeof(UChar) == 2 && sizeof(nsAString::char_type) == 2,
                  "Unexpected character size - the following cast is unsafe");
    double val = unum_parseDouble(format,
                                  (const UChar*)PromiseFlatString(aValue).get(),
                                  length, &parsePos, &status);
    if (U_SUCCESS(status) && parsePos == (int32_t)length) {
      return val;
    }
    aLangTags.GetNext(langTag);
  }
  return std::numeric_limits<float>::quiet_NaN();
}

 void
ICUUtils::AssignUCharArrayToString(UChar* aICUString,
                                   int32_t aLength,
                                   nsAString& aMozString)
{
  
  

  static_assert(sizeof(UChar) == 2 && sizeof(nsAString::char_type) == 2,
                "Unexpected character size - the following cast is unsafe");

  aMozString.Assign((const nsAString::char_type*)aICUString, aLength);

  NS_ASSERTION((int32_t)aMozString.Length() == aLength, "Conversion failed");
}

#if 0
 Locale
ICUUtils::BCP47CodeToLocale(const nsAString& aBCP47Code)
{
  MOZ_ASSERT(!aBCP47Code.IsEmpty(), "Don't pass an empty BCP 47 code");

  Locale locale;
  locale.setToBogus();

  
  NS_LossyConvertUTF16toASCII bcp47code(aBCP47Code);

  UErrorCode status = U_ZERO_ERROR;
  int32_t needed;

  char localeID[256];
  needed = uloc_forLanguageTag(bcp47code.get(), localeID,
                               PR_ARRAY_SIZE(localeID) - 1, nullptr,
                               &status);
  MOZ_ASSERT(needed < int32_t(PR_ARRAY_SIZE(localeID)) - 1,
             "Need a bigger buffer");
  if (needed <= 0 || U_FAILURE(status)) {
    return locale;
  }

  char lang[64];
  needed = uloc_getLanguage(localeID, lang, PR_ARRAY_SIZE(lang) - 1,
                            &status);
  MOZ_ASSERT(needed < int32_t(PR_ARRAY_SIZE(lang)) - 1,
             "Need a bigger buffer");
  if (needed <= 0 || U_FAILURE(status)) {
    return locale;
  }

  char country[64];
  needed = uloc_getCountry(localeID, country, PR_ARRAY_SIZE(country) - 1,
                           &status);
  MOZ_ASSERT(needed < int32_t(PR_ARRAY_SIZE(country)) - 1,
             "Need a bigger buffer");
  if (needed > 0 && U_SUCCESS(status)) {
    locale = Locale(lang, country);
  }

  if (locale.isBogus()) {
    
    locale = Locale(lang);
  }

  return locale;
}

 void
ICUUtils::ToMozString(UnicodeString& aICUString, nsAString& aMozString)
{
  
  

  static_assert(sizeof(UChar) == 2 && sizeof(nsAString::char_type) == 2,
                "Unexpected character size - the following cast is unsafe");

  const nsAString::char_type* buf =
    (const nsAString::char_type*)aICUString.getTerminatedBuffer();
  aMozString.Assign(buf);

  NS_ASSERTION(aMozString.Length() == (uint32_t)aICUString.length(),
               "Conversion failed");
}

 void
ICUUtils::ToICUString(nsAString& aMozString, UnicodeString& aICUString)
{
  
  

  static_assert(sizeof(UChar) == 2 && sizeof(nsAString::char_type) == 2,
                "Unexpected character size - the following cast is unsafe");

  aICUString.setTo((UChar*)PromiseFlatString(aMozString).get(),
                   aMozString.Length());

  NS_ASSERTION(aMozString.Length() == (uint32_t)aICUString.length(),
               "Conversion failed");
}
#endif

#endif 
#endif 

