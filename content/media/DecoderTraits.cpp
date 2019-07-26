





#include "DecoderTraits.h"
#include "MediaDecoder.h"
#include "nsCharSeparatedTokenizer.h"

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
#include "mozilla/Preferences.h"
#include "GStreamerDecoder.h"
#include "GStreamerReader.h"
#endif
#ifdef MOZ_MEDIA_PLUGINS
#include "MediaPluginHost.h"
#include "MediaPluginDecoder.h"
#include "MediaPluginReader.h"
#include "MediaPluginHost.h"
#endif
#ifdef MOZ_WIDGET_GONK
#include "MediaOmxDecoder.h"
#include "MediaOmxReader.h"
#endif
#ifdef MOZ_DASH
#include "DASHDecoder.h"
#endif
#ifdef MOZ_WMF
#include "WMFDecoder.h"
#include "WMFReader.h"
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


bool
DecoderTraits::IsRawType(const nsACString& aType)
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

bool
DecoderTraits::IsOggType(const nsACString& aType)
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

bool
DecoderTraits::IsWaveType(const nsACString& aType)
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

bool
DecoderTraits::IsWebMType(const nsACString& aType)
{
  if (!MediaDecoder::IsWebMEnabled()) {
    return false;
  }

  return CodecListContains(gWebMTypes, aType);
}
#endif

#ifdef MOZ_GSTREAMER
static const char* const gH264Types[4] = {
  "video/mp4",
  "video/3gpp",
  "video/quicktime",
  nullptr
};

bool
DecoderTraits::IsGStreamerSupportedType(const nsACString& aMimeType)
{
  if (!MediaDecoder::IsGStreamerEnabled())
    return false;
  if (IsH264Type(aMimeType))
    return true;
  if (!Preferences::GetBool("media.prefer-gstreamer", false))
    return false;
#ifdef MOZ_WEBM
  if (IsWebMType(aMimeType))
    return true;
#endif
#ifdef MOZ_OGG
  if (IsOggType(aMimeType))
    return true;
#endif
  return false;
}

bool
DecoderTraits::IsH264Type(const nsACString& aType)
{
  return CodecListContains(gH264Types, aType);
}
#endif

#ifdef MOZ_WIDGET_GONK
static const char* const gOmxTypes[6] = {
  "audio/mpeg",
  "audio/mp4",
  "video/mp4",
  "video/3gpp",
  "video/quicktime",
  nullptr
};

bool
DecoderTraits::IsOmxSupportedType(const nsACString& aType)
{
  if (!MediaDecoder::IsOmxEnabled()) {
    return false;
  }

  return CodecListContains(gOmxTypes, aType);
}
#endif

#if defined(MOZ_GSTREAMER) || defined(MOZ_WIDGET_GONK)
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
#endif

#ifdef MOZ_MEDIA_PLUGINS
bool
DecoderTraits::IsMediaPluginsType(const nsACString& aType)
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


bool
DecoderTraits::IsDASHMPDType(const nsACString& aType)
{
  if (!MediaDecoder::IsDASHEnabled()) {
    return false;
  }

  return CodecListContains(gDASHMPDTypes, aType);
}
#endif

#ifdef MOZ_WMF
bool DecoderTraits::IsWMFSupportedType(const nsACString& aType)
{
  return WMFDecoder::GetSupportedCodecs(aType, nullptr);
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
  if (IsH264Type(nsDependentCString(aMIMEType))) {
    codecList = gH264Codecs;
    result = CANPLAY_MAYBE;
  }
#endif
#ifdef MOZ_WIDGET_GONK
  if (IsOmxSupportedType(nsDependentCString(aMIMEType))) {
    codecList = gH264Codecs;
    result = CANPLAY_MAYBE;
  }
#endif
#ifdef MOZ_WMF
  if (WMFDecoder::GetSupportedCodecs(nsDependentCString(aMIMEType), &codecList)) {
    result = CANPLAY_MAYBE;
  }
#endif
#ifdef MOZ_MEDIA_PLUGINS
  if (MediaDecoder::IsMediaPluginsEnabled() &&
      GetMediaPluginHost()->FindDecoder(nsDependentCString(aMIMEType), &codecList))
    result = CANPLAY_MAYBE;
#endif
  if (result == CANPLAY_NO || !aHaveRequestedCodecs) {
    return result;
  }

  
  
  nsCharSeparatedTokenizer tokenizer(aRequestedCodecs, ',');
  bool expectMoreTokens = false;
  while (tokenizer.hasMoreTokens()) {
    const nsSubstring& token = tokenizer.nextToken();

    if (!CodecListContains(codecList, token)) {
      
      return CANPLAY_NO;
    }
    expectMoreTokens = tokenizer.lastTokenEndedWithSeparator();
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
#ifdef MOZ_WIDGET_GONK
  if (IsOmxSupportedType(aType)) {
    decoder = new MediaOmxDecoder();
  }
#endif
#ifdef MOZ_MEDIA_PLUGINS
  if (MediaDecoder::IsMediaPluginsEnabled() && GetMediaPluginHost()->FindDecoder(aType, NULL)) {
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
#ifdef MOZ_WMF
  if (IsWMFSupportedType(aType)) {
    decoder = new WMFDecoder();
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
#ifdef MOZ_WIDGET_GONK
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
#ifdef MOZ_WMF
  if (IsWMFSupportedType(aType)) {
    decoderReader = new WMFReader(aDecoder);
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
#ifdef MOZ_WIDGET_GONK
    IsOmxSupportedType(aType) ||
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
    IsWMFSupportedType(aType) ||
#endif
    false;
}

}
