





#include "PlatformDecoderModule.h"
#include "AVCCDecoderModule.h"

#ifdef XP_WIN
#include "WMFDecoderModule.h"
#endif
#ifdef MOZ_FFMPEG
#include "FFmpegRuntimeLinker.h"
#endif
#ifdef MOZ_APPLEMEDIA
#include "AppleDecoderModule.h"
#endif
#ifdef MOZ_GONK_MEDIACODEC
#include "GonkDecoderModule.h"
#endif
#ifdef MOZ_WIDGET_ANDROID
#include "AndroidDecoderModule.h"
#endif
#include "GMPDecoderModule.h"

#include "mozilla/Preferences.h"
#ifdef MOZ_EME
#include "EMEDecoderModule.h"
#include "mozilla/CDMProxy.h"
#endif
#include "SharedThreadPool.h"
#include "MediaTaskQueue.h"

namespace mozilla {

extern already_AddRefed<PlatformDecoderModule> CreateBlankDecoderModule();

bool PlatformDecoderModule::sUseBlankDecoder = false;
bool PlatformDecoderModule::sFFmpegDecoderEnabled = false;
bool PlatformDecoderModule::sGonkDecoderEnabled = false;
bool PlatformDecoderModule::sAndroidMCDecoderEnabled = false;
bool PlatformDecoderModule::sAndroidMCDecoderPreferred = false;
bool PlatformDecoderModule::sGMPDecoderEnabled = false;


void
PlatformDecoderModule::Init()
{
  MOZ_ASSERT(NS_IsMainThread());
  static bool alreadyInitialized = false;
  if (alreadyInitialized) {
    return;
  }
  alreadyInitialized = true;

  Preferences::AddBoolVarCache(&sUseBlankDecoder,
                               "media.fragmented-mp4.use-blank-decoder");
  Preferences::AddBoolVarCache(&sFFmpegDecoderEnabled,
                               "media.fragmented-mp4.ffmpeg.enabled", false);

#ifdef MOZ_GONK_MEDIACODEC
  Preferences::AddBoolVarCache(&sGonkDecoderEnabled,
                               "media.fragmented-mp4.gonk.enabled", false);
#endif
#ifdef MOZ_WIDGET_ANDROID
  Preferences::AddBoolVarCache(&sAndroidMCDecoderEnabled,
                               "media.fragmented-mp4.android-media-codec.enabled", false);
  Preferences::AddBoolVarCache(&sAndroidMCDecoderPreferred,
                               "media.fragmented-mp4.android-media-codec.preferred", false);
#endif

  Preferences::AddBoolVarCache(&sGMPDecoderEnabled,
                               "media.fragmented-mp4.gmp.enabled", false);

#ifdef XP_WIN
  WMFDecoderModule::Init();
#endif
#ifdef MOZ_APPLEMEDIA
  AppleDecoderModule::Init();
#endif
}

#ifdef MOZ_EME

already_AddRefed<PlatformDecoderModule>
PlatformDecoderModule::CreateCDMWrapper(CDMProxy* aProxy,
                                        bool aHasAudio,
                                        bool aHasVideo)
{
  bool cdmDecodesAudio;
  bool cdmDecodesVideo;
  {
    CDMCaps::AutoLock caps(aProxy->Capabilites());
    cdmDecodesAudio = caps.CanDecryptAndDecodeAudio();
    cdmDecodesVideo = caps.CanDecryptAndDecodeVideo();
  }

  nsRefPtr<PlatformDecoderModule> pdm;
  if ((!cdmDecodesAudio && aHasAudio) || (!cdmDecodesVideo && aHasVideo)) {
    
    
    pdm = Create();
    if (!pdm) {
      return nullptr;
    }
  }

  nsRefPtr<PlatformDecoderModule> emepdm(
    new AVCCDecoderModule(new EMEDecoderModule(aProxy,
                                               pdm,
                                               cdmDecodesAudio,
                                               cdmDecodesVideo)));
  return emepdm.forget();
}
#endif


already_AddRefed<PlatformDecoderModule>
PlatformDecoderModule::Create()
{
  
  MOZ_ASSERT(!NS_IsMainThread());

  nsRefPtr<PlatformDecoderModule> m(CreatePDM());

  if (m && NS_SUCCEEDED(m->Startup())) {
    return m.forget();
  }
  return nullptr;
}


already_AddRefed<PlatformDecoderModule>
PlatformDecoderModule::CreatePDM()
{
#ifdef MOZ_WIDGET_ANDROID
  if(sAndroidMCDecoderPreferred && sAndroidMCDecoderEnabled){
    nsRefPtr<PlatformDecoderModule> m(new AndroidDecoderModule());
    return m.forget();
  }
#endif
  if (sUseBlankDecoder) {
    return CreateBlankDecoderModule();
  }
#ifdef XP_WIN
  nsRefPtr<PlatformDecoderModule> m(new WMFDecoderModule());
  return m.forget();
#endif
#ifdef MOZ_FFMPEG
  if (sFFmpegDecoderEnabled) {
    nsRefPtr<PlatformDecoderModule> m = FFmpegRuntimeLinker::CreateDecoderModule();
    if (m) {
      nsRefPtr<PlatformDecoderModule> m2(new AVCCDecoderModule(m));
      return m2.forget();
    }
  }
#endif
#ifdef MOZ_APPLEMEDIA
  nsRefPtr<PlatformDecoderModule> m(new AVCCDecoderModule(new AppleDecoderModule()));
  return m.forget();
#endif
#ifdef MOZ_GONK_MEDIACODEC
  if (sGonkDecoderEnabled) {
    nsRefPtr<PlatformDecoderModule> m(new GonkDecoderModule());
    return m.forget();
  }
#endif
#ifdef MOZ_WIDGET_ANDROID
  if(sAndroidMCDecoderEnabled){
    nsRefPtr<PlatformDecoderModule> m(new AndroidDecoderModule());
    return m.forget();
  }
#endif
  if (sGMPDecoderEnabled) {
    nsRefPtr<PlatformDecoderModule> m(new AVCCDecoderModule(new GMPDecoderModule()));
    return m.forget();
  }
  return nullptr;
}

bool
PlatformDecoderModule::SupportsAudioMimeType(const char* aMimeType)
{
  return !strcmp(aMimeType, "audio/mp4a-latm");
}

bool
PlatformDecoderModule::SupportsVideoMimeType(const char* aMimeType)
{
  return !strcmp(aMimeType, "video/mp4") || !strcmp(aMimeType, "video/avc");
}

bool
PlatformDecoderModule::DecoderNeedsAVCC(const mp4_demuxer::VideoDecoderConfig& aConfig)
{
  return false;
}

} 
