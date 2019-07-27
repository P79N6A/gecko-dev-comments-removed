





#include "AppleATDecoder.h"
#include "AppleCMLinker.h"
#include "AppleDecoderModule.h"
#include "AppleVDADecoder.h"
#include "AppleVDALinker.h"
#include "AppleVTDecoder.h"
#include "AppleVTLinker.h"
#include "mozilla/Preferences.h"
#include "mozilla/DebugOnly.h"
#include "mozilla/Logging.h"

PRLogModuleInfo* GetAppleMediaLog() {
  static PRLogModuleInfo* log = nullptr;
  if (!log) {
    log = PR_NewLogModule("AppleMedia");
  }
  return log;
}

namespace mozilla {

bool AppleDecoderModule::sInitialized = false;
bool AppleDecoderModule::sIsVTAvailable = false;
bool AppleDecoderModule::sIsVTHWAvailable = false;
bool AppleDecoderModule::sIsVDAAvailable = false;
bool AppleDecoderModule::sForceVDA = false;

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

  if (sInitialized) {
    return;
  }

  Preferences::AddBoolVarCache(&sForceVDA, "media.apple.forcevda", false);

  
  sIsVDAAvailable = AppleVDALinker::Link();

  
  bool haveCoreMedia = AppleCMLinker::Link();
  
  
  
  bool haveVideoToolbox = AppleVTLinker::Link();
  sIsVTAvailable = haveCoreMedia && haveVideoToolbox;

  sIsVTHWAvailable = AppleVTLinker::skPropEnableHWAccel != nullptr;

  sInitialized = true;
}

nsresult
AppleDecoderModule::Startup()
{
  if (!sInitialized || (!sIsVDAAvailable && !sIsVTAvailable)) {
    return NS_ERROR_FAILURE;
  }
  return NS_OK;
}

already_AddRefed<MediaDataDecoder>
AppleDecoderModule::CreateVideoDecoder(const VideoInfo& aConfig,
                                       layers::LayersBackend aLayersBackend,
                                       layers::ImageContainer* aImageContainer,
                                       FlushableTaskQueue* aVideoTaskQueue,
                                       MediaDataDecoderCallback* aCallback)
{
  nsRefPtr<MediaDataDecoder> decoder;

  if (sIsVDAAvailable && (!sIsVTHWAvailable || sForceVDA)) {
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
AppleDecoderModule::CreateAudioDecoder(const AudioInfo& aConfig,
                                       FlushableTaskQueue* aAudioTaskQueue,
                                       MediaDataDecoderCallback* aCallback)
{
  nsRefPtr<MediaDataDecoder> decoder =
    new AppleATDecoder(aConfig, aAudioTaskQueue, aCallback);
  return decoder.forget();
}

bool
AppleDecoderModule::SupportsMimeType(const nsACString& aMimeType)
{
  return aMimeType.EqualsLiteral("audio/mpeg") ||
    PlatformDecoderModule::SupportsMimeType(aMimeType);
}

PlatformDecoderModule::ConversionRequired
AppleDecoderModule::DecoderNeedsConversion(const TrackInfo& aConfig) const
{
  if (aConfig.IsVideo()) {
    return kNeedAVCC;
  } else {
    return kNeedNone;
  }
}

} 
