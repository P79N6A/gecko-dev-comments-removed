





#include "DecoderTraits.h"
#include "MediaDecoder.h"
#include "nsCharSeparatedTokenizer.h"
#include "mozilla/Preferences.h"

#ifdef MOZ_MEDIA_PLUGINS
#include "MediaPluginHost.h"
#endif

#ifdef MOZ_OGG
#include "OggDecoder.h"
#include "OggReader.h"
#endif
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
#ifdef MOZ_MEDIA_PLUGINS
#include "MediaPluginHost.h"
#include "MediaPluginDecoder.h"
#include "MediaPluginReader.h"
#include "MediaPluginHost.h"
#endif
#ifdef MOZ_OMX_DECODER
#include "MediaOmxDecoder.h"
#include "MediaOmxReader.h"
#include "nsIPrincipal.h"
#include "mozilla/dom/HTMLMediaElement.h"
#endif
#ifdef MOZ_RTSP
#include "RtspOmxDecoder.h"
#include "RtspOmxReader.h"
#endif
#ifdef MOZ_DASH
#include "DASHDecoder.h"
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

#ifdef MOZ_OGG


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
#endif

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

static char const *const gWebMCodecs[4] = {
  "vp8",
  "vp8.0",
  "vorbis",
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
#ifdef MOZ_OGG
  if (IsOggType(aMimeType) && !Preferences::GetBool("media.prefer-gstreamer", false))
    return false;
#endif

  return GStreamerDecoder::CanHandleMediaType(aMimeType, nullptr);
}
#endif

#ifdef MOZ_OMX_DECODER
static const char* const gOmxTypes[7] = {
  "audio/mpeg",
  "audio/mp4",
  "audio/amr",
  "video/mp4",
  "video/3gpp",
  "video/quicktime",
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
#endif

#ifdef MOZ_RTSP
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
#ifdef MOZ_RTSP
  return CodecListContains(gRtspTypes, aMimeType);
#else
  return false;
#endif
}

#ifdef MOZ_MEDIA_PLUGINS
static bool
IsMediaPluginsType(const nsACString& aType)
{
  if (!MediaDecoder::IsMediaPluginsEnabled()) {
    return false;
  }

  static const char* supportedTypes[] = {
    "audio/mpeg", "audio/mp4", "video/mp4", nullptr
  };
  return CodecListContains(supportedTypes, aType);
}
#endif

#ifdef MOZ_DASH

static const char* const gDASHMPDTypes[2] = {
  "application/dash+xml",
  nullptr
};

static bool
IsDASHMPDType(const nsACString& aType)
{
  if (!MediaDecoder::IsDASHEnabled()) {
    return false;
  }

  return CodecListContains(gDASHMPDTypes, aType);
}
#endif

#ifdef MOZ_WMF
static bool
IsWMFSupportedType(const nsACString& aType)
{
  return WMFDecoder::GetSupportedCodecs(aType, nullptr);
}
#endif

#ifdef MOZ_DIRECTSHOW
static bool
IsDirectShowSupportedType(const nsACString& aType)
{
  return DirectShowDecoder::GetSupportedCodecs(aType, nullptr);
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
  char const* const* codecList = nullptr;
  CanPlayStatus result = CANPLAY_NO;
#ifdef MOZ_RAW
  if (IsRawType(nsDependentCString(aMIMEType))) {
    codecList = gRawCodecs;
    result = CANPLAY_MAYBE;
  }
#endif
#ifdef MOZ_OGG
  if (IsOggType(nsDependentCString(aMIMEType))) {
    codecList = MediaDecoder::IsOpusEnabled() ? gOggCodecsWithOpus : gOggCodecs;
    result = CANPLAY_MAYBE;
  }
#endif
#ifdef MOZ_WAVE
  if (IsWaveType(nsDependentCString(aMIMEType))) {
    codecList = gWaveCodecs;
    result = CANPLAY_MAYBE;
  }
#endif
#ifdef MOZ_WEBM
  if (IsWebMType(nsDependentCString(aMIMEType))) {
    codecList = gWebMCodecs;
    result = CANPLAY_YES;
  }
#endif
#ifdef MOZ_DASH
  if (IsDASHMPDType(nsDependentCString(aMIMEType))) {
    
    codecList = gWebMCodecs;
    result = CANPLAY_YES;
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
    } else {
      codecList = gH264Codecs;
    }
  }
#endif
#ifdef MOZ_WMF
  if (WMFDecoder::GetSupportedCodecs(nsDependentCString(aMIMEType), &codecList)) {
    result = CANPLAY_MAYBE;
  }
#endif
#ifdef MOZ_DIRECTSHOW
  if (DirectShowDecoder::GetSupportedCodecs(nsDependentCString(aMIMEType), &codecList)) {
    result = CANPLAY_MAYBE;
  }
#endif
#ifdef MOZ_APPLEMEDIA
  if (IsAppleMediaSupportedType(nsDependentCString(aMIMEType), &codecList)) {
    result = CANPLAY_MAYBE;
  }
