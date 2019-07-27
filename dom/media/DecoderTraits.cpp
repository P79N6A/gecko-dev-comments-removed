





#include "DecoderTraits.h"
#include "MediaDecoder.h"
#include "nsCharSeparatedTokenizer.h"
#include "nsMimeTypes.h"
#include "mozilla/Preferences.h"

#ifdef MOZ_ANDROID_OMX
#include "AndroidMediaPluginHost.h"
#endif

#include "OggDecoder.h"
#include "OggReader.h"
#ifdef MOZ_WAVE
#include "WaveDecoder.h"
#include "WaveReader.h"
#endif
#ifdef MOZ_WEBM
#include "WebMDecoder.h"
#include "WebMReader.h"
#endif
#ifdef MOZ_RAW
#include "RawDecoder.h"
#include "RawReader.h"
#endif
#ifdef MOZ_GSTREAMER
#include "GStreamerDecoder.h"
#include "GStreamerReader.h"
#endif
#ifdef MOZ_ANDROID_OMX
#include "AndroidMediaPluginHost.h"
#include "AndroidMediaDecoder.h"
#include "AndroidMediaReader.h"
#include "AndroidMediaPluginHost.h"
#endif
#ifdef MOZ_OMX_DECODER
#include "MediaOmxDecoder.h"
#include "MediaOmxReader.h"
#include "nsIPrincipal.h"
#include "mozilla/dom/HTMLMediaElement.h"
#if ANDROID_VERSION >= 18
#include "MediaCodecDecoder.h"
#include "MediaCodecReader.h"
#endif
#endif
#ifdef NECKO_PROTOCOL_rtsp
#if ANDROID_VERSION >= 18
#include "RtspMediaCodecDecoder.h"
#include "RtspMediaCodecReader.h"
#endif
#include "RtspOmxDecoder.h"
#include "RtspOmxReader.h"
#endif
#ifdef MOZ_WMF
#include "WMFDecoder.h"
#include "WMFReader.h"
#endif
#ifdef MOZ_DIRECTSHOW
#include "DirectShowDecoder.h"
#include "DirectShowReader.h"
#endif
#ifdef MOZ_APPLEMEDIA
#include "AppleDecoder.h"
#include "AppleMP3Reader.h"
#endif
#ifdef MOZ_FMP4
#include "MP4Reader.h"
#include "MP4Decoder.h"
#endif

