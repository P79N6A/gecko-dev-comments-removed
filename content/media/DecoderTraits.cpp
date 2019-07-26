





#include "DecoderTraits.h"
#include "MediaDecoder.h"
#include "nsCharSeparatedTokenizer.h"
#ifdef MOZ_MEDIA_PLUGINS
#include "MediaPluginHost.h"
#endif
#ifdef MOZ_GSTREAMER
#include "mozilla/Preferences.h"
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

}

