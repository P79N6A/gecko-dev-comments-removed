






































#include "nsIGenericFactory.h"
#include "nsIModule.h"

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

static const nsModuleComponentInfo components[] =
{
#ifdef USE_ICON_DECODER
  { "icon decoder",
    NS_ICONDECODER_CID,
    "@mozilla.org/image/decoder;2?type=image/icon",
    nsIconDecoderConstructor, },
#endif

   { "Icon Protocol Handler",      
      NS_ICONPROTOCOL_CID,
      NS_NETWORK_PROTOCOL_CONTRACTID_PREFIX "moz-icon",
      nsIconProtocolHandlerConstructor
    }
};

PR_STATIC_CALLBACK(nsresult)
IconDecoderModuleCtor(nsIModule* aSelf)
{
  nsMozIconURI::InitAtoms();
  return NS_OK;
}

PR_STATIC_CALLBACK(void)
IconDecoderModuleDtor(nsIModule* aSelf)
{
#ifdef MOZ_ENABLE_GNOMEUI
  nsIconChannel::Shutdown();
#endif
}

NS_IMPL_NSGETMODULE_WITH_CTOR_DTOR(nsIconDecoderModule, components,
                                   IconDecoderModuleCtor, IconDecoderModuleDtor)
