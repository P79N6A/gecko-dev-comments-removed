





#include "WMF.h"
#include "WMFDecoderModule.h"
#include "WMFVideoMFTManager.h"
#include "WMFAudioMFTManager.h"
#include "mozilla/Preferences.h"
#include "mozilla/DebugOnly.h"
#include "mozilla/Services.h"
#include "WMFMediaDataDecoder.h"
#include "nsIWindowsRegKey.h"
#include "nsComponentManagerUtils.h"
#include "nsServiceManagerUtils.h"
#include "nsIGfxInfo.h"
#include "GfxDriverInfo.h"
#include "gfxWindowsPlatform.h"
#include "MediaInfo.h"
#include "prsystem.h"

namespace mozilla {

static bool sIsWMFEnabled = false;
static bool sDXVAEnabled = false;
static int  sNumDecoderThreads = -1;

WMFDecoderModule::WMFDecoderModule()
  : mWMFInitialized(false)
{
}

WMFDecoderModule::~WMFDecoderModule()
{
  if (mWMFInitialized) {
    DebugOnly<HRESULT> hr = wmf::MFShutdown();
    NS_ASSERTION(SUCCEEDED(hr), "MFShutdown failed");
  }
}

void
WMFDecoderModule::DisableHardwareAcceleration()
{
  sDXVAEnabled = false;
}

static void
SetNumOfDecoderThreads()
{
  MOZ_ASSERT(NS_IsMainThread(), "Preferences can only be read on main thread");
  int32_t numCores = PR_GetNumberOfProcessors();

  
  
  const int WMF_DECODER_DEFAULT = -1;
  int32_t prefThreadCount = Preferences::GetInt("media.wmf.decoder.thread-count", -1);
  if (prefThreadCount != WMF_DECODER_DEFAULT) {
    sNumDecoderThreads = std::max(prefThreadCount, 1);
  } else if (numCores > 4) {
    sNumDecoderThreads = WMF_DECODER_DEFAULT;
  } else {
    sNumDecoderThreads = std::max(numCores - 1, 1);
  }
}


void
WMFDecoderModule::Init()
{
  MOZ_ASSERT(NS_IsMainThread(), "Must be on main thread.");
  sIsWMFEnabled = Preferences::GetBool("media.windows-media-foundation.enabled", false);
  sDXVAEnabled = gfxPlatform::GetPlatform()->CanUseHardwareVideoDecoding();
  SetNumOfDecoderThreads();
}


int
WMFDecoderModule::GetNumDecoderThreads()
{
  return sNumDecoderThreads;
}

nsresult
WMFDecoderModule::Startup()
{
  if (sIsWMFEnabled) {
    mWMFInitialized = SUCCEEDED(wmf::MFStartup());
  }
  return mWMFInitialized ? NS_OK : NS_ERROR_FAILURE;
}

already_AddRefed<MediaDataDecoder>
WMFDecoderModule::CreateVideoDecoder(const VideoInfo& aConfig,
                                     layers::LayersBackend aLayersBackend,
                                     layers::ImageContainer* aImageContainer,
                                     FlushableTaskQueue* aVideoTaskQueue,
                                     MediaDataDecoderCallback* aCallback)
{
  nsRefPtr<MediaDataDecoder> decoder =
    new WMFMediaDataDecoder(new WMFVideoMFTManager(aConfig,
                                                   aLayersBackend,
                                                   aImageContainer,
                                                   sDXVAEnabled && ShouldUseDXVA(aConfig)),
                            aVideoTaskQueue,
                            aCallback);
  return decoder.forget();
}

already_AddRefed<MediaDataDecoder>
WMFDecoderModule::CreateAudioDecoder(const AudioInfo& aConfig,
                                     FlushableTaskQueue* aAudioTaskQueue,
                                     MediaDataDecoderCallback* aCallback)
{
  nsRefPtr<MediaDataDecoder> decoder =
    new WMFMediaDataDecoder(new WMFAudioMFTManager(aConfig),
                            aAudioTaskQueue,
                            aCallback);
  return decoder.forget();
}

bool
WMFDecoderModule::ShouldUseDXVA(const VideoInfo& aConfig) const
{
  static bool isAMD = false;
  static bool initialized = false;
  if (!initialized) {
    nsCOMPtr<nsIGfxInfo> gfxInfo = services::GetGfxInfo();
    nsAutoString vendor;
    gfxInfo->GetAdapterVendorID(vendor);
    isAMD = vendor.Equals(widget::GfxDriverInfo::GetDeviceVendor(widget::VendorAMD), nsCaseInsensitiveStringComparator()) ||
            vendor.Equals(widget::GfxDriverInfo::GetDeviceVendor(widget::VendorATI), nsCaseInsensitiveStringComparator());
    initialized = true;
  }
  if (!isAMD) {
    return true;
  }
  
  return aConfig.mDisplay.width <= 1920 && aConfig.mDisplay.height <= 1200;
}

bool
WMFDecoderModule::SupportsSharedDecoders(const VideoInfo& aConfig) const
{
  
  
  return !sDXVAEnabled || ShouldUseDXVA(aConfig);
}

bool
WMFDecoderModule::SupportsMimeType(const nsACString& aMimeType)
{
  return aMimeType.EqualsLiteral("video/mp4") ||
         aMimeType.EqualsLiteral("video/avc") ||
         aMimeType.EqualsLiteral("video/webm; codecs=vp8") ||
         aMimeType.EqualsLiteral("video/webm; codecs=vp9") ||
         aMimeType.EqualsLiteral("audio/mp4a-latm") ||
         aMimeType.EqualsLiteral("audio/mpeg");
}

PlatformDecoderModule::ConversionRequired
WMFDecoderModule::DecoderNeedsConversion(const TrackInfo& aConfig) const
{
  if (aConfig.IsVideo() &&
      (aConfig.mMimeType.EqualsLiteral("video/avc") ||
       aConfig.mMimeType.EqualsLiteral("video/mp4"))) {
    return kNeedAnnexB;
  } else {
    return kNeedNone;
  }
}

static bool
ClassesRootRegKeyExists(const nsAString& aRegKeyPath)
{
  nsresult rv;

  nsCOMPtr<nsIWindowsRegKey> regKey =
    do_CreateInstance("@mozilla.org/windows-registry-key;1", &rv);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return false;
  }

  rv = regKey->Open(nsIWindowsRegKey::ROOT_KEY_CLASSES_ROOT,
                    aRegKeyPath,
                    nsIWindowsRegKey::ACCESS_READ);
  if (NS_FAILED(rv)) {
    return false;
  }

  regKey->Close();

  return true;
}

 bool
WMFDecoderModule::HasH264()
{
  
  return ClassesRootRegKeyExists(
    NS_LITERAL_STRING("CLSID\\{32D186A7-218F-4C75-8876-DD77273A8999}"));
}

 bool
WMFDecoderModule::HasAAC()
{
  
  return ClassesRootRegKeyExists(
    NS_LITERAL_STRING("CLSID\\{62CE7E72-4C71-4D20-B15D-452831A87D9D}"));
}

} 