namespace mozilla
{

template <class String>
static bool
CodecListContains(char const *const * aCodecs, const String& aCodec)
{
  for (int32_t i = 0; aCodecs[i]; ++i) {
    if (aCodec.EqualsASCII(aCodecs[i]))
      return true;
  }
  return false;
}

#ifdef MOZ_RAW
static const char* gRawTypes[3] = {
  "video/x-raw",
  "video/x-raw-yuv",
  nullptr
};

static const char* gRawCodecs[1] = {
  nullptr
};

static bool
IsRawType(const nsACString& aType)
{
  if (!MediaDecoder::IsRawEnabled()) {
    return false;
  }

  return CodecListContains(gRawTypes, aType);
}
#endif



static const char* const gOggTypes[4] = {
  "video/ogg",
  "audio/ogg",
  "application/ogg",
  nullptr
};

static char const *const gOggCodecs[3] = {
  "vorbis",
  "theora",
  nullptr
};

static char const *const gOggCodecsWithOpus[4] = {
  "vorbis",
  "opus",
  "theora",
  nullptr
};

static bool
IsOggType(const nsACString& aType)
{
  if (!MediaDecoder::IsOggEnabled()) {
    return false;
  }

  return CodecListContains(gOggTypes, aType);
}

#ifdef MOZ_WAVE



static const char* const gWaveTypes[5] = {
  "audio/x-wav",
  "audio/wav",
  "audio/wave",
  "audio/x-pn-wav",
  nullptr
};

static char const *const gWaveCodecs[2] = {
  "1", 
  nullptr
};

static bool
IsWaveType(const nsACString& aType)
{
  if (!MediaDecoder::IsWaveEnabled()) {
    return false;
  }

  return CodecListContains(gWaveTypes, aType);
}
#endif

#ifdef MOZ_WEBM
static const char* const gWebMTypes[3] = {
  "video/webm",
  "audio/webm",
  nullptr
};

static char const *const gWebMCodecs[7] = {
  "vp8",
  "vp8.0",
  "vp9",
  "vp9.0",
  "vorbis",
  "opus",
  nullptr
};

static bool
IsWebMType(const nsACString& aType)
{
  if (!MediaDecoder::IsWebMEnabled()) {
    return false;
  }

  return CodecListContains(gWebMTypes, aType);
}
#endif

#ifdef MOZ_GSTREAMER
static bool
IsGStreamerSupportedType(const nsACString& aMimeType)
{
  if (!MediaDecoder::IsGStreamerEnabled())
    return false;

#ifdef MOZ_WEBM
  if (IsWebMType(aMimeType) && !Preferences::GetBool("media.prefer-gstreamer", false))
    return false;
#endif
  if (IsOggType(aMimeType) && !Preferences::GetBool("media.prefer-gstreamer", false))
    return false;

  return GStreamerDecoder::CanHandleMediaType(aMimeType, nullptr);
}
#endif

#ifdef MOZ_OMX_DECODER
static const char* const gOmxTypes[] = {
  "audio/mpeg",
  "audio/mp4",
  "audio/amr",
  "audio/3gpp",
  "audio/flac",
  "video/mp4",
  "video/3gpp",
  "video/3gpp2",
  "video/quicktime",
#ifdef MOZ_OMX_WEBM_DECODER
  "video/webm",
  "audio/webm",
#endif
  "audio/x-matroska",
  "video/mp2t",
  "video/avi",
  "video/x-matroska",
  nullptr
};

static const char* const gB2GOnlyTypes[] = {
  "audio/3gpp",
  "audio/amr",
  "audio/x-matroska",
  "video/mp2t",
  "video/avi",
  "video/x-matroska",
  nullptr
};

static bool
IsOmxSupportedType(const nsACString& aType)
{
  if (!MediaDecoder::IsOmxEnabled()) {
    return false;
  }

  return CodecListContains(gOmxTypes, aType);
}

static bool
IsB2GSupportOnlyType(const nsACString& aType)
{
  return CodecListContains(gB2GOnlyTypes, aType);
}

static char const *const gH264Codecs[9] = {
  "avc1.42E01E",  
  "avc1.42001E",  
  "avc1.58A01E",  
  "avc1.4D401E",  
  "avc1.64001E",  
  "avc1.64001F",  
  "mp4v.20.3",    
  "mp4a.40.2",    
  nullptr
};

static char const *const gMpegAudioCodecs[2] = {
  "mp3",          
  nullptr
};

#ifdef MOZ_OMX_WEBM_DECODER
static char const *const gOMXWebMCodecs[4] = {
  "vorbis",
  "vp8",
  "vp8.0",
  nullptr
};
#endif 

#endif

#ifdef NECKO_PROTOCOL_rtsp
static const char* const gRtspTypes[2] = {
    "RTSP",
    nullptr
};

static bool
IsRtspSupportedType(const nsACString& aMimeType)
{
  return MediaDecoder::IsRtspEnabled() &&
    CodecListContains(gRtspTypes, aMimeType);
}
#endif


bool DecoderTraits::DecoderWaitsForOnConnected(const nsACString& aMimeType) {
#ifdef NECKO_PROTOCOL_rtsp
  return CodecListContains(gRtspTypes, aMimeType);
#else
  return false;
#endif
}

#ifdef MOZ_ANDROID_OMX
static bool
IsAndroidMediaType(const nsACString& aType)
{
  if (!MediaDecoder::IsAndroidMediaEnabled()) {
    return false;
  }

  static const char* supportedTypes[] = {
    "audio/mpeg", "audio/mp4", "video/mp4", nullptr
  };
  return CodecListContains(supportedTypes, aType);
}
#endif

#ifdef MOZ_WMF
static bool
IsWMFSupportedType(const nsACString& aType)
{
  return WMFDecoder::CanPlayType(aType, NS_LITERAL_STRING(""));
}
#endif

#ifdef MOZ_DIRECTSHOW
static bool
IsDirectShowSupportedType(const nsACString& aType)
{
  return DirectShowDecoder::GetSupportedCodecs(aType, nullptr);
}
#endif

#ifdef MOZ_FMP4
static bool
IsMP4SupportedType(const nsACString& aType,
                   const nsAString& aCodecs = EmptyString())
{
  
  bool haveAAC, haveMP3, haveH264;
  return Preferences::GetBool("media.fragmented-mp4.exposed", false) &&
         MP4Decoder::CanHandleMediaType(aType, aCodecs, haveAAC, haveH264, haveMP3);
}
#endif

#ifdef MOZ_APPLEMEDIA
static const char * const gAppleMP3Types[] = {
  "audio/mp3",
  "audio/mpeg",
  nullptr,
};

static const char * const gAppleMP3Codecs[] = {
  "mp3",
  nullptr
};

static bool
IsAppleMediaSupportedType(const nsACString& aType,
                     const char * const ** aCodecs = nullptr)
{
  if (MediaDecoder::IsAppleMP3Enabled()
      && CodecListContains(gAppleMP3Types, aType)) {

    if (aCodecs) {
      *aCodecs = gAppleMP3Codecs;
    }

    return true;
  }

  

  return false;
}
#endif


bool DecoderTraits::ShouldHandleMediaType(const char* aMIMEType)
{
#ifdef MOZ_WAVE
  if (IsWaveType(nsDependentCString(aMIMEType))) {
    
    
    
    
    
    return false;
  }
#endif
  return CanHandleMediaType(aMIMEType, false, EmptyString()) != CANPLAY_NO;
}


CanPlayStatus
DecoderTraits::CanHandleMediaType(const char* aMIMEType,
                                  bool aHaveRequestedCodecs,
                                  const nsAString& aRequestedCodecs)
{
  MOZ_ASSERT(NS_IsMainThread());
  char const* const* codecList = nullptr;
  CanPlayStatus result = CANPLAY_NO;
#ifdef MOZ_RAW
  if (IsRawType(nsDependentCString(aMIMEType))) {
    codecList = gRawCodecs;
    result = CANPLAY_MAYBE;
  }
#endif
  if (IsOggType(nsDependentCString(aMIMEType))) {
    codecList = MediaDecoder::IsOpusEnabled() ? gOggCodecsWithOpus : gOggCodecs;
    result = CANPLAY_MAYBE;
  }
#ifdef MOZ_WAVE
  if (IsWaveType(nsDependentCString(aMIMEType))) {
    codecList = gWaveCodecs;
    result = CANPLAY_MAYBE;
  }
#endif
#if defined(MOZ_WEBM) && !defined(MOZ_OMX_WEBM_DECODER)
  if (IsWebMType(nsDependentCString(aMIMEType))) {
    codecList = gWebMCodecs;
    result = CANPLAY_MAYBE;
  }
#endif
#ifdef MOZ_FMP4
  if (IsMP4SupportedType(nsDependentCString(aMIMEType),
                                     aRequestedCodecs)) {
    return aHaveRequestedCodecs ? CANPLAY_YES : CANPLAY_MAYBE;
  }
#endif
#ifdef MOZ_GSTREAMER
  if (GStreamerDecoder::CanHandleMediaType(nsDependentCString(aMIMEType),
                                           aHaveRequestedCodecs ? &aRequestedCodecs : nullptr)) {
    if (aHaveRequestedCodecs)
      return CANPLAY_YES;
    return CANPLAY_MAYBE;
  }
#endif
#ifdef MOZ_OMX_DECODER
  if (IsOmxSupportedType(nsDependentCString(aMIMEType))) {
    result = CANPLAY_MAYBE;
    if (nsDependentCString(aMIMEType).EqualsASCII("audio/mpeg")) {
      codecList = gMpegAudioCodecs;
#ifdef MOZ_OMX_WEBM_DECODER
    } else if (nsDependentCString(aMIMEType).EqualsASCII("audio/webm") ||
               nsDependentCString(aMIMEType).EqualsASCII("video/webm")) {
      codecList = gOMXWebMCodecs;
#endif
    } else {
      codecList = gH264Codecs;
    }
  }
#endif
#ifdef MOZ_DIRECTSHOW
  
  
  if (DirectShowDecoder::GetSupportedCodecs(nsDependentCString(aMIMEType), &codecList)) {
    result = CANPLAY_MAYBE;
  }
#endif
#ifdef MOZ_WMF
  if (!Preferences::GetBool("media.fragmented-mp4.exposed", false) &&
      IsWMFSupportedType(nsDependentCString(aMIMEType))) {
    if (!aHaveRequestedCodecs) {
      return CANPLAY_MAYBE;
    }
    return WMFDecoder::CanPlayType(nsDependentCString(aMIMEType),
                                   aRequestedCodecs)
           ? CANPLAY_YES : CANPLAY_NO;
  }
#endif
#ifdef MOZ_APPLEMEDIA
  if (IsAppleMediaSupportedType(nsDependentCString(aMIMEType), &codecList)) {
    result = CANPLAY_MAYBE;
  }
#endif
#ifdef MOZ_ANDROID_OMX
  if (MediaDecoder::IsAndroidMediaEnabled() &&
      EnsureAndroidMediaPluginHost()->FindDecoder(nsDependentCString(aMIMEType), &codecList))
    result = CANPLAY_MAYBE;
#endif
#ifdef NECKO_PROTOCOL_rtsp
  if (IsRtspSupportedType(nsDependentCString(aMIMEType))) {
    result = CANPLAY_MAYBE;
  }
#endif
  if (result == CANPLAY_NO || !aHaveRequestedCodecs || !codecList) {
    return result;
  }

  
  
  nsCharSeparatedTokenizer tokenizer(aRequestedCodecs, ',');
  bool expectMoreTokens = false;
  while (tokenizer.hasMoreTokens()) {
    const nsSubstring& token = tokenizer.nextToken();

    if (!CodecListContains(codecList, token)) {
      
      return CANPLAY_NO;
    }
    expectMoreTokens = tokenizer.separatorAfterCurrentToken();
  }
  if (expectMoreTokens) {
    
    return CANPLAY_NO;
  }
  return CANPLAY_YES;
}


static
already_AddRefed<MediaDecoder>
InstantiateDecoder(const nsACString& aType, MediaDecoderOwner* aOwner)
{
  MOZ_ASSERT(NS_IsMainThread());
  nsRefPtr<MediaDecoder> decoder;

#ifdef MOZ_FMP4
  if (IsMP4SupportedType(aType)) {
    decoder = new MP4Decoder();
    return decoder.forget();
  }
#endif
#ifdef MOZ_GSTREAMER
  if (IsGStreamerSupportedType(aType)) {
    decoder = new GStreamerDecoder();
    return decoder.forget();
  }
#endif
#ifdef MOZ_RAW
  if (IsRawType(aType)) {
    decoder = new RawDecoder();
    return decoder.forget();
  }
#endif
  if (IsOggType(aType)) {
    decoder = new OggDecoder();
    return decoder.forget();
  }
#ifdef MOZ_WAVE
  if (IsWaveType(aType)) {
    decoder = new WaveDecoder();
    return decoder.forget();
  }
#endif
#ifdef MOZ_OMX_DECODER
  if (IsOmxSupportedType(aType)) {
    
    
    if (IsB2GSupportOnlyType(aType)) {
      dom::HTMLMediaElement* element = aOwner->GetMediaElement();
      if (!element) {
        return nullptr;
      }
      nsIPrincipal* principal = element->NodePrincipal();
      if (!principal) {
        return nullptr;
      }
      if (principal->GetAppStatus() < nsIPrincipal::APP_STATUS_PRIVILEGED) {
        return nullptr;
      }
    }
#if ANDROID_VERSION >= 18
    decoder = MediaDecoder::IsOmxAsyncEnabled()
      ? static_cast<MediaDecoder*>(new MediaCodecDecoder())
      : static_cast<MediaDecoder*>(new MediaOmxDecoder());
#else
    decoder = new MediaOmxDecoder();
#endif
    return decoder.forget();
  }
#endif
#ifdef NECKO_PROTOCOL_rtsp
  if (IsRtspSupportedType(aType)) {
#if ANDROID_VERSION >= 18
    decoder = MediaDecoder::IsOmxAsyncEnabled()
      ? static_cast<MediaDecoder*>(new RtspMediaCodecDecoder())
      : static_cast<MediaDecoder*>(new RtspOmxDecoder());
#else
    decoder = new RtspOmxDecoder();
#endif
    return decoder.forget();
  }
#endif
#ifdef MOZ_ANDROID_OMX
  if (MediaDecoder::IsAndroidMediaEnabled() &&
      EnsureAndroidMediaPluginHost()->FindDecoder(aType, nullptr)) {
    decoder = new AndroidMediaDecoder(aType);
    return decoder.forget();
  }
#endif
#ifdef MOZ_WEBM
  if (IsWebMType(aType)) {
    decoder = new WebMDecoder();
    return decoder.forget();
  }
#endif
#ifdef MOZ_DIRECTSHOW
  
  
  if (IsDirectShowSupportedType(aType)) {
    decoder = new DirectShowDecoder();
    return decoder.forget();
  }
#endif
#ifdef MOZ_WMF
  if (IsWMFSupportedType(aType)) {
    decoder = new WMFDecoder();
    return decoder.forget();
  }
#endif
#ifdef MOZ_APPLEMEDIA
  if (IsAppleMediaSupportedType(aType)) {
    decoder = new AppleDecoder();
    return decoder.forget();
  }
#endif

  NS_ENSURE_TRUE(decoder != nullptr, nullptr);
  NS_ENSURE_TRUE(decoder->Init(aOwner), nullptr);
  return nullptr;
}


already_AddRefed<MediaDecoder>
DecoderTraits::CreateDecoder(const nsACString& aType, MediaDecoderOwner* aOwner)
{
  MOZ_ASSERT(NS_IsMainThread());
  nsRefPtr<MediaDecoder> decoder(InstantiateDecoder(aType, aOwner));
  NS_ENSURE_TRUE(decoder != nullptr, nullptr);
  NS_ENSURE_TRUE(decoder->Init(aOwner), nullptr);

  return decoder.forget();
}


MediaDecoderReader* DecoderTraits::CreateReader(const nsACString& aType, AbstractMediaDecoder* aDecoder)
{
  MOZ_ASSERT(NS_IsMainThread());
  MediaDecoderReader* decoderReader = nullptr;

  if (!aDecoder) {
    return decoderReader;
  }
#ifdef MOZ_FMP4
  if (IsMP4SupportedType(aType)) {
    decoderReader = new MP4Reader(aDecoder);
  } else
#endif
#ifdef MOZ_GSTREAMER
  if (IsGStreamerSupportedType(aType)) {
    decoderReader = new GStreamerReader(aDecoder);
  } else
#endif
#ifdef MOZ_RAW
  if (IsRawType(aType)) {
    decoderReader = new RawReader(aDecoder);
  } else
#endif
  if (IsOggType(aType)) {
    decoderReader = new OggReader(aDecoder);
  } else
#ifdef MOZ_WAVE
  if (IsWaveType(aType)) {
    decoderReader = new WaveReader(aDecoder);
  } else
#endif
#ifdef MOZ_OMX_DECODER
  if (IsOmxSupportedType(aType)) {
#if ANDROID_VERSION >= 18
    decoderReader = MediaDecoder::IsOmxAsyncEnabled()
      ? static_cast<MediaDecoderReader*>(new MediaCodecReader(aDecoder))
      : static_cast<MediaDecoderReader*>(new MediaOmxReader(aDecoder));
#else
    decoderReader = new MediaOmxReader(aDecoder);
#endif
  } else
#endif
#ifdef MOZ_ANDROID_OMX
  if (MediaDecoder::IsAndroidMediaEnabled() &&
      EnsureAndroidMediaPluginHost()->FindDecoder(aType, nullptr)) {
    decoderReader = new AndroidMediaReader(aDecoder, aType);
  } else
#endif
#ifdef MOZ_WEBM
  if (IsWebMType(aType)) {
    decoderReader = new WebMReader(aDecoder);
  } else
#endif
#ifdef MOZ_DIRECTSHOW
  
  
  if (IsDirectShowSupportedType(aType)) {
    decoderReader = new DirectShowReader(aDecoder);
  } else
#endif
#ifdef MOZ_WMF
  if (IsWMFSupportedType(aType)) {
    decoderReader = new WMFReader(aDecoder);
  } else
#endif
#ifdef MOZ_APPLEMEDIA
  if (IsAppleMediaSupportedType(aType)) {
    decoderReader = new AppleMP3Reader(aDecoder);
  } else
#endif
  if (false) {} 

  return decoderReader;
}


bool DecoderTraits::IsSupportedInVideoDocument(const nsACString& aType)
{
  
  
  
  if (!Preferences::GetBool("media.windows-media-foundation.play-stand-alone", true) ||
      !Preferences::GetBool("media.play-stand-alone", true)) {
    return false;
  }

  return
    IsOggType(aType) ||
#ifdef MOZ_OMX_DECODER
    
    
    
    (IsOmxSupportedType(aType) &&
     !IsB2GSupportOnlyType(aType)) ||
#endif
#ifdef MOZ_WEBM
    IsWebMType(aType) ||
#endif
#ifdef MOZ_GSTREAMER
    IsGStreamerSupportedType(aType) ||
#endif
#ifdef MOZ_ANDROID_OMX
    (MediaDecoder::IsAndroidMediaEnabled() && IsAndroidMediaType(aType)) ||
#endif
#ifdef MOZ_FMP4
    IsMP4SupportedType(aType) ||
#endif
#ifdef MOZ_WMF
    IsWMFSupportedType(aType) ||
#endif
#ifdef MOZ_DIRECTSHOW
    IsDirectShowSupportedType(aType) ||
#endif
#ifdef MOZ_APPLEMEDIA
    IsAppleMediaSupportedType(aType) ||
#endif
#ifdef NECKO_PROTOCOL_rtsp
    IsRtspSupportedType(aType) ||
#endif
    false;
}

}
