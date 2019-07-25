








































#ifndef nsILanguageAtomService_h_
#define nsILanguageAtomService_h_






#include "nsISupports.h"
#include "nsCOMPtr.h"
#include "nsIAtom.h"

#define NS_ILANGUAGEATOMSERVICE_IID \
  {0xAF4C48CF, 0x8F76, 0x4477, \
    { 0xA7, 0x0E, 0xAB, 0x09, 0x74, 0xE2, 0x41, 0xF0 }}

#define NS_LANGUAGEATOMSERVICE_CONTRACTID \
  "@mozilla.org/intl/nslanguageatomservice;1"

class nsILanguageAtomService : public nsISupports
{
 public: 
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_ILANGUAGEATOMSERVICE_IID)

  virtual nsIAtom* LookupLanguage(const nsACString &aLanguage,
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
