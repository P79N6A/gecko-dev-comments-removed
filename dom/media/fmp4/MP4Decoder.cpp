





#include "MP4Decoder.h"
#include "MP4Reader.h"
#include "MediaDecoderStateMachine.h"
#include "MediaFormatReader.h"
#include "MP4Demuxer.h"
#include "mozilla/Preferences.h"
#include "nsCharSeparatedTokenizer.h"
#ifdef MOZ_EME
#include "mozilla/CDMProxy.h"
#endif
#include "mozilla/Logging.h"

#ifdef XP_WIN
#include "mozilla/WindowsVersion.h"
#endif
#ifdef MOZ_WIDGET_ANDROID
#include "nsIGfxInfo.h"
#include "AndroidBridge.h"
#endif
#include "mozilla/layers/LayersTypes.h"

namespace mozilla {

#if defined(MOZ_GONK_MEDIACODEC) || defined(XP_WIN) || defined(MOZ_APPLEMEDIA) || defined(MOZ_FFMPEG)
#define MP4_READER_DORMANT_HEURISTIC
#else
#undef MP4_READER_DORMANT_HEURISTIC
#endif

MP4Decoder::MP4Decoder()
{
#if defined(MP4_READER_DORMANT_HEURISTIC)
  mDormantSupported = Preferences::GetBool("media.decoder.heuristic.dormant.enabled", false);
#endif
}

MediaDecoderStateMachine* MP4Decoder::CreateStateMachine()
{
  bool useFormatDecoder =
    Preferences::GetBool("media.format-reader.mp4", true);
  nsRefPtr<MediaDecoderReader> reader = useFormatDecoder ?
    static_cast<MediaDecoderReader*>(new MediaFormatReader(this, new MP4Demuxer(GetResource()))) :
    static_cast<MediaDecoderReader*>(new MP4Reader(this));

  return new MediaDecoderStateMachine(this, reader);
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
    return MP4Decoder::CanCreateAACDecoder() &&
           (aCodecs.IsEmpty() ||
            IsSupportedAudioCodec(aCodecs,
                                  aOutContainsAAC,
                                  aOutContainsMP3));
  }

