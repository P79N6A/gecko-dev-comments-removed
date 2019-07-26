





#include "wmf.h"
#include "WMFDecoderModule.h"
#include "WMFDecoder.h"
#include "WMFVideoDecoder.h"
#include "WMFAudioDecoder.h"
#include "mozilla/Preferences.h"
#include "mozilla/DebugOnly.h"

namespace mozilla {

WMFDecoderModule::WMFDecoderModule()
  : mDXVAEnabled(Preferences::GetBool("media.windows-media-foundation.use-dxva", false))
{
  MOZ_ASSERT(NS_IsMainThread(), "Must be on main thread.");
}

WMFDecoderModule::~WMFDecoderModule()
{
  MOZ_ASSERT(NS_IsMainThread(), "Must be on main thread.");
}

nsresult
WMFDecoderModule::Init()
{
  MOZ_ASSERT(NS_IsMainThread(), "Must be on main thread.");
  if (!Preferences::GetBool("media.windows-media-foundation.enabled", false)) {
    return NS_ERROR_FAILURE;
  }

  nsresult rv = WMFDecoder::LoadDLLs();
  NS_ENSURE_SUCCESS(rv, rv);

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
WMFDecoderModule::CreateVideoDecoder(mozilla::layers::LayersBackend aLayersBackend,
                                     mozilla::layers::ImageContainer* aImageContainer)
{
  nsAutoPtr<WMFVideoDecoder> decoder(new WMFVideoDecoder(mDXVAEnabled));
  nsresult rv = decoder->Init(aLayersBackend, aImageContainer);
  NS_ENSURE_SUCCESS(rv, nullptr);
  return decoder.forget();
}

MediaDataDecoder*
WMFDecoderModule::CreateAudioDecoder(uint32_t aChannelCount,
                                     uint32_t aSampleRate,
                                     uint16_t aBitsPerSample,
                                     const uint8_t* aUserData,
                                     uint32_t aUserDataLength)
{
  nsAutoPtr<WMFAudioDecoder> decoder(new WMFAudioDecoder());
  nsresult rv = decoder->Init(aChannelCount,
                              aSampleRate,
                              aBitsPerSample,
                              aUserData,
                              aUserDataLength);
  NS_ENSURE_SUCCESS(rv, nullptr);
  return decoder.forget();
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
