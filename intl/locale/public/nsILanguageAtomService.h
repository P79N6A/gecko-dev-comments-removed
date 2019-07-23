







































#ifndef nsILanguageAtomService_h_
#define nsILanguageAtomService_h_






#include "nsISupports.h"
#include "nsCOMPtr.h"
#include "nsIAtom.h"

#define NS_ILANGUAGEATOMSERVICE_IID \
  {0x24b45737, 0x9e94, 0x4e40, \
    { 0x9d, 0x59, 0x29, 0xcd, 0x62, 0x96, 0x3a, 0xdd }}

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

  virtual nsIAtom* GetLocaleLanguageGroup(nsresult *aError = nsnull) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsILanguageAtomService,
                              NS_ILANGUAGEATOMSERVICE_IID)

#endif
