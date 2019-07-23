






































#include "nsIGenericFactory.h"
#include "nsIModule.h"
#include "nsICategoryManager.h"
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

static NS_METHOD IconDecoderRegisterProc(nsIComponentManager *aCompMgr,
                                   nsIFile *aPath,
                                   const char *registryLocation,
                                   const char *componentType,
                                   const nsModuleComponentInfo *info) {
  nsresult rv;
  nsCOMPtr<nsICategoryManager> catMan(do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv));
  if (NS_FAILED(rv))
    return rv;
  catMan->AddCategoryEntry("Gecko-Content-Viewers", gIconMimeType,
                           "@mozilla.org/content/document-loader-factory;1",
                           PR_TRUE, PR_TRUE, nsnull);
  return NS_OK;
}

static NS_METHOD IconDecoderUnregisterProc(nsIComponentManager *aCompMgr,
                                     nsIFile *aPath,
                                     const char *registryLocation,
                                     const nsModuleComponentInfo *info) {
  nsresult rv;
  nsCOMPtr<nsICategoryManager> catMan(do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv));
  if (NS_FAILED(rv))
    return rv;
  catMan->DeleteCategoryEntry("Gecko-Content-Viewers", gIconMimeType, PR_TRUE);
  return NS_OK;
}

#endif

static const nsModuleComponentInfo components[] =
{
#ifdef USE_ICON_DECODER
  { "icon decoder",
    NS_ICONDECODER_CID,
    "@mozilla.org/image/decoder;3?type=image/icon",
    nsIconDecoderConstructor,
    IconDecoderRegisterProc,
    IconDecoderUnregisterProc, },
#endif

   { "Icon Protocol Handler",      
      NS_ICONPROTOCOL_CID,
      NS_NETWORK_PROTOCOL_CONTRACTID_PREFIX "moz-icon",
      nsIconProtocolHandlerConstructor
    }
};

static nsresult
IconDecoderModuleCtor(nsIModule* aSelf)
{
  return NS_OK;
}

static void
IconDecoderModuleDtor(nsIModule* aSelf)
{
#ifdef MOZ_WIDGET_GTK2
  nsIconChannel::Shutdown();
#endif
}

NS_IMPL_NSGETMODULE_WITH_CTOR_DTOR(nsIconDecoderModule, components,
                                   IconDecoderModuleCtor, IconDecoderModuleDtor)
