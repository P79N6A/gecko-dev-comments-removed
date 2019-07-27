





#include "PlatformDecoderModule.h"
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

#include "mozilla/Preferences.h"
#ifdef MOZ_EME
#include "EMEDecoderModule.h"
#include "mozilla/CDMProxy.h"
#endif
#include "SharedThreadPool.h"
#include "MediaTaskQueue.h"

namespace mozilla {

extern PlatformDecoderModule* CreateBlankDecoderModule();

bool PlatformDecoderModule::sUseBlankDecoder = false;
bool PlatformDecoderModule::sFFmpegDecoderEnabled = false;
bool PlatformDecoderModule::sGonkDecoderEnabled = false;
bool PlatformDecoderModule::sAndroidMCDecoderEnabled = false;
bool PlatformDecoderModule::sAndroidMCDecoderPreferred = false;


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

#ifdef XP_WIN
  WMFDecoderModule::Init();
#endif
#ifdef MOZ_APPLEMEDIA
  AppleDecoderModule::Init();
#endif
}

#ifdef MOZ_EME

PlatformDecoderModule*
PlatformDecoderModule::CreateCDMWrapper(CDMProxy* aProxy,
                                        bool aHasAudio,
                                        bool aHasVideo,
                                        MediaTaskQueue* aTaskQueue)
{
  bool cdmDecodesAudio;
  bool cdmDecodesVideo;
  {
    CDMCaps::AutoLock caps(aProxy->Capabilites());
    cdmDecodesAudio = caps.CanDecryptAndDecodeAudio();
    cdmDecodesVideo = caps.CanDecryptAndDecodeVideo();
  }

  nsAutoPtr<PlatformDecoderModule> pdm;
  if ((!cdmDecodesAudio && aHasAudio) || (!cdmDecodesVideo && aHasVideo)) {
    
    
    pdm = Create();
    if (!pdm) {
      return nullptr;
    }
  }

  return new EMEDecoderModule(aProxy,
                              pdm.forget(),
                              cdmDecodesAudio,
                              cdmDecodesVideo);
}
#endif


PlatformDecoderModule*
PlatformDecoderModule::Create()
{
  
  MOZ_ASSERT(!NS_IsMainThread());

#ifdef MOZ_WIDGET_ANDROID
  if(sAndroidMCDecoderPreferred && sAndroidMCDecoderEnabled){
    return new AndroidDecoderModule();
  }
#endif
  if (sUseBlankDecoder) {
    return CreateBlankDecoderModule();
  }
#ifdef XP_WIN
  nsAutoPtr<WMFDecoderModule> m(new WMFDecoderModule());
  if (NS_SUCCEEDED(m->Startup())) {
    return m.forget();
  }
#endif
#ifdef MOZ_FFMPEG
  if (sFFmpegDecoderEnabled) {
    nsAutoPtr<PlatformDecoderModule> m(FFmpegRuntimeLinker::CreateDecoderModule());
    if (m) {
      return m.forget();
    }
  }
#endif
#ifdef MOZ_APPLEMEDIA
  nsAutoPtr<AppleDecoderModule> m(new AppleDecoderModule());
  if (NS_SUCCEEDED(m->Startup())) {
    return m.forget();
  }
#endif
#ifdef MOZ_GONK_MEDIACODEC
  if (sGonkDecoderEnabled) {
    return new GonkDecoderModule();
  }
#endif
#ifdef MOZ_WIDGET_ANDROID
  if(sAndroidMCDecoderEnabled){
    return new AndroidDecoderModule();
  }
#endif
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

} 
