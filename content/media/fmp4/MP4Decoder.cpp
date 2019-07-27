





#include "MP4Decoder.h"
#include "MP4Reader.h"
#include "MediaDecoderStateMachine.h"
#include "mozilla/Preferences.h"
#include "nsCharSeparatedTokenizer.h"
#ifdef MOZ_EME
#include "mozilla/CDMProxy.h"
#endif
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

#ifdef MOZ_EME
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
#endif

static bool
IsSupportedAudioCodec(const nsAString& aCodec)
{
  
  return aCodec.EqualsASCII("mp4a.40.2") ||
#ifndef MOZ_GONK_MEDIACODEC 
         aCodec.EqualsASCII("mp3") ||
#endif
         aCodec.EqualsASCII("mp4a.40.5");
}

static bool
IsSupportedH264Codec(const nsAString& aCodec)
{
  int16_t profile = 0, level = 0;

  if (!ExtractH264CodecDetails(aCodec, profile, level)) {
    return false;
  }

  
  
  
  
  
  
  
  
  return level >= H264_LEVEL_1 &&
         level <= H264_LEVEL_5_1 &&
         (profile == H264_PROFILE_BASE ||
          profile == H264_PROFILE_MAIN ||
          profile == H264_PROFILE_EXTENDED ||
          profile == H264_PROFILE_HIGH);
}


bool
MP4Decoder::CanHandleMediaType(const nsACString& aType,
                               const nsAString& aCodecs)
{
  if (!IsEnabled()) {
    return false;
  }

  if (aType.EqualsASCII("audio/mp4") || aType.EqualsASCII("audio/x-m4a")) {
    return aCodecs.IsEmpty() || IsSupportedAudioCodec(aCodecs);
  }

  if (!aType.EqualsASCII("video/mp4")) {
    return false;
  }

  
  
  nsCharSeparatedTokenizer tokenizer(aCodecs, ',');
  bool expectMoreTokens = false;
  while (tokenizer.hasMoreTokens()) {
    const nsSubstring& token = tokenizer.nextToken();
    expectMoreTokens = tokenizer.separatorAfterCurrentToken();
    if (IsSupportedAudioCodec(token) || IsSupportedH264Codec(token)) {
      continue;
    }
    return false;
  }
  if (expectMoreTokens) {
    
    return false;
  }
  return true;

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
IsGonkMP4DecoderAvailable()
{
  return Preferences::GetBool("media.fragmented-mp4.gonk.enabled", false);
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
         IsGonkMP4DecoderAvailable() ||
         
         false;
}


bool
MP4Decoder::IsEnabled()
{
  return Preferences::GetBool("media.fragmented-mp4.enabled") &&
         HavePlatformMPEGDecoders();
}

} 

