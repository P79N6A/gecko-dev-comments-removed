






































#include "nsImgBuildDefines.h"

#include "nsIDeviceContext.h"
#include "mozilla/ModuleUtils.h"
#include "nsXPCOMCID.h"
#include "nsServiceManagerUtils.h"

#include "RasterImage.h"







#undef LoadImage

#include "imgLoader.h"
#include "imgRequest.h"
#include "imgRequestProxy.h"
#include "imgTools.h"
#include "DiscardTracker.h"

#ifdef IMG_BUILD_ENCODER_png

#include "nsPNGEncoder.h"
#endif
#ifdef IMG_BUILD_ENCODER_jpeg

#include "nsJPEGEncoder.h"
#endif


namespace mozilla {
namespace imagelib {
NS_GENERIC_FACTORY_CONSTRUCTOR(RasterImage)
}
}
using namespace mozilla::imagelib;

NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(imgLoader, Init)
NS_GENERIC_FACTORY_CONSTRUCTOR(imgRequestProxy)
NS_GENERIC_FACTORY_CONSTRUCTOR(imgTools)

#ifdef IMG_BUILD_ENCODER_jpeg

NS_GENERIC_FACTORY_CONSTRUCTOR(nsJPEGEncoder)
#endif

#ifdef IMG_BUILD_ENCODER_png

NS_GENERIC_FACTORY_CONSTRUCTOR(nsPNGEncoder)
#endif

NS_DEFINE_NAMED_CID(NS_IMGLOADER_CID);
NS_DEFINE_NAMED_CID(NS_IMGREQUESTPROXY_CID);
NS_DEFINE_NAMED_CID(NS_IMGTOOLS_CID);
NS_DEFINE_NAMED_CID(NS_RASTERIMAGE_CID);
#ifdef IMG_BUILD_ENCODER_jpeg
NS_DEFINE_NAMED_CID(NS_JPEGENCODER_CID);
#endif
#ifdef IMG_BUILD_ENCODER_png
NS_DEFINE_NAMED_CID(NS_PNGENCODER_CID);
#endif


static const mozilla::Module::CIDEntry kImageCIDs[] = {
  { &kNS_IMGLOADER_CID, false, NULL, imgLoaderConstructor, },
  { &kNS_IMGREQUESTPROXY_CID, false, NULL, imgRequestProxyConstructor, },
  { &kNS_IMGTOOLS_CID, false, NULL, imgToolsConstructor, },
  { &kNS_RASTERIMAGE_CID, false, NULL, RasterImageConstructor, },
#ifdef IMG_BUILD_ENCODER_jpeg
  { &kNS_JPEGENCODER_CID, false, NULL, nsJPEGEncoderConstructor, },
#endif
#ifdef IMG_BUILD_ENCODER_png
  { &kNS_PNGENCODER_CID, false, NULL, nsPNGEncoderConstructor, },
#endif
  { NULL }
};

static const mozilla::Module::ContractIDEntry kImageContracts[] = {
  { "@mozilla.org/image/cache;1", &kNS_IMGLOADER_CID },
  { "@mozilla.org/image/loader;1", &kNS_IMGLOADER_CID },
  { "@mozilla.org/image/request;1", &kNS_IMGREQUESTPROXY_CID },
  { "@mozilla.org/image/tools;1", &kNS_IMGTOOLS_CID },
  { "@mozilla.org/image/rasterimage;1", &kNS_RASTERIMAGE_CID },
#ifdef IMG_BUILD_ENCODER_jpeg
  { "@mozilla.org/image/encoder;2?type=image/jpeg", &kNS_JPEGENCODER_CID },
#endif
#ifdef IMG_BUILD_ENCODER_png
  { "@mozilla.org/image/encoder;2?type=image/png", &kNS_PNGENCODER_CID },
#endif
  { NULL }
};

static const mozilla::Module::CategoryEntry kImageCategories[] = {
  { "Gecko-Content-Viewers", "image/gif", "@mozilla.org/content/document-loader-factory;1" },
  { "Gecko-Content-Viewers", "image/jpeg", "@mozilla.org/content/document-loader-factory;1" },
  { "Gecko-Content-Viewers", "image/pjpeg", "@mozilla.org/content/document-loader-factory;1" },
  { "Gecko-Content-Viewers", "image/jpg", "@mozilla.org/content/document-loader-factory;1" },
  { "Gecko-Content-Viewers", "image/x-icon", "@mozilla.org/content/document-loader-factory;1" },
  { "Gecko-Content-Viewers", "image/vnd.microsoft.icon", "@mozilla.org/content/document-loader-factory;1" },
  { "Gecko-Content-Viewers", "image/bmp", "@mozilla.org/content/document-loader-factory;1" },
  { "Gecko-Content-Viewers", "image/x-ms-bmp", "@mozilla.org/content/document-loader-factory;1" },
  { "Gecko-Content-Viewers", "image/icon", "@mozilla.org/content/document-loader-factory;1" },
  { "Gecko-Content-Viewers", "image/png", "@mozilla.org/content/document-loader-factory;1" },
  { "Gecko-Content-Viewers", "image/x-png", "@mozilla.org/content/document-loader-factory;1" },
  { "content-sniffing-services", "@mozilla.org/image/loader;1", "@mozilla.org/image/loader;1" },
  { NULL }
};

static nsresult
imglib_Initialize()
{
  
  
  
  nsCOMPtr<nsIDeviceContext> devctx = 
    do_CreateInstance("@mozilla.org/gfx/devicecontext;1");

  imgLoader::InitCache();
  return NS_OK;
}

static void
imglib_Shutdown()
{
  imgLoader::Shutdown();
  mozilla::imagelib::DiscardTracker::Shutdown();
}

static const mozilla::Module kImageModule = {
  mozilla::Module::kVersion,
  kImageCIDs,
  kImageContracts,
  kImageCategories,
  NULL,
  imglib_Initialize,
  imglib_Shutdown
};

NSMODULE_DEFN(nsImageLib2Module) = &kImageModule;
