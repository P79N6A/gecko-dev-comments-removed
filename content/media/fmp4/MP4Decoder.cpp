





#include "MP4Decoder.h"
#include "MP4Reader.h"
#include "MediaDecoderStateMachine.h"
#include "mozilla/Preferences.h"
#include "mozilla/CDMProxy.h"
#include "prlog.h"

#ifdef XP_WIN
#include "mozilla/WindowsVersion.h"
#endif
#ifdef MOZ_FFMPEG
#include "FFmpegRuntimeLinker.h"
#endif
#ifdef MOZ_APPLEMEDIA
#include "apple/AppleCMLinker.h"
#include "apple/AppleVTLinker.h"
#endif

namespace mozilla {

MediaDecoderStateMachine* MP4Decoder::CreateStateMachine()
{
  return new MediaDecoderStateMachine(this, new MP4Reader(this));
}

nsresult
MP4Decoder::SetCDMProxy(CDMProxy* aProxy)
{
  nsresult rv = MediaDecoder::SetCDMProxy(aProxy);
  NS_ENSURE_SUCCESS(rv, rv);
  {
    
    
    
    CDMCaps::AutoLock caps(aProxy->Capabilites());
    nsRefPtr<nsIRunnable> task(
      NS_NewRunnableMethod(this, &MediaDecoder::NotifyWaitingForResourcesStatusChanged));
    caps.CallOnMainThreadWhenCapsAvailable(task);
  }
  return NS_OK;
}

bool
MP4Decoder::GetSupportedCodecs(const nsACString& aType,
                               char const *const ** aCodecList)
{
  if (!IsEnabled()) {
    return false;
  }

  
  static char const *const aacAudioCodecs[] = {
    "mp4a.40.2",    
    
    nullptr
  };
  if (aType.EqualsASCII("audio/mp4") ||
      aType.EqualsASCII("audio/x-m4a")) {
    if (aCodecList) {
      *aCodecList = aacAudioCodecs;
    }
    return true;
  }

  
  static char const *const h264Codecs[] = {
    "avc1.42E01E",  
    "avc1.42001E",  
    "avc1.58A01E",  
    "avc1.4D401E",  
    "avc1.64001E",  
    "avc1.64001F",  
    "mp4a.40.2",    
    
    nullptr
  };
  if (aType.EqualsASCII("video/mp4")) {
    if (aCodecList) {
      *aCodecList = h264Codecs;
    }
    return true;
  }

  return false;
}

static bool
IsFFmpegAvailable()
{
#ifndef MOZ_FFMPEG
  return false;
#else
  if (!Preferences::GetBool("media.fragmented-mp4.ffmpeg.enabled", false)) {
    return false;
  }

  
  
  return FFmpegRuntimeLinker::Link();
#endif
}

static bool
IsAppleAvailable()
{
#ifndef MOZ_APPLEMEDIA
  
  return false;
#else
  if (!Preferences::GetBool("media.apple.mp4.enabled", false)) {
    
    return false;
  }
  
  bool haveCoreMedia = AppleCMLinker::Link();
  if (!haveCoreMedia) {
    return false;
  }
  bool haveVideoToolbox = AppleVTLinker::Link();
  if (!haveVideoToolbox) {
    return false;
  }
  
  return true;
#endif
}

static bool
HavePlatformMPEGDecoders()
{
  return Preferences::GetBool("media.fragmented-mp4.use-blank-decoder") ||
#ifdef XP_WIN
         
         IsVistaOrLater() ||
#endif
         IsFFmpegAvailable() ||
         IsAppleAvailable() ||
         
         false;
}


bool
MP4Decoder::IsEnabled()
{
  return HavePlatformMPEGDecoders() &&
         Preferences::GetBool("media.fragmented-mp4.enabled");
}

} 