  if (!aType.EqualsASCII("video/mp4") ||
      !MP4Decoder::CanCreateH264Decoder()) {
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
  return Preferences::GetBool("media.fragmented-mp4.ffmpeg.enabled", false);
#endif
}

static bool
IsAppleAvailable()
{
#ifndef MOZ_APPLEMEDIA
  
  return false;
#else
  return Preferences::GetBool("media.apple.mp4.enabled", false);
#endif
}

static bool
IsAndroidAvailable()
{
#ifndef MOZ_WIDGET_ANDROID
  return false;
#else
  
  return AndroidBridge::Bridge() && (AndroidBridge::Bridge()->GetAPIVersion() >= 16);
#endif
}

static bool
IsGonkMP4DecoderAvailable()
{
#ifndef MOZ_GONK_MEDIACODEC
  return false;
#else
  return Preferences::GetBool("media.fragmented-mp4.gonk.enabled", false);
#endif
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

static const uint8_t sTestH264ExtraData[] = {
  0x01, 0x64, 0x00, 0x0a, 0xff, 0xe1, 0x00, 0x17, 0x67, 0x64,
  0x00, 0x0a, 0xac, 0xd9, 0x44, 0x26, 0x84, 0x00, 0x00, 0x03,
  0x00, 0x04, 0x00, 0x00, 0x03, 0x00, 0xc8, 0x3c, 0x48, 0x96,
  0x58, 0x01, 0x00, 0x06, 0x68, 0xeb, 0xe3, 0xcb, 0x22, 0xc0
};

static already_AddRefed<MediaDataDecoder>
CreateTestH264Decoder(layers::LayersBackend aBackend,
                      VideoInfo& aConfig)
{
  aConfig.mMimeType = "video/avc";
  aConfig.mId = 1;
  aConfig.mDuration = 40000;
  aConfig.mMediaTime = 0;
  aConfig.mDisplay = aConfig.mImage = nsIntSize(64, 64);
  aConfig.mExtraData = new MediaByteBuffer();
  aConfig.mExtraData->AppendElements(sTestH264ExtraData,
                                     MOZ_ARRAY_LENGTH(sTestH264ExtraData));

  PlatformDecoderModule::Init();

  nsRefPtr<PlatformDecoderModule> platform = PlatformDecoderModule::Create();
  if (!platform) {
    return nullptr;
  }

  nsRefPtr<MediaDataDecoder> decoder(
    platform->CreateDecoder(aConfig, nullptr, nullptr, aBackend, nullptr));
  if (!decoder) {
    return nullptr;
  }
  nsresult rv = decoder->Init();
  NS_ENSURE_SUCCESS(rv, nullptr);

  return decoder.forget();
}

 bool
MP4Decoder::IsVideoAccelerated(layers::LayersBackend aBackend)
{
  VideoInfo config;
  nsRefPtr<MediaDataDecoder> decoder(CreateTestH264Decoder(aBackend, config));
  if (!decoder) {
    return false;
  }
  bool result = decoder->IsHardwareAccelerated();
  decoder->Shutdown();
  return result;
}

 bool
MP4Decoder::CanCreateH264Decoder()
{
#ifdef XP_WIN
  static bool haveCachedResult = false;
  static bool result = false;
  if (haveCachedResult) {
    return result;
  }
  VideoInfo config;
  nsRefPtr<MediaDataDecoder> decoder(
    CreateTestH264Decoder(layers::LayersBackend::LAYERS_BASIC, config));
  if (decoder) {
    decoder->Shutdown();
    result = true;
  }
  haveCachedResult = true;
  return result;
#else
  return IsEnabled();
#endif
}

#ifdef XP_WIN
static already_AddRefed<MediaDataDecoder>
CreateTestAACDecoder(AudioInfo& aConfig)
{
  PlatformDecoderModule::Init();

  nsRefPtr<PlatformDecoderModule> platform = PlatformDecoderModule::Create();
  if (!platform) {
    return nullptr;
  }

  nsRefPtr<MediaDataDecoder> decoder(
    platform->CreateDecoder(aConfig, nullptr, nullptr));
  if (!decoder) {
    return nullptr;
  }
  nsresult rv = decoder->Init();
  NS_ENSURE_SUCCESS(rv, nullptr);

  return decoder.forget();
}


static const uint8_t sTestAACExtraData[] = {
  0x03, 0x80, 0x80, 0x80, 0x22, 0x00, 0x02, 0x00, 0x04, 0x80,
  0x80, 0x80, 0x14, 0x40, 0x15, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x11, 0x51, 0x00, 0x00, 0x11, 0x51, 0x05, 0x80, 0x80, 0x80,
  0x02, 0x13, 0x90, 0x06, 0x80, 0x80, 0x80, 0x01, 0x02
};

static const uint8_t sTestAACConfig[] = { 0x13, 0x90 };

#endif 

 bool
MP4Decoder::CanCreateAACDecoder()
{
#ifdef XP_WIN
  static bool haveCachedResult = false;
  static bool result = false;
  if (haveCachedResult) {
    return result;
  }
  AudioInfo config;
  config.mMimeType = "audio/mp4a-latm";
  config.mRate = 22050;
  config.mChannels = 2;
  config.mBitDepth = 16;
  config.mProfile = 2;
  config.mExtendedProfile = 2;
  config.mCodecSpecificConfig->AppendElements(sTestAACConfig,
                                              MOZ_ARRAY_LENGTH(sTestAACConfig));
  config.mExtraData->AppendElements(sTestAACExtraData,
                                    MOZ_ARRAY_LENGTH(sTestAACExtraData));
  nsRefPtr<MediaDataDecoder> decoder(CreateTestAACDecoder(config));
  if (decoder) {
    decoder->Shutdown();
    result = true;
  }
  haveCachedResult = true;
  return result;
#else
  return IsEnabled();
#endif
}

} 
