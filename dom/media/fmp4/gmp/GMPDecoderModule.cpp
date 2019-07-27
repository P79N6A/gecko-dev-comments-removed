





#include "GMPDecoderModule.h"
#include "GMPAudioDecoder.h"
#include "GMPVideoDecoder.h"
#include "MediaDataDecoderProxy.h"
#include "mozIGeckoMediaPluginService.h"
#include "nsServiceManagerUtils.h"

namespace mozilla {

GMPDecoderModule::GMPDecoderModule()
{
}

GMPDecoderModule::~GMPDecoderModule()
{
}

static already_AddRefed<MediaDataDecoderProxy>
CreateDecoderWrapper(MediaDataDecoderCallback* aCallback)
{
  nsCOMPtr<mozIGeckoMediaPluginService> gmpService = do_GetService("@mozilla.org/gecko-media-plugin-service;1");
  if (!gmpService) {
    return nullptr;
  }

  nsCOMPtr<nsIThread> thread;
  nsresult rv = gmpService->GetThread(getter_AddRefs(thread));
  if (NS_FAILED(rv)) {
    return nullptr;
  }

  nsRefPtr<MediaDataDecoderProxy> decoder(new MediaDataDecoderProxy(thread, aCallback));
  return decoder.forget();
}

already_AddRefed<MediaDataDecoder>
GMPDecoderModule::CreateVideoDecoder(const mp4_demuxer::VideoDecoderConfig& aConfig,
                                     layers::LayersBackend aLayersBackend,
                                     layers::ImageContainer* aImageContainer,
                                     FlushableMediaTaskQueue* aVideoTaskQueue,
                                     MediaDataDecoderCallback* aCallback)
{
  if (!aConfig.mime_type.EqualsLiteral("video/avc")) {
    return nullptr;
  }

  nsRefPtr<MediaDataDecoderProxy> wrapper = CreateDecoderWrapper(aCallback);
  wrapper->SetProxyTarget(new GMPVideoDecoder(aConfig,
                                              aLayersBackend,
                                              aImageContainer,
                                              aVideoTaskQueue,
                                              wrapper->Callback()));
  return wrapper.forget();
}

already_AddRefed<MediaDataDecoder>
GMPDecoderModule::CreateAudioDecoder(const mp4_demuxer::AudioDecoderConfig& aConfig,
                                     FlushableMediaTaskQueue* aAudioTaskQueue,
                                     MediaDataDecoderCallback* aCallback)
{
  if (!aConfig.mime_type.EqualsLiteral("audio/mp4a-latm")) {
    return nullptr;
  }

  nsRefPtr<MediaDataDecoderProxy> wrapper = CreateDecoderWrapper(aCallback);
  wrapper->SetProxyTarget(new GMPAudioDecoder(aConfig,
                                              aAudioTaskQueue,
                                              wrapper->Callback()));
  return wrapper.forget();
}

PlatformDecoderModule::ConversionRequired
GMPDecoderModule::DecoderNeedsConversion(const mp4_demuxer::TrackConfig& aConfig) const
{
  
  if (aConfig.IsVideoConfig()) {
    return kNeedAVCC;
  } else {
    return kNeedNone;
  }
}

} 
