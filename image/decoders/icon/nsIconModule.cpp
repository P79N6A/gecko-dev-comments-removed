





#include "mozilla/ModuleUtils.h"
#include "nsServiceManagerUtils.h"

#include "nsIconProtocolHandler.h"
#include "nsIconURI.h"
#include "nsIconChannel.h"





#define NS_ICONPROTOCOL_CID { 0xd0f9db12, 0x249c, 0x11d5, \
                              { 0x99, 0x5, 0x0, 0x10, 0x83, 0x1, 0xe, 0x9b } }

NS_GENERIC_FACTORY_CONSTRUCTOR(nsIconProtocolHandler)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsMozIconURI)

NS_DEFINE_NAMED_CID(NS_ICONPROTOCOL_CID);
NS_DEFINE_NAMED_CID(NS_MOZICONURI_CID);

static const mozilla::Module::CIDEntry kIconCIDs[] = {
  { &kNS_ICONPROTOCOL_CID, false, nullptr, nsIconProtocolHandlerConstructor },
  { &kNS_MOZICONURI_CID, false, nullptr, nsMozIconURIConstructor },
  { nullptr }
};

static const mozilla::Module::ContractIDEntry kIconContracts[] = {
  { NS_NETWORK_PROTOCOL_CONTRACTID_PREFIX "moz-icon", &kNS_ICONPROTOCOL_CID },
  { nullptr }
};

static const mozilla::Module::CategoryEntry kIconCategories[] = {
  { nullptr }
};

static void
IconDecoderModuleDtor()
{
#if (MOZ_WIDGET_GTK == 2)
  nsIconChannel::Shutdown();
#endif
}

static const mozilla::Module kIconModule = {
  mozilla::Module::kVersion,
  kIconCIDs,
  kIconContracts,
  kIconCategories,
  nullptr,
  nullptr,
  IconDecoderModuleDtor
};

NSMODULE_DEFN(nsIconDecoderModule) = &kIconModule;
