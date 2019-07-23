





































#include "nsIGenericFactory.h"
#include "nsPrefService.h"
#include "nsPrefBranch.h"
#include "prefapi.h"


NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsPrefService, Init)
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsPrefLocalizedString, Init)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsRelativeFilePref)
 

static const nsModuleComponentInfo components[] = 
{
  {
    NS_PREFSERVICE_CLASSNAME, 
    NS_PREFSERVICE_CID,
    NS_PREFSERVICE_CONTRACTID, 
    nsPrefServiceConstructor
  },

  {
    NS_PREFLOCALIZEDSTRING_CLASSNAME, 
    NS_PREFLOCALIZEDSTRING_CID,
    NS_PREFLOCALIZEDSTRING_CONTRACTID, 
    nsPrefLocalizedStringConstructor
  },

  {
    NS_RELATIVEFILEPREF_CLASSNAME, 
    NS_RELATIVEFILEPREF_CID,
    NS_RELATIVEFILEPREF_CONTRACTID, 
    nsRelativeFilePrefConstructor
  },

  { 
    NS_PREFSERVICE_CLASSNAME,
    NS_PREFSERVICE_CID,
    "@mozilla.org/preferences;1",
    nsPrefServiceConstructor
  }
};

static void
UnloadPrefsModule(nsIModule* unused)
{
  PREF_Cleanup();
}

NS_IMPL_NSGETMODULE_WITH_DTOR(nsPrefModule, components, UnloadPrefsModule)
