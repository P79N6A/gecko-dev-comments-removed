



#include "mozilla/ModuleUtils.h"
#include "nsIClassInfoImpl.h"

#ifdef MOZ_WEBRTC

#include "nsPicoService.h"

using namespace mozilla::dom;

#define PICOSERVICE_CID \
  {0x346c4fc8, 0x12fe, 0x459c, {0x81, 0x19, 0x9a, 0xa7, 0x73, 0x37, 0x7f, 0xf4}}

#define PICOSERVICE_CONTRACTID "@mozilla.org/synthpico;1"


NS_GENERIC_FACTORY_SINGLETON_CONSTRUCTOR(nsPicoService,
                                         nsPicoService::GetInstanceForService)


NS_DEFINE_NAMED_CID(PICOSERVICE_CID);

static const mozilla::Module::CIDEntry kCIDs[] = {
  { &kPICOSERVICE_CID, true, NULL, nsPicoServiceConstructor },
  { NULL }
};

static const mozilla::Module::ContractIDEntry kContracts[] = {
  { PICOSERVICE_CONTRACTID, &kPICOSERVICE_CID },
  { NULL }
};

static const mozilla::Module::CategoryEntry kCategories[] = {
  { "profile-after-change", "Pico Speech Synth", PICOSERVICE_CONTRACTID },
  { NULL }
};

static void
UnloadPicoModule()
{
  nsPicoService::Shutdown();
}

static const mozilla::Module kModule = {
  mozilla::Module::kVersion,
  kCIDs,
  kContracts,
  kCategories,
  NULL,
  NULL,
  UnloadPicoModule
};

NSMODULE_DEFN(synthpico) = &kModule;
#endif
