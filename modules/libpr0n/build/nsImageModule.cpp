






































#include "nsImgBuildDefines.h"

#ifdef XP_MAC
#define IMG_BUILD_gif 1
#define IMG_BUILD_bmp 1
#define IMG_BUILD_png 1
#define IMG_BUILD_jpeg 1
#endif

#include "nsIDeviceContext.h"
#include "nsIGenericFactory.h"
#include "nsIModule.h"
#include "nsICategoryManager.h"
#include "nsXPCOMCID.h"
#include "nsServiceManagerUtils.h"

#include "imgContainer.h"
#include "imgLoader.h"
#include "imgRequest.h"
#include "imgRequestProxy.h"
#include "imgTools.h"

#ifdef IMG_BUILD_DECODER_gif

#include "nsGIFDecoder2.h"
#endif

#ifdef IMG_BUILD_DECODER_bmp

#include "nsBMPDecoder.h"
#include "nsICODecoder.h"
#endif

#ifdef IMG_BUILD_DECODER_png

#include "nsPNGDecoder.h"
#endif

#ifdef IMG_BUILD_DECODER_jpeg

#include "nsJPEGDecoder.h"
#endif

#ifdef IMG_BUILD_ENCODER_png

#include "nsPNGEncoder.h"
#endif
#ifdef IMG_BUILD_ENCODER_jpeg

#include "nsJPEGEncoder.h"
#endif




NS_GENERIC_FACTORY_CONSTRUCTOR(imgContainer)
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(imgLoader, Init)
NS_GENERIC_FACTORY_CONSTRUCTOR(imgRequestProxy)
NS_GENERIC_FACTORY_CONSTRUCTOR(imgTools)

#ifdef IMG_BUILD_DECODER_gif

NS_GENERIC_FACTORY_CONSTRUCTOR(nsGIFDecoder2)
#endif

#ifdef IMG_BUILD_DECODER_jpeg

NS_GENERIC_FACTORY_CONSTRUCTOR(nsJPEGDecoder)
#endif
#ifdef IMG_BUILD_ENCODER_jpeg

NS_GENERIC_FACTORY_CONSTRUCTOR(nsJPEGEncoder)
#endif

#ifdef IMG_BUILD_DECODER_bmp

NS_GENERIC_FACTORY_CONSTRUCTOR(nsICODecoder)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsBMPDecoder)
#endif

#ifdef IMG_BUILD_DECODER_png

NS_GENERIC_FACTORY_CONSTRUCTOR(nsPNGDecoder)
#endif
#ifdef IMG_BUILD_ENCODER_png

NS_GENERIC_FACTORY_CONSTRUCTOR(nsPNGEncoder)
#endif

static const char* gImageMimeTypes[] = {
#ifdef IMG_BUILD_DECODER_gif
  "image/gif",
#endif
#ifdef IMG_BUILD_DECODER_jpeg
  "image/jpeg",
  "image/pjpeg",
  "image/jpg",
#endif
#ifdef IMG_BUILD_DECODER_bmp
  "image/x-icon",
  "image/vnd.microsoft.icon",
  "image/bmp",
  "image/x-ms-bmp",
#endif
#ifdef IMG_BUILD_DECODER_png
  "image/png",
  "image/x-png",
#endif
};

static NS_METHOD ImageRegisterProc(nsIComponentManager *aCompMgr,
                                   nsIFile *aPath,
                                   const char *registryLocation,
                                   const char *componentType,
                                   const nsModuleComponentInfo *info) {
  nsresult rv;
  nsCOMPtr<nsICategoryManager> catMan(do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv));
  if (NS_FAILED(rv))
    return rv;
  for (unsigned i = 0; i < sizeof(gImageMimeTypes)/sizeof(*gImageMimeTypes); i++) {
    catMan->AddCategoryEntry("Gecko-Content-Viewers", gImageMimeTypes[i],
                             "@mozilla.org/content/document-loader-factory;1",
                             PR_TRUE, PR_TRUE, nsnull);
  }

  catMan->AddCategoryEntry("content-sniffing-services", "@mozilla.org/image/loader;1",
                           "@mozilla.org/image/loader;1", PR_TRUE, PR_TRUE,
                           nsnull);
  return NS_OK;
}

