




































#include "nsCOMPtr.h"
#include "nsICharsetConverterManager.h"
#include "nsILanguageAtomService.h"
#include "nsIStringBundle.h"
#include "nsISupportsArray.h"
#include "nsCRT.h"
#include "nsInterfaceHashtable.h"
#include "nsIAtom.h"

#define NS_LANGUAGEATOMSERVICE_CID \
  {0xa6cf9120, 0x15b3, 0x11d2, {0x93, 0x2e, 0x00, 0x80, 0x5f, 0x8a, 0xdd, 0x32}}

class nsLanguageAtomService : public nsILanguageAtomService
{
public:
  NS_DECL_ISUPPORTS

  
  virtual NS_HIDDEN_(nsIAtom*)
    LookupLanguage(const nsAString &aLanguage, nsresult *aError);

  virtual NS_HIDDEN_(already_AddRefed<nsIAtom>)
    LookupCharSet(const char *aCharSet, nsresult *aError);

  virtual NS_HIDDEN_(nsIAtom*) GetLocaleLanguageGroup(nsresult *aError);

  nsLanguageAtomService() NS_HIDDEN;

private:
  NS_HIDDEN ~nsLanguageAtomService() { }

protected:
  NS_HIDDEN_(nsresult) InitLangGroupTable();

  nsCOMPtr<nsICharsetConverterManager> mCharSets;
  nsInterfaceHashtable<nsStringHashKey, nsIAtom> mLangs;
  nsCOMPtr<nsIStringBundle> mLangGroups;
  nsCOMPtr<nsIAtom> mLocaleLangGroup;
};
