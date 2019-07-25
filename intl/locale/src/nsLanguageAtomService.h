




































#include "nsCOMPtr.h"
#include "nsICharsetConverterManager.h"
#include "nsILanguageAtomService.h"
#include "nsIStringBundle.h"
#include "nsCRT.h"
#include "nsInterfaceHashtable.h"
#include "nsIAtom.h"

#define NS_LANGUAGEATOMSERVICE_CID \
  {0xB7C65853, 0x2996, 0x435E, {0x96, 0x54, 0xDC, 0xC1, 0x78, 0xAA, 0xB4, 0x8C}}

class nsLanguageAtomService : public nsILanguageAtomService
{
public:
  NS_DECL_ISUPPORTS

  
  virtual NS_HIDDEN_(nsIAtom*)
    LookupLanguage(const nsACString &aLanguage, nsresult *aError);

  virtual NS_HIDDEN_(already_AddRefed<nsIAtom>)
    LookupCharSet(const char *aCharSet, nsresult *aError);

  virtual NS_HIDDEN_(nsIAtom*) GetLocaleLanguage(nsresult *aError);

  virtual NS_HIDDEN_(nsIAtom*) GetLanguageGroup(nsIAtom *aLanguage,
                                                nsresult *aError);

  nsLanguageAtomService() NS_HIDDEN;

private:
  NS_HIDDEN ~nsLanguageAtomService() { }

protected:
  NS_HIDDEN_(nsresult) InitLangGroupTable();

  nsCOMPtr<nsICharsetConverterManager> mCharSets;
  nsInterfaceHashtable<nsISupportsHashKey, nsIAtom> mLangToGroup;
  nsCOMPtr<nsIStringBundle> mLangGroups;
  nsCOMPtr<nsIAtom> mLocaleLanguage;
};
