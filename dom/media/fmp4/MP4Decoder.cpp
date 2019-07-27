





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
#include "WMFDecoderModule.h"
#endif
#ifdef MOZ_FFMPEG
#include "FFmpegRuntimeLinker.h"
#endif
#ifdef MOZ_APPLEMEDIA
#include "apple/AppleDecoderModule.h"
#endif
#ifdef MOZ_WIDGET_ANDROID
#include "nsIGfxInfo.h"
#include "AndroidBridge.h"
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
  if (aProxy) {
    
    
    
    CDMCaps::AutoLock caps(aProxy->Capabilites());
    nsCOMPtr<nsIRunnable> task(
      NS_NewRunnableMethod(this, &MediaDecoder::NotifyWaitingForResourcesStatusChanged));
    caps.CallOnMainThreadWhenCapsAvailable(task);
  }
  return NS_OK;
}
#endif

static bool
IsSupportedAudioCodec(const nsAString& aCodec,
                      bool& aOutContainsAAC,
                      bool& aOutContainsMP3)
{
  
  aOutContainsAAC = aCodec.EqualsASCII("mp4a.40.2") ||
                    aCodec.EqualsASCII("mp4a.40.5");
  if (aOutContainsAAC) {
#ifdef XP_WIN
    if (!Preferences::GetBool("media.fragmented-mp4.use-blank-decoder") &&
        !WMFDecoderModule::HasAAC()) {
      return false;
    }
#endif
    return true;
  }
#ifndef MOZ_GONK_MEDIACODEC 
  aOutContainsMP3 = aCodec.EqualsASCII("mp3");
  if (aOutContainsMP3) {
    return true;
  }
#else
  aOutContainsMP3 = false;
#endif
  return false;
}

static bool
IsSupportedH264Codec(const nsAString& aCodec)
{
  int16_t profile = 0, level = 0;

  if (!ExtractH264CodecDetails(aCodec, profile, level)) {
    return false;
  }

#ifdef XP_WIN
  if (!Preferences::GetBool("media.fragmented-mp4.use-blank-decoder") &&
      !WMFDecoderModule::HasH264()) {
    return false;
  }

  
  if (!IsWin7OrLater() &&
      level >= H264_LEVEL_5) {
    return false;
  }
#endif

  
  
  
  
  
  
  
  
  return level >= H264_LEVEL_1 &&
         level <= H264_LEVEL_5_1 &&
         (profile == H264_PROFILE_BASE ||
          profile == H264_PROFILE_MAIN ||
          profile == H264_PROFILE_EXTENDED ||
          profile == H264_PROFILE_HIGH);
}


bool
MP4Decoder::CanHandleMediaType(const nsACString& aType,
                               const nsAString& aCodecs,
                               bool& aOutContainsAAC,
                               bool& aOutContainsH264,
                               bool& aOutContainsMP3)
{
  if (!IsEnabled()) {
    return false;
  }

  if (aType.EqualsASCII("audio/mp4") || aType.EqualsASCII("audio/x-m4a")) {
    return aCodecs.IsEmpty() ||
           IsSupportedAudioCodec(aCodecs,
                                 aOutContainsAAC,
                                 aOutContainsMP3);
  }

  if (!aType.EqualsASCII("video/mp4")) {
    return false;
  }

  
  
  nsCharSeparatedTokenizer tokenizer(aCodecs, ',');
  bool expectMoreTokens = false;
  while (tokenizer.hasMoreTokens()) {
    const nsSubstring& token = tokenizer.nextToken();
    expectMoreTokens = tokenizer.separatorAfterCurrentToken();
    if (IsSupportedAudioCodec(token,
                              aOutContainsAAC,
                              aOutContainsMP3)) {
      continue;
    }
    if (IsSupportedH264Codec(token)) {
      aOutContainsH264 = true;
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
  return NS_SUCCEEDED(AppleDecoderModule::CanDecode());
#endif
}

static bool
IsAndroidAvailable()
{
#ifndef MOZ_WIDGET_ANDROID
  return false;
#else
  
  return AndroidBridge::Bridge()->GetAPIVersion() >= 16;
#endif
}

static bool
IsGonkMP4DecoderAvailable()
{
  return Preferences::GetBool("media.fragmented-mp4.gonk.enabled", false);
}

static bool
IsGMPDecoderAvailable()
{
  return Preferences::GetBool("media.fragmented-mp4.gmp.enabled", false);
}

static bool
HavePlatformMPEGDecoders()
{
  return Preferences::GetBool("media.fragmented-mp4.use-blank-decoder") ||
#ifdef XP_WIN
         
         IsVistaOrLater() ||
#endif
         IsAndroidAvailable() ||
         IsFFmpegAvailable() ||
         IsAppleAvailable() ||
         IsGonkMP4DecoderAvailable() ||
         IsGMPDecoderAvailable() ||
         
         false;
}


bool
MP4Decoder::IsEnabled()
{
  return Preferences::GetBool("media.fragmented-mp4.enabled") &&
         HavePlatformMPEGDecoders();
}

} 

