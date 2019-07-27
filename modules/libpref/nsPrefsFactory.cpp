




#include "mozilla/ModuleUtils.h"
#include "mozilla/Preferences.h"
#include "nsPrefBranch.h"
#include "prefapi.h"

using namespace mozilla;

NS_GENERIC_FACTORY_SINGLETON_CONSTRUCTOR(Preferences,
                                         Preferences::GetInstanceForService)
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(nsPrefLocalizedString, Init)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsRelativeFilePref)

static NS_DEFINE_CID(kPrefServiceCID, NS_PREFSERVICE_CID);
static NS_DEFINE_CID(kPrefLocalizedStringCID, NS_PREFLOCALIZEDSTRING_CID);
static NS_DEFINE_CID(kRelativeFilePrefCID, NS_RELATIVEFILEPREF_CID);
 
static mozilla::Module::CIDEntry kPrefCIDs[] = {
  { &kPrefServiceCID, true, nullptr, PreferencesConstructor },
  { &kPrefLocalizedStringCID, false, nullptr, nsPrefLocalizedStringConstructor },
  { &kRelativeFilePrefCID, false, nullptr, nsRelativeFilePrefConstructor },
  { nullptr }
};

static mozilla::Module::ContractIDEntry kPrefContracts[] = {
  { NS_PREFSERVICE_CONTRACTID, &kPrefServiceCID },
  { NS_PREFLOCALIZEDSTRING_CONTRACTID, &kPrefLocalizedStringCID },
  { NS_RELATIVEFILEPREF_CONTRACTID, &kRelativeFilePrefCID },
  
  { "@mozilla.org/preferences;1", &kPrefServiceCID },
  { nullptr }
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
  nullptr,
  nullptr,
  nullptr,
  UnloadPrefsModule
};

NSMODULE_DEFN(nsPrefModule) = &kPrefModule;
