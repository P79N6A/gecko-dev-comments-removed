




#include "nsLanguageAtomService.h"
#include "nsILocaleService.h"
#include "nsUConvPropertySearch.h"
#include "nsUnicharUtils.h"
#include "nsIAtom.h"
#include "mozilla/ArrayUtils.h"
#include "mozilla/Services.h"
#include "nsServiceManagerUtils.h"
#include "mozilla/dom/EncodingUtils.h"

using namespace mozilla;

static const char* kLangGroups[][3] = {
#include "langGroups.properties.h"
};

NS_IMPL_ISUPPORTS(nsLanguageAtomService, nsILanguageAtomService)

nsLanguageAtomService::nsLanguageAtomService()
{
}

nsIAtom*
nsLanguageAtomService::LookupLanguage(const nsACString &aLanguage,
                                      nsresult *aError)
{
  nsAutoCString lowered(aLanguage);
  ToLowerCase(lowered);

  nsCOMPtr<nsIAtom> lang = do_GetAtom(lowered);
  return GetLanguageGroup(lang, aError);
}

already_AddRefed<nsIAtom>
nsLanguageAtomService::LookupCharSet(const nsACString& aCharSet)
{
  nsAutoCString group;
  mozilla::dom::EncodingUtils::LangGroupForEncoding(aCharSet, group);
  return do_GetAtom(group);
}

nsIAtom*
nsLanguageAtomService::GetLocaleLanguage(nsresult *aError)
{
  nsresult res = NS_OK;

  do {
    if (!mLocaleLanguage) {
      nsCOMPtr<nsILocaleService> localeService;
      localeService = do_GetService(NS_LOCALESERVICE_CONTRACTID);
      if (!localeService) {
        res = NS_ERROR_FAILURE;
        break;
      }

      nsCOMPtr<nsILocale> locale;
      res = localeService->GetApplicationLocale(getter_AddRefs(locale));
      if (NS_FAILED(res))
        break;

      nsAutoString loc;
      res = locale->GetCategory(NS_LITERAL_STRING(NSILOCALE_MESSAGE), loc);
      if (NS_FAILED(res))
        break;

      ToLowerCase(loc); 
      mLocaleLanguage = do_GetAtom(loc);
    }
  } while (0);

  if (aError)
    *aError = res;

  return mLocaleLanguage;
}

nsIAtom*
nsLanguageAtomService::GetLanguageGroup(nsIAtom *aLanguage,
                                        nsresult *aError)
{
  nsIAtom *retVal;
  nsresult res = NS_OK;

  retVal = mLangToGroup.GetWeak(aLanguage);

  if (!retVal) {
    nsAutoCString langStr;
    aLanguage->ToUTF8String(langStr);

    nsAutoCString langGroupStr;
    res = nsUConvPropertySearch::SearchPropertyValue(kLangGroups,
                                                     ArrayLength(kLangGroups),
                                                     langStr, langGroupStr);
    while (NS_FAILED(res)) {
      int32_t hyphen = langStr.RFindChar('-');
      if (hyphen <= 0) {
        langGroupStr.AssignLiteral("x-unicode");
        break;
      }
      langStr.Truncate(hyphen);
      res = nsUConvPropertySearch::SearchPropertyValue(kLangGroups,
                                                       ArrayLength(kLangGroups),
                                                       langStr, langGroupStr);
    }

    nsCOMPtr<nsIAtom> langGroup = do_GetAtom(langGroupStr);

    
    mLangToGroup.Put(aLanguage, langGroup);
    retVal = langGroup.get();
  }

  if (aError) {
    *aError = res;
  }

  return retVal;
}
