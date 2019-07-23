








































#ifndef nsILanguageAtomService_h_
#define nsILanguageAtomService_h_






#include "nsISupports.h"
#include "nsCOMPtr.h"
#include "nsIAtom.h"

#define NS_ILANGUAGEATOMSERVICE_IID \
  {0xE8ABCA7C, 0x3909, 0x4DBC, \
    { 0x9D, 0x03, 0xD3, 0xB5, 0xBE, 0xE4, 0xFD, 0x3F }}

#define NS_LANGUAGEATOMSERVICE_CONTRACTID \
  "@mozilla.org/intl/nslanguageatomservice;1"

class nsILanguageAtomService : public nsISupports
{
 public: 
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ILANGUAGEATOMSERVICE_IID)

  virtual nsIAtom* LookupLanguage(const nsAString &aLanguage,
                                  nsresult *aError = nsnull) = 0;
  virtual already_AddRefed<nsIAtom>
  LookupCharSet(const char *aCharSet, nsresult *aError = nsnull) = 0;

  virtual nsIAtom* GetLocaleLanguage(nsresult *aError = nsnull) = 0;

  virtual nsIAtom* GetLanguageGroup(nsIAtom *aLanguage,
                                    nsresult *aError = nsnull) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsILanguageAtomService,
                              NS_ILANGUAGEATOMSERVICE_IID)

#endif