#endif
#ifdef MOZ_MEDIA_PLUGINS
  if (MediaDecoder::IsMediaPluginsEnabled() &&
      GetMediaPluginHost()->FindDecoder(nsDependentCString(aMIMEType), &codecList))
    result = CANPLAY_MAYBE;
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


already_AddRefed<MediaDecoder>
DecoderTraits::CreateDecoder(const nsACString& aType, MediaDecoderOwner* aOwner)
{
  nsRefPtr<MediaDecoder> decoder;

#ifdef MOZ_GSTREAMER
  if (IsGStreamerSupportedType(aType)) {
    decoder = new GStreamerDecoder();
  }
#endif
#ifdef MOZ_RAW
  if (IsRawType(aType)) {
    decoder = new RawDecoder();
  }
#endif
#ifdef MOZ_OGG
  if (IsOggType(aType)) {
    decoder = new OggDecoder();
  }
#endif
#ifdef MOZ_WAVE
  if (IsWaveType(aType)) {
    decoder = new WaveDecoder();
  }
#endif
#ifdef MOZ_OMX_DECODER
  if (IsOmxSupportedType(aType)) {
    
    
    if (aType.EqualsASCII("audio/amr")) {
      HTMLMediaElement* element = aOwner->GetMediaElement();
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
    decoder = new MediaOmxDecoder();
  }
#endif
#ifdef MOZ_RTSP
  if (IsRtspSupportedType(aType)) {
    decoder = new RtspOmxDecoder();
  }
#endif
#ifdef MOZ_MEDIA_PLUGINS
  if (MediaDecoder::IsMediaPluginsEnabled() &&
      GetMediaPluginHost()->FindDecoder(aType, nullptr)) {
    decoder = new MediaPluginDecoder(aType);
  }
#endif
#ifdef MOZ_WEBM
  if (IsWebMType(aType)) {
    decoder = new WebMDecoder();
  }
#endif
#ifdef MOZ_DASH
  if (IsDASHMPDType(aType)) {
    decoder = new DASHDecoder();
  }
#endif
#ifdef MOZ_DIRECTSHOW
  
  
  if (IsDirectShowSupportedType(aType)) {
    decoder = new DirectShowDecoder();
  }
#endif
#ifdef MOZ_WMF
  if (IsWMFSupportedType(aType)) {
    decoder = new WMFDecoder();
  }
#endif
#ifdef MOZ_APPLEMEDIA
  if (IsAppleMediaSupportedType(aType)) {
    decoder = new AppleDecoder();
  }
#endif

  NS_ENSURE_TRUE(decoder != nullptr, nullptr);
  NS_ENSURE_TRUE(decoder->Init(aOwner), nullptr);

  return decoder.forget();
}


MediaDecoderReader* DecoderTraits::CreateReader(const nsACString& aType, AbstractMediaDecoder* aDecoder)
{
  MediaDecoderReader* decoderReader = nullptr;

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
#ifdef MOZ_OGG
  if (IsOggType(aType)) {
    decoderReader = new OggReader(aDecoder);
  } else
#endif
#ifdef MOZ_WAVE
  if (IsWaveType(aType)) {
    decoderReader = new WaveReader(aDecoder);
  } else
#endif
#ifdef MOZ_OMX_DECODER
  if (IsOmxSupportedType(aType)) {
    decoderReader = new MediaOmxReader(aDecoder);
  } else
#endif
#ifdef MOZ_MEDIA_PLUGINS
  if (MediaDecoder::IsMediaPluginsEnabled() &&
      GetMediaPluginHost()->FindDecoder(aType, nullptr)) {
    decoderReader = new MediaPluginReader(aDecoder, aType);
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
#ifdef MOZ_DASH
  
#endif
  if (false) {} 

  return decoderReader;
}


bool DecoderTraits::IsSupportedInVideoDocument(const nsACString& aType)
{
  return
#ifdef MOZ_OGG
    IsOggType(aType) ||
#endif
#ifdef MOZ_OMX_DECODER
    
    
    (IsOmxSupportedType(aType) && !aType.EqualsASCII("audio/amr")) ||
#endif
#ifdef MOZ_WEBM
    IsWebMType(aType) ||
#endif
#ifdef MOZ_DASH
    IsDASHMPDType(aType) ||
#endif
#ifdef MOZ_GSTREAMER
    IsGStreamerSupportedType(aType) ||
#endif
#ifdef MOZ_MEDIA_PLUGINS
    (MediaDecoder::IsMediaPluginsEnabled() && IsMediaPluginsType(aType)) ||
#endif
#ifdef MOZ_WMF
    (IsWMFSupportedType(aType) &&
     Preferences::GetBool("media.windows-media-foundation.play-stand-alone", true)) ||
#endif
#ifdef MOZ_DIRECTSHOW
    IsDirectShowSupportedType(aType) ||
#endif
#ifdef MOZ_APPLEMEDIA
    IsAppleMediaSupportedType(aType) ||
#endif
    false;
}

}
