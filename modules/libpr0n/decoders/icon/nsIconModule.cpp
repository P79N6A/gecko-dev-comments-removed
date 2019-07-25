






































#include "mozilla/ModuleUtils.h"
#include "nsServiceManagerUtils.h"

#ifdef USE_ICON_DECODER
#include "nsIconDecoder.h"
#endif
#include "nsIconProtocolHandler.h"
#include "nsIconURI.h"
#include "nsIconChannel.h"





#define NS_ICONPROTOCOL_CID   { 0xd0f9db12, 0x249c, 0x11d5, { 0x99, 0x5, 0x0, 0x10, 0x83, 0x1, 0xe, 0x9b } } 

#ifdef USE_ICON_DECODER
NS_GENERIC_FACTORY_CONSTRUCTOR(nsIconDecoder)
#endif
NS_GENERIC_FACTORY_CONSTRUCTOR(nsIconProtocolHandler)

#ifdef USE_ICON_DECODER
static const char gIconMimeType[] = "image/icon";
NS_DEFINE_NAMED_CID(NS_ICONDECODER_CID);
#endif

NS_DEFINE_NAMED_CID(NS_ICONPROTOCOL_CID);

static const mozilla::Module::CIDEntry kIconCIDs[] = {
#ifdef USE_ICON_DECODER
  { &kNS_ICONDECODER_CID, false, NULL, nsIconDecoderConstructor },
#endif
  { &kNS_ICONPROTOCOL_CID, false, NULL, nsIconProtocolHandlerConstructor },
  { NULL }
};

static const mozilla::Module::ContractIDEntry kIconContracts[] = {
#ifdef USE_ICON_DECODER
  { "@mozilla.org/image/decoder;3?type=image/icon", &kNS_ICONDECODER_CID },
#endif
  { NS_NETWORK_PROTOCOL_CONTRACTID_PREFIX "moz-icon", &kNS_ICONPROTOCOL_CID },
  { NULL }
};

static const mozilla::Module::CategoryEntry kIconCategories[] = {
#ifdef USE_ICON_DECODER
  { "Gecko-Content-Viewers", gIconMimeType, "@mozilla.org/content/document-loader-factory;1" },
#endif
  { NULL }
};

static void
IconDecoderModuleDtor()
{
#ifdef MOZ_WIDGET_GTK2
  nsIconChannel::Shutdown();
#endif
}

static const mozilla::Module kIconModule = {
  mozilla::Module::kVersion,
  kIconCIDs,
  kIconContracts,
  kIconCategories,
  NULL,
  NULL,
  IconDecoderModuleDtor
};

NSMODULE_DEFN(nsIconDecoderModule) = &kIconModule;