static NS_METHOD ImageUnregisterProc(nsIComponentManager *aCompMgr,
                                     nsIFile *aPath,
                                     const char *registryLocation,
                                     const nsModuleComponentInfo *info) {
  nsresult rv;
  nsCOMPtr<nsICategoryManager> catMan(do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv));
  if (NS_FAILED(rv))
    return rv;
  for (unsigned i = 0; i < sizeof(gImageMimeTypes)/sizeof(*gImageMimeTypes); i++)
    catMan->DeleteCategoryEntry("Gecko-Content-Viewers", gImageMimeTypes[i], PR_TRUE);

  return NS_OK;
}

static const nsModuleComponentInfo components[] =
{
  { "image cache",
    NS_IMGLOADER_CID,
    "@mozilla.org/image/cache;1",
    imgLoaderConstructor, },
  { "image container",
    NS_IMGCONTAINER_CID,
    "@mozilla.org/image/container;3",
    imgContainerConstructor, },
  { "image loader",
    NS_IMGLOADER_CID,
    "@mozilla.org/image/loader;1",
    imgLoaderConstructor,
    ImageRegisterProc, 
    ImageUnregisterProc, },
  { "image request proxy",
    NS_IMGREQUESTPROXY_CID,
    "@mozilla.org/image/request;1",
    imgRequestProxyConstructor, },
  { "image tools",
    NS_IMGTOOLS_CID,
    "@mozilla.org/image/tools;1",
    imgToolsConstructor, },

#ifdef IMG_BUILD_DECODER_gif
  
  { "GIF Decoder",
     NS_GIFDECODER2_CID,
     "@mozilla.org/image/decoder;3?type=image/gif",
     nsGIFDecoder2Constructor, },
#endif

#ifdef IMG_BUILD_DECODER_jpeg
  
  { "JPEG decoder",
    NS_JPEGDECODER_CID,
    "@mozilla.org/image/decoder;3?type=image/jpeg",
    nsJPEGDecoderConstructor, },
  { "JPEG decoder",
    NS_JPEGDECODER_CID,
    "@mozilla.org/image/decoder;3?type=image/pjpeg",
    nsJPEGDecoderConstructor, },
  { "JPEG decoder",
    NS_JPEGDECODER_CID,
    "@mozilla.org/image/decoder;3?type=image/jpg",
    nsJPEGDecoderConstructor, },
#endif
#ifdef IMG_BUILD_ENCODER_jpeg
  
  { "JPEG Encoder",
    NS_JPEGENCODER_CID,
    "@mozilla.org/image/encoder;2?type=image/jpeg",
    nsJPEGEncoderConstructor, },
#endif

#ifdef IMG_BUILD_DECODER_bmp
  
  { "ICO Decoder",
     NS_ICODECODER_CID,
     "@mozilla.org/image/decoder;3?type=image/x-icon",
     nsICODecoderConstructor, },
  { "ICO Decoder",
     NS_ICODECODER_CID,
     "@mozilla.org/image/decoder;3?type=image/vnd.microsoft.icon",
     nsICODecoderConstructor, },
  { "BMP Decoder",
     NS_BMPDECODER_CID,
     "@mozilla.org/image/decoder;3?type=image/bmp",
     nsBMPDecoderConstructor, },
  { "BMP Decoder",
     NS_BMPDECODER_CID,
     "@mozilla.org/image/decoder;3?type=image/x-ms-bmp",
     nsBMPDecoderConstructor, },
#endif

#ifdef IMG_BUILD_DECODER_png
  
  { "PNG Decoder",
    NS_PNGDECODER_CID,
    "@mozilla.org/image/decoder;3?type=image/png",
    nsPNGDecoderConstructor, },
  { "PNG Decoder",
    NS_PNGDECODER_CID,
    "@mozilla.org/image/decoder;3?type=image/x-png",
    nsPNGDecoderConstructor, },
#endif
#ifdef IMG_BUILD_ENCODER_png
  
  { "PNG Encoder",
    NS_PNGENCODER_CID,
    "@mozilla.org/image/encoder;2?type=image/png",
    nsPNGEncoderConstructor, },
#endif
};

static nsresult
imglib_Initialize(nsIModule* aSelf)
{
  
  
  
  nsCOMPtr<nsIDeviceContext> devctx = 
    do_CreateInstance("@mozilla.org/gfx/devicecontext;1");

  imgLoader::InitCache();
  return NS_OK;
}

static void
imglib_Shutdown(nsIModule* aSelf)
{
  imgLoader::Shutdown();
}

NS_IMPL_NSGETMODULE_WITH_CTOR_DTOR(nsImageLib2Module, components,
                                   imglib_Initialize, imglib_Shutdown)

