





































#include "nsIGenericFactory.h"

#include "nsBrowserCompsCID.h"
#include "nsPlacesImportExportService.h"

#if defined(XP_WIN)
#include "nsWindowsShellService.h"
#elif defined(XP_MACOSX)
#include "nsMacShellService.h"
#elif defined(MOZ_WIDGET_GTK2)
#include "nsGNOMEShellService.h"
#endif

#ifndef WINCE

#include "nsProfileMigrator.h"
#if !defined(XP_BEOS)
#include "nsDogbertProfileMigrator.h"
#endif
#if !defined(XP_OS2)
#include "nsOperaProfileMigrator.h"
#endif
#include "nsPhoenixProfileMigrator.h"
#include "nsSeamonkeyProfileMigrator.h"
#if defined(XP_WIN) && !defined(__MINGW32__)
#include "nsIEProfileMigrator.h"
#elif defined(XP_MACOSX)
#include "nsSafariProfileMigrator.h"
#include "nsOmniWebProfileMigrator.h"
#include "nsMacIEProfileMigrator.h"
#include "nsCaminoProfileMigrator.h"
#include "nsICabProfileMigrator.h"
#endif

#endif 

#include "rdf.h"
#include "nsFeedSniffer.h"
#include "nsAboutFeeds.h"
#include "nsIAboutModule.h"



NS_GENERIC_FACTORY_CONSTRUCTOR(nsPlacesImportExportService)
#if defined(XP_WIN)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsWindowsShellService)
#elif defined(XP_MACOSX)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsMacShellService)
#elif defined(MOZ_WIDGET_GTK2)
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsGNOMEShellService, Init)
#endif

#ifndef WINCE

#if !defined(XP_BEOS)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsDogbertProfileMigrator)
#endif
#if !defined(XP_OS2)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsOperaProfileMigrator)
#endif
NS_GENERIC_FACTORY_CONSTRUCTOR(nsPhoenixProfileMigrator)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsProfileMigrator)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsSeamonkeyProfileMigrator)
#if defined(XP_WIN) && !defined(__MINGW32__)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsIEProfileMigrator)
#elif defined(XP_MACOSX)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsSafariProfileMigrator)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsOmniWebProfileMigrator)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsMacIEProfileMigrator)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsCaminoProfileMigrator)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsICabProfileMigrator)
#endif

#endif

NS_GENERIC_FACTORY_CONSTRUCTOR(nsFeedSniffer)



static const nsModuleComponentInfo components[] =
{
#if defined(XP_WIN)
  { "Browser Shell Service",
    NS_SHELLSERVICE_CID,
    NS_SHELLSERVICE_CONTRACTID,
    nsWindowsShellServiceConstructor},

#elif defined(MOZ_WIDGET_GTK2)
  { "Browser Shell Service",
    NS_SHELLSERVICE_CID,
    NS_SHELLSERVICE_CONTRACTID,
    nsGNOMEShellServiceConstructor },

#endif


  { "Places Import/Export Service",
    NS_PLACESIMPORTEXPORTSERVICE_CID,
    NS_PLACESIMPORTEXPORTSERVICE_CONTRACTID,
    nsPlacesImportExportServiceConstructor},

  { "Feed Sniffer",
    NS_FEEDSNIFFER_CID,
    NS_FEEDSNIFFER_CONTRACTID,
    nsFeedSnifferConstructor,
    nsFeedSniffer::Register },

  { "about:feeds Page",
    NS_ABOUTFEEDS_CID,
    NS_ABOUT_MODULE_CONTRACTID_PREFIX "feeds",
    nsAboutFeeds::Create
  },

#ifndef WINCE

  { "Profile Migrator",
    NS_FIREFOX_PROFILEMIGRATOR_CID,
    NS_PROFILEMIGRATOR_CONTRACTID,
    nsProfileMigratorConstructor },

#if defined(XP_WIN) && !defined(__MINGW32__)
  { "Internet Explorer (Windows) Profile Migrator",
    NS_WINIEPROFILEMIGRATOR_CID,
    NS_BROWSERPROFILEMIGRATOR_CONTRACTID_PREFIX "ie",
    nsIEProfileMigratorConstructor },

#elif defined(XP_MACOSX)
  { "Browser Shell Service",
    NS_SHELLSERVICE_CID,
    NS_SHELLSERVICE_CONTRACTID,
    nsMacShellServiceConstructor },

  { "Safari Profile Migrator",
    NS_SAFARIPROFILEMIGRATOR_CID,
    NS_BROWSERPROFILEMIGRATOR_CONTRACTID_PREFIX "safari",
    nsSafariProfileMigratorConstructor },

  { "Internet Explorer (Macintosh) Profile Migrator",
    NS_MACIEPROFILEMIGRATOR_CID,
    NS_BROWSERPROFILEMIGRATOR_CONTRACTID_PREFIX "macie",
    nsMacIEProfileMigratorConstructor },

  { "OmniWeb Profile Migrator",
    NS_OMNIWEBPROFILEMIGRATOR_CID,
    NS_BROWSERPROFILEMIGRATOR_CONTRACTID_PREFIX "omniweb",
    nsOmniWebProfileMigratorConstructor },

  { "Camino Profile Migrator",
    NS_CAMINOPROFILEMIGRATOR_CID,
    NS_BROWSERPROFILEMIGRATOR_CONTRACTID_PREFIX "camino",
    nsCaminoProfileMigratorConstructor },

  { "iCab Profile Migrator",
    NS_ICABPROFILEMIGRATOR_CID,
    NS_BROWSERPROFILEMIGRATOR_CONTRACTID_PREFIX "icab",
    nsICabProfileMigratorConstructor },

#endif

#if !defined(XP_OS2)
  { "Opera Profile Migrator",
    NS_OPERAPROFILEMIGRATOR_CID,
    NS_BROWSERPROFILEMIGRATOR_CONTRACTID_PREFIX "opera",
    nsOperaProfileMigratorConstructor },
#endif

#if !defined(XP_BEOS)
  { "Netscape 4.x Profile Migrator",
    NS_DOGBERTPROFILEMIGRATOR_CID,
    NS_BROWSERPROFILEMIGRATOR_CONTRACTID_PREFIX "dogbert",
    nsDogbertProfileMigratorConstructor },
#endif

  { "Phoenix Profile Migrator",
    NS_PHOENIXPROFILEMIGRATOR_CID,
    NS_BROWSERPROFILEMIGRATOR_CONTRACTID_PREFIX "phoenix",
    nsPhoenixProfileMigratorConstructor },

  { "Seamonkey Profile Migrator",
    NS_SEAMONKEYPROFILEMIGRATOR_CID,
    NS_BROWSERPROFILEMIGRATOR_CONTRACTID_PREFIX "seamonkey",
    nsSeamonkeyProfileMigratorConstructor }

#endif 
};

NS_IMPL_NSGETMODULE(nsBrowserCompsModule, components)

