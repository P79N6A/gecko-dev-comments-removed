






































#include "mozilla/ModuleUtils.h"

#include "RasterImage.h"







#undef LoadImage

#include "imgLoader.h"
#include "imgRequest.h"
#include "imgRequestProxy.h"
#include "imgTools.h"
#include "DiscardTracker.h"

#include "nsPNGEncoder.h"
#include "nsJPEGEncoder.h"


namespace mozilla {
namespace imagelib {
NS_GENERIC_FACTORY_CONSTRUCTOR(RasterImage)
}
}
using namespace mozilla::imagelib;

NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(imgLoader, Init)
NS_GENERIC_FACTORY_CONSTRUCTOR(imgRequestProxy)
NS_GENERIC_FACTORY_CONSTRUCTOR(imgTools)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsJPEGEncoder)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsPNGEncoder)
NS_DEFINE_NAMED_CID(NS_IMGLOADER_CID);
NS_DEFINE_NAMED_CID(NS_IMGREQUESTPROXY_CID);
NS_DEFINE_NAMED_CID(NS_IMGTOOLS_CID);
NS_DEFINE_NAMED_CID(NS_RASTERIMAGE_CID);
NS_DEFINE_NAMED_CID(NS_JPEGENCODER_CID);
NS_DEFINE_NAMED_CID(NS_PNGENCODER_CID);

static const mozilla::Module::CIDEntry kImageCIDs[] = {
  { &kNS_IMGLOADER_CID, false, NULL, imgLoaderConstructor, },
  { &kNS_IMGREQUESTPROXY_CID, false, NULL, imgRequestProxyConstructor, },
  { &kNS_IMGTOOLS_CID, false, NULL, imgToolsConstructor, },
  { &kNS_RASTERIMAGE_CID, false, NULL, RasterImageConstructor, },
  { &kNS_JPEGENCODER_CID, false, NULL, nsJPEGEncoderConstructor, },
  { &kNS_PNGENCODER_CID, false, NULL, nsPNGEncoderConstructor, },
  { NULL }
};

static const mozilla::Module::ContractIDEntry kImageContracts[] = {
  { "@mozilla.org/image/cache;1", &kNS_IMGLOADER_CID },
  { "@mozilla.org/image/loader;1", &kNS_IMGLOADER_CID },
  { "@mozilla.org/image/request;1", &kNS_IMGREQUESTPROXY_CID },
  { "@mozilla.org/image/tools;1", &kNS_IMGTOOLS_CID },
  { "@mozilla.org/image/rasterimage;1", &kNS_RASTERIMAGE_CID },
  { "@mozilla.org/image/encoder;2?type=image/jpeg", &kNS_JPEGENCODER_CID },
  { "@mozilla.org/image/encoder;2?type=image/png", &kNS_PNGENCODER_CID },
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
