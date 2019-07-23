





































#ifndef nsLocaleConstructors_h__
#define nsLocaleConstructors_h__

#include "nsCollationCID.h"
#include "nsDateTimeFormatCID.h"
#include "nsIGenericFactory.h"
#include "nsILocaleService.h"
#include "nsIScriptableDateFormat.h"
#include "nsIServiceManager.h"
#include "nsLanguageAtomService.h"
#include "nsLocaleCID.h"

#if defined(XP_MAC) || defined(XP_MACOSX)
#define USE_MAC_LOCALE
#endif

#if (defined(XP_UNIX) && !defined(XP_MACOSX)) || defined(XP_BEOS)
#define USE_UNIX_LOCALE
#endif

#ifdef XP_WIN
#include "nsIwin32LocaleImpl.h"
#include "nsCollationWin.h"
#include "nsDateTimeFormatWin.h"
#endif

#ifdef XP_OS2
#include "nsOS2Locale.h"
#include "nsCollationOS2.h"
#include "nsDateTimeFormatOS2.h"
#endif

#ifdef USE_MAC_LOCALE
#ifdef USE_UCCOLLATIONKEY
#include "nsCollationMacUC.h"
#else
#include "nsCollationMac.h"
#endif
#include "nsDateTimeFormatMac.h"
#include "nsMacLocale.h"
#endif

#ifdef USE_UNIX_LOCALE
#include "nsCollationUnix.h"
#include "nsDateTimeFormatUnix.h"
#include "nsPosixLocale.h"
#endif

#define NSLOCALE_MAKE_CTOR(ctor_, iface_, func_)          \
static NS_IMETHODIMP                                      \
ctor_(nsISupports* aOuter, REFNSIID aIID, void** aResult) \
{                                                         \
  *aResult = nsnull;                                      \
  if (aOuter)                                             \
    return NS_ERROR_NO_AGGREGATION;                       \
  iface_* inst;                                           \
  nsresult rv = func_(&inst);                             \
  if (NS_SUCCEEDED(rv)) {                                 \
    rv = inst->QueryInterface(aIID, aResult);             \
    NS_RELEASE(inst);                                     \
  }                                                       \
  return rv;                                              \
}


NSLOCALE_MAKE_CTOR(CreateLocaleService, nsILocaleService, NS_NewLocaleService)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsCollationFactory)

NS_GENERIC_FACTORY_CONSTRUCTOR(nsLanguageAtomService)


#ifdef XP_WIN
NS_GENERIC_FACTORY_CONSTRUCTOR(nsIWin32LocaleImpl)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsCollationWin)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsDateTimeFormatWin)
#endif

#ifdef USE_UNIX_LOCALE
NS_GENERIC_FACTORY_CONSTRUCTOR(nsPosixLocale)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsCollationUnix)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsDateTimeFormatUnix)
#endif  

#ifdef USE_MAC_LOCALE
NS_GENERIC_FACTORY_CONSTRUCTOR(nsMacLocale)
#ifdef USE_UCCOLLATIONKEY 
NS_GENERIC_FACTORY_CONSTRUCTOR(nsCollationMacUC)
#else
NS_GENERIC_FACTORY_CONSTRUCTOR(nsCollationMac)
#endif
NS_GENERIC_FACTORY_CONSTRUCTOR(nsDateTimeFormatMac)
#endif  

#ifdef XP_OS2
NS_GENERIC_FACTORY_CONSTRUCTOR(nsOS2Locale)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsCollationOS2)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsDateTimeFormatOS2)
#endif

#endif
