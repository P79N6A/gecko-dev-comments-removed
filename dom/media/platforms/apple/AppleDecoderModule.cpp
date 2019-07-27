





#include "AppleATDecoder.h"
#include "AppleCMLinker.h"
#include "AppleDecoderModule.h"
#include "AppleVDADecoder.h"
#include "AppleVDALinker.h"
#include "AppleVTDecoder.h"
#include "AppleVTLinker.h"
#include "mozilla/Preferences.h"
#include "mozilla/DebugOnly.h"
#include "prlog.h"

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

class LinkTask : public nsRunnable {
public:
  NS_IMETHOD Run() override {
    MOZ_ASSERT(NS_IsMainThread(), "Must be on main thread.");
    MOZ_ASSERT(AppleDecoderModule::sInitialized);
    if (AppleDecoderModule::sIsVDAAvailable) {
      AppleVDALinker::Link();
    }
    if (AppleDecoderModule::sIsVTAvailable) {
      AppleVTLinker::Link();
      AppleCMLinker::Link();
    }
    return NS_OK;
  }
};

class UnlinkTask : public nsRunnable {
public:
  NS_IMETHOD Run() override {
    MOZ_ASSERT(NS_IsMainThread(), "Must be on main thread.");
    MOZ_ASSERT(AppleDecoderModule::sInitialized);
    if (AppleDecoderModule::sIsVDAAvailable) {
      AppleVDALinker::Unlink();
    }
    if (AppleDecoderModule::sIsVTAvailable) {
      AppleVTLinker::Unlink();
      AppleCMLinker::Unlink();
    }
    return NS_OK;
  }
};

AppleDecoderModule::AppleDecoderModule()
{
}

AppleDecoderModule::~AppleDecoderModule()
{
  nsCOMPtr<nsIRunnable> task(new UnlinkTask());
  if (!NS_IsMainThread()) {
    NS_DispatchToMainThread(task);
  } else {
    task->Run();
  }
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

  if (sIsVDAAvailable) {
    AppleVDALinker::Unlink();
  }
  if (sIsVTAvailable) {
    AppleVTLinker::Unlink();
    AppleCMLinker::Unlink();
  }
  sInitialized = true;
}

class InitTask : public nsRunnable {
public:
  NS_IMETHOD Run() override {
    MOZ_ASSERT(NS_IsMainThread(), "Must be on main thread.");
    AppleDecoderModule::Init();
    return NS_OK;
  }
};

nsresult
AppleDecoderModule::Startup()
{
  if (!sIsVDAAvailable && !sIsVTAvailable) {
    return NS_ERROR_FAILURE;
  }

  
  nsCOMPtr<nsIRunnable> task(new LinkTask());
  if (!NS_IsMainThread()) {
    NS_DispatchToMainThread(task, NS_DISPATCH_SYNC);
  } else {
    task->Run();
  }

  return NS_OK;
}

already_AddRefed<MediaDataDecoder>
AppleDecoderModule::CreateVideoDecoder(const VideoInfo& aConfig,
                                       layers::LayersBackend aLayersBackend,
                                       layers::ImageContainer* aImageContainer,
                                       FlushableMediaTaskQueue* aVideoTaskQueue,
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
                                       FlushableMediaTaskQueue* aAudioTaskQueue,
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
