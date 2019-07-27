





#include "AppleATDecoder.h"
#include "AppleCMLinker.h"
#include "AppleDecoderModule.h"
#include "AppleVDADecoder.h"
#include "AppleVDALinker.h"
#include "AppleVTDecoder.h"
#include "AppleVTLinker.h"
#include "mozilla/Preferences.h"
#include "mozilla/DebugOnly.h"

namespace mozilla {

bool AppleDecoderModule::sIsVTAvailable = false;
bool AppleDecoderModule::sIsVDAAvailable = false;

AppleDecoderModule::AppleDecoderModule()
{
}

AppleDecoderModule::~AppleDecoderModule()
{
}


void
AppleDecoderModule::Init()
{
  MOZ_ASSERT(NS_IsMainThread(), "Must be on main thread.");

  if (!Preferences::GetBool("media.apple.mp4.enabled", false)) {
    return;
  }

  
  sIsVDAAvailable = AppleVDALinker::Link();

  
  bool haveCoreMedia = AppleCMLinker::Link();
  
  
  
  sIsVTAvailable = AppleVTLinker::Link() && haveCoreMedia;
}

nsresult
AppleDecoderModule::Startup()
{
  
  
  
  if (!sIsVDAAvailable && !sIsVTAvailable) {
    return NS_ERROR_FAILURE;
  }
  return NS_OK;
}

class UnlinkTask : public nsRunnable {
public:
  NS_IMETHOD Run() MOZ_OVERRIDE {
    MOZ_ASSERT(NS_IsMainThread(), "Must be on main thread.");
    AppleVDALinker::Unlink();
    AppleVTLinker::Unlink();
    AppleCMLinker::Unlink();
    return NS_OK;
  }
};

nsresult
AppleDecoderModule::Shutdown()
{
  nsRefPtr<nsIRunnable> task(new UnlinkTask());
  NS_DispatchToMainThread(task);
  return NS_OK;
}

already_AddRefed<MediaDataDecoder>
AppleDecoderModule::CreateH264Decoder(const mp4_demuxer::VideoDecoderConfig& aConfig,
                                      layers::LayersBackend aLayersBackend,
                                      layers::ImageContainer* aImageContainer,
                                      MediaTaskQueue* aVideoTaskQueue,
                                      MediaDataDecoderCallback* aCallback)
{
  nsRefPtr<MediaDataDecoder> decoder;

  if (sIsVDAAvailable) {
    decoder =
      AppleVDADecoder::CreateVDADecoder(aConfig,
                                        aVideoTaskQueue,
                                        aCallback,
                                        aImageContainer);
    if (decoder) {
      return decoder.forget();
    }
  }
  
  
  if (sIsVTAvailable) {
    decoder =
      new AppleVTDecoder(aConfig, aVideoTaskQueue, aCallback, aImageContainer);
  }
  return decoder.forget();
}

already_AddRefed<MediaDataDecoder>
AppleDecoderModule::CreateAudioDecoder(const mp4_demuxer::AudioDecoderConfig& aConfig,
                                       MediaTaskQueue* aAudioTaskQueue,
                                       MediaDataDecoderCallback* aCallback)
{
  nsRefPtr<MediaDataDecoder> decoder =
    new AppleATDecoder(aConfig, aAudioTaskQueue, aCallback);
  return decoder.forget();
}

bool
AppleDecoderModule::SupportsAudioMimeType(const char* aMimeType)
{
  return !strcmp(aMimeType, "audio/mp4a-latm") || !strcmp(aMimeType, "audio/mpeg");
}

} 
