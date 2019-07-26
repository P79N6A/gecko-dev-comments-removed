





#include "WMF.h"
#include "WMFDecoderModule.h"
#include "WMFDecoder.h"
#include "WMFVideoDecoder.h"
#include "WMFAudioDecoder.h"
#include "mozilla/Preferences.h"
#include "mozilla/DebugOnly.h"
#include "mp4_demuxer/audio_decoder_config.h"

namespace mozilla {

bool WMFDecoderModule::sIsWMFEnabled = false;
bool WMFDecoderModule::sDXVAEnabled = false;

WMFDecoderModule::WMFDecoderModule()
{
}

WMFDecoderModule::~WMFDecoderModule()
{
}


void
WMFDecoderModule::Init()
{
  MOZ_ASSERT(NS_IsMainThread(), "Must be on main thread.");
  sIsWMFEnabled = Preferences::GetBool("media.windows-media-foundation.enabled", false);
  if (!sIsWMFEnabled) {
    return;
  }
  if (NS_FAILED(WMFDecoder::LoadDLLs())) {
    sIsWMFEnabled = false;
  }
  sDXVAEnabled = Preferences::GetBool("media.windows-media-foundation.use-dxva", false);
}

nsresult
WMFDecoderModule::Startup()
{
  if (!sIsWMFEnabled) {
    return NS_ERROR_FAILURE;
  }
  if (FAILED(wmf::MFStartup())) {
    NS_WARNING("Failed to initialize Windows Media Foundation");
    return NS_ERROR_FAILURE;
  }
  return NS_OK;
}

nsresult
WMFDecoderModule::Shutdown()
{
  MOZ_ASSERT(NS_IsMainThread(), "Must be on main thread.");

  DebugOnly<HRESULT> hr = wmf::MFShutdown();
  NS_ASSERTION(SUCCEEDED(hr), "MFShutdown failed");

  return NS_OK;
}

MediaDataDecoder*
WMFDecoderModule::CreateH264Decoder(const mp4_demuxer::VideoDecoderConfig& aConfig,
                                    mozilla::layers::LayersBackend aLayersBackend,
                                    mozilla::layers::ImageContainer* aImageContainer)
{
  return new WMFVideoDecoder(aLayersBackend,
                             aImageContainer,
                             sDXVAEnabled);
}

MediaDataDecoder*
WMFDecoderModule::CreateAACDecoder(const mp4_demuxer::AudioDecoderConfig& aConfig)
{
  return new WMFAudioDecoder(ChannelLayoutToChannelCount(aConfig.channel_layout()),
                             aConfig.samples_per_second(),
                             aConfig.bits_per_channel(),
                             aConfig.extra_data(),
                             aConfig.extra_data_size());
}

void
WMFDecoderModule::OnDecodeThreadStart()
{
  MOZ_ASSERT(!NS_IsMainThread(), "Must not be on main thread.");
  
  
  
  
  
  CoInitializeEx(0, COINIT_MULTITHREADED);
}

void
WMFDecoderModule::OnDecodeThreadFinish()
{
  MOZ_ASSERT(!NS_IsMainThread(), "Must be on main thread.");
  CoUninitialize();
}

} 
