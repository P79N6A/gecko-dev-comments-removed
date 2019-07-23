




































#include "nsIComponentManager.h"
#include "nsLanguageAtomService.h"
#include "nsICharsetConverterManager.h"
#include "nsILocaleService.h"
#include "nsXPIDLString.h"
#include "nsUnicharUtils.h"
#include "nsIServiceManager.h"

NS_IMPL_ISUPPORTS1(nsLanguageAtomService, nsILanguageAtomService)

nsLanguageAtomService::nsLanguageAtomService()
{
  mLangs.Init();
}

nsresult
nsLanguageAtomService::InitLangGroupTable()
{
  if (mLangGroups) return NS_OK;
  nsresult rv;
  
  nsCOMPtr<nsIStringBundleService> bundleService =
    do_GetService(NS_STRINGBUNDLE_CONTRACTID, &rv);
  if (NS_FAILED(rv)) return rv;


  rv = bundleService->CreateBundle("resource://gre/res/langGroups.properties",
                                   getter_AddRefs(mLangGroups));
  return rv;
}

nsIAtom*
nsLanguageAtomService::LookupLanguage(const nsAString &aLanguage,
                                      nsresult *aError)
{
  nsresult res = NS_OK;

  nsAutoString lowered(aLanguage);
  ToLowerCase(lowered);

  nsIAtom *lang = mLangs.GetWeak(lowered);

  if (!lang) {
    nsXPIDLString langGroupStr;

    if (lowered.EqualsLiteral("en-us")) {
      langGroupStr.AssignLiteral("x-western");
    } else if (lowered.EqualsLiteral("de-de")) {
      langGroupStr.AssignLiteral("x-western");
    } else if (lowered.EqualsLiteral("ja-jp")) {
      langGroupStr.AssignLiteral("ja");
    } else {
      if (!mLangGroups) {
        if (NS_FAILED(InitLangGroupTable())) {
          if (aError)
            *aError = NS_ERROR_FAILURE;

          return nsnull;
        }
      }
      res = mLangGroups->GetStringFromName(lowered.get(), getter_Copies(langGroupStr));
      if (NS_FAILED(res)) {
        PRInt32 hyphen = lowered.FindChar('-');
        if (hyphen >= 0) {
          nsAutoString truncated(lowered);
          truncated.Truncate(hyphen);
          res = mLangGroups->GetStringFromName(truncated.get(), getter_Copies(langGroupStr));
          if (NS_FAILED(res)) {
            langGroupStr.AssignLiteral("x-unicode");
          }
        } else {
          langGroupStr.AssignLiteral("x-unicode");
        }
      }
    }
    nsCOMPtr<nsIAtom> langGroup = do_GetAtom(langGroupStr);

    
    mLangs.Put(lowered, langGroup);
    lang = langGroup;
  }

  if (aError)
    *aError = res;

  return lang;
}

already_AddRefed<nsIAtom>
nsLanguageAtomService::LookupCharSet(const char *aCharSet, nsresult *aError)
{
  if (!mCharSets) {
    mCharSets = do_GetService(NS_CHARSETCONVERTERMANAGER_CONTRACTID);
    if (!mCharSets) {
      if (aError)
        *aError = NS_ERROR_FAILURE;

      return nsnull;
    }
  }

  nsCOMPtr<nsIAtom> langGroup;
  mCharSets->GetCharsetLangGroup(aCharSet, getter_AddRefs(langGroup));
  if (!langGroup) {
    if (aError)
      *aError = NS_ERROR_FAILURE;

    return nsnull;
  }

  
  nsIAtom *raw = nsnull;
  langGroup.swap(raw);

  if (aError)
    *aError = NS_OK;

  return raw;
}

nsIAtom*
nsLanguageAtomService::GetLocaleLanguageGroup(nsresult *aError)
{
  nsresult res = NS_OK;

  do {
    if (!mLocaleLangGroup) {
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

      nsAutoString category;
      category.AssignWithConversion(NSILOCALE_MESSAGE);
      nsAutoString loc;
      res = locale->GetCategory(category, loc);
      if (NS_FAILED(res))
        break;

      mLocaleLangGroup = LookupLanguage(loc, &res);
    }
  } while (0);

  if (aError)
    *aError = res;

  return mLocaleLangGroup;
}
