




































#include "nsIGenericFactory.h"
#include "nsCOMPtr.h"
#include "nsICategoryManager.h"
#include "nsIServiceManager.h"


#include "nsLWBrkConstructors.h"
#include "nsSemanticUnitScanner.h"


#include "nsUcharUtilConstructors.h"


#include "nsStrBundleConstructors.h"


#include "nsLocaleConstructors.h"


NS_GENERIC_FACTORY_CONSTRUCTOR(nsSemanticUnitScanner)

static nsModuleComponentInfo components[] =
{
 
  { "Line Breaker", NS_LBRK_CID, 
    NS_LBRK_CONTRACTID, nsJISx4051LineBreakerConstructor},
  { "Word Breaker", NS_WBRK_CID,
    NS_WBRK_CONTRACTID, nsSampleWordBreakerConstructor},
  { "Semantic Unit Scanner", NS_SEMANTICUNITSCANNER_CID,
    NS_SEMANTICUNITSCANNER_CONTRACTID, nsSemanticUnitScannerConstructor},

 
  { "Unichar Utility", NS_UNICHARUTIL_CID, 
      NS_UNICHARUTIL_CONTRACTID, nsCaseConversionImp2Constructor},
  { "Unicode To Entity Converter", NS_ENTITYCONVERTER_CID, 
      NS_ENTITYCONVERTER_CONTRACTID, nsEntityConverterConstructor },
  { "Unicode To Charset Converter", NS_SAVEASCHARSET_CID, 
      NS_SAVEASCHARSET_CONTRACTID, nsSaveAsCharsetConstructor},
  { "Japanese Hankaku To Zenkaku", NS_HANKAKUTOZENKAKU_CID, 
      NS_HANKAKUTOZENKAKU_CONTRACTID, CreateNewHankakuToZenkaku},
  { "Unicode Normlization", NS_UNICODE_NORMALIZER_CID, 
      NS_UNICODE_NORMALIZER_CONTRACTID,  nsUnicodeNormalizerConstructor},


 
  { "String Bundle", NS_STRINGBUNDLESERVICE_CID, NS_STRINGBUNDLE_CONTRACTID,
    nsStringBundleServiceConstructor},
  { "String Textfile Overrides", NS_STRINGBUNDLETEXTOVERRIDE_CID,
    NS_STRINGBUNDLETEXTOVERRIDE_CONTRACTID,
    nsStringBundleTextOverrideConstructor },

 
  { "nsLocaleService component",
    NS_LOCALESERVICE_CID,
    NS_LOCALESERVICE_CONTRACTID,
    CreateLocaleService },
  { "Collation factory",
    NS_COLLATIONFACTORY_CID,
    NS_COLLATIONFACTORY_CONTRACTID,
    nsCollationFactoryConstructor },
  { "Scriptable Date Format",
    NS_SCRIPTABLEDATEFORMAT_CID,
    NS_SCRIPTABLEDATEFORMAT_CONTRACTID,
    NS_NewScriptableDateFormat },
  { "Language Atom Service",
    NS_LANGUAGEATOMSERVICE_CID,
    NS_LANGUAGEATOMSERVICE_CONTRACTID,
    nsLanguageAtomServiceConstructor },
 
#ifdef XP_WIN 
  { "Platform locale",
    NS_WIN32LOCALE_CID,
    NS_WIN32LOCALE_CONTRACTID,
    nsIWin32LocaleImplConstructor },
  { "Collation",
    NS_COLLATION_CID,
    NS_COLLATION_CONTRACTID,
    nsCollationWinConstructor },
  { "Date/Time formatter",
    NS_DATETIMEFORMAT_CID,
    NS_DATETIMEFORMAT_CONTRACTID,
    nsDateTimeFormatWinConstructor },
#endif
 
#ifdef USE_UNIX_LOCALE
  { "Platform locale",
    NS_POSIXLOCALE_CID,
    NS_POSIXLOCALE_CONTRACTID,
    nsPosixLocaleConstructor },

  { "Collation",
    NS_COLLATION_CID,
    NS_COLLATION_CONTRACTID,
    nsCollationUnixConstructor },

  { "Date/Time formatter",
    NS_DATETIMEFORMAT_CID,
    NS_DATETIMEFORMAT_CONTRACTID,
    nsDateTimeFormatUnixConstructor },
#endif

#ifdef USE_MAC_LOCALE
  { "Mac locale",
    NS_MACLOCALE_CID,
    NS_MACLOCALE_CONTRACTID,
    nsMacLocaleConstructor },
  { "Collation",
    NS_COLLATION_CID,
    NS_COLLATION_CONTRACTID,
#ifdef USE_UCCOLLATIONKEY
    nsCollationMacUCConstructor },
#else
    nsCollationMacConstructor },
#endif
  { "Date/Time formatter",
    NS_DATETIMEFORMAT_CID,
    NS_DATETIMEFORMAT_CONTRACTID,
    nsDateTimeFormatMacConstructor },
#endif

#ifdef XP_OS2
  { "OS/2 locale",
    NS_OS2LOCALE_CID,
    NS_OS2LOCALE_CONTRACTID,
    nsOS2LocaleConstructor },
  { "Collation",
    NS_COLLATION_CID,
    NS_COLLATION_CONTRACTID,
    nsCollationOS2Constructor },
  { "Date/Time formatter",
    NS_DATETIMEFORMAT_CID,
    NS_DATETIMEFORMAT_CONTRACTID,
    nsDateTimeFormatOS2Constructor },
#endif
      
};


NS_IMPL_NSGETMODULE(nsI18nModule, components)
