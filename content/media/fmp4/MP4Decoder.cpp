





#include "MP4Decoder.h"
#include "MP4Reader.h"
#include "MediaDecoderStateMachine.h"
#include "mozilla/Preferences.h"

#ifdef XP_WIN
#include "mozilla/WindowsVersion.h"
#endif
#ifdef MOZ_FFMPEG
#include "FFmpegDecoderModule.h"
#endif

namespace mozilla {

MediaDecoderStateMachine* MP4Decoder::CreateStateMachine()
{
  return new MediaDecoderStateMachine(this, new MP4Reader(this));
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

  
  
  return FFmpegDecoderModule::Link();
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
         
         false;
}


bool
MP4Decoder::IsEnabled()
{
  return HavePlatformMPEGDecoders() &&
         Preferences::GetBool("media.fragmented-mp4.enabled");
}

} 

