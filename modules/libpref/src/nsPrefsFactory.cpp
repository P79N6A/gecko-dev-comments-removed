





































#include "mozilla/ModuleUtils.h"
#include "mozilla/Preferences.h"
#include "nsPrefBranch.h"
#include "prefapi.h"

using namespace mozilla;

NS_GENERIC_FACTORY_SINGLETON_CONSTRUCTOR(Preferences, Preferences::GetInstance)
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsPrefLocalizedString, Init)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsRelativeFilePref)

static NS_DEFINE_CID(kPrefServiceCID, NS_PREFSERVICE_CID);
static NS_DEFINE_CID(kPrefLocalizedStringCID, NS_PREFLOCALIZEDSTRING_CID);
static NS_DEFINE_CID(kRelativeFilePrefCID, NS_RELATIVEFILEPREF_CID);
 
static mozilla::Module::CIDEntry kPrefCIDs[] = {
  { &kPrefServiceCID, true, NULL, PreferencesConstructor },
  { &kPrefLocalizedStringCID, false, NULL, nsPrefLocalizedStringConstructor },
  { &kRelativeFilePrefCID, false, NULL, nsRelativeFilePrefConstructor },
  { NULL }
};

static mozilla::Module::ContractIDEntry kPrefContracts[] = {
  { NS_PREFSERVICE_CONTRACTID, &kPrefServiceCID },
  { NS_PREFLOCALIZEDSTRING_CONTRACTID, &kPrefLocalizedStringCID },
  { NS_RELATIVEFILEPREF_CONTRACTID, &kRelativeFilePrefCID },
  
  { "@mozilla.org/preferences;1", &kPrefServiceCID },
  { NULL }
};

static void
UnloadPrefsModule()
{
  Preferences::Shutdown();
}

static const mozilla::Module kPrefModule = {
  mozilla::Module::kVersion,
  kPrefCIDs,
  kPrefContracts,
  NULL,
  NULL,
  NULL,
  UnloadPrefsModule
};

NSMODULE_DEFN(nsPrefModule) = &kPrefModule;
