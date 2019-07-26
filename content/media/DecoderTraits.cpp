





#include "DecoderTraits.h"
#include "MediaDecoder.h"
#include "nsCharSeparatedTokenizer.h"
#ifdef MOZ_MEDIA_PLUGINS
#include "MediaPluginHost.h"
#endif

namespace mozilla
{

static bool
CodecListContains(char const *const * aCodecs, const nsAString& aCodec)
{
  for (int32_t i = 0; aCodecs[i]; ++i) {
    if (aCodec.EqualsASCII(aCodecs[i]))
      return true;
  }
  return false;
}

#ifdef MOZ_RAW
static const char gRawTypes[2][16] = {
  "video/x-raw",
  "video/x-raw-yuv"
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

  for (uint32_t i = 0; i < ArrayLength(gRawTypes); ++i) {
    if (aType.EqualsASCII(gRawTypes[i])) {
      return true;
    }
  }

  return false;
}
#endif

#ifdef MOZ_OGG


static const char gOggTypes[3][16] = {
  "video/ogg",
  "audio/ogg",
  "application/ogg"
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

  for (uint32_t i = 0; i < ArrayLength(gOggTypes); ++i) {
    if (aType.EqualsASCII(gOggTypes[i])) {
      return true;
    }
  }

  return false;
}
#endif

#ifdef MOZ_WAVE



static const char gWaveTypes[4][15] = {
  "audio/x-wav",
  "audio/wav",
  "audio/wave",
  "audio/x-pn-wav"
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

  for (uint32_t i = 0; i < ArrayLength(gWaveTypes); ++i) {
    if (aType.EqualsASCII(gWaveTypes[i])) {
      return true;
    }
  }

  return false;
}
#endif

#ifdef MOZ_WEBM
static const char gWebMTypes[2][11] = {
  "video/webm",
  "audio/webm"
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

  for (uint32_t i = 0; i < ArrayLength(gWebMTypes); ++i) {
    if (aType.EqualsASCII(gWebMTypes[i])) {
      return true;
    }
  }

  return false;
}
#endif

#ifdef MOZ_GSTREAMER
static const char gH264Types[3][16] = {
  "video/mp4",
  "video/3gpp",
  "video/quicktime",
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
  for (uint32_t i = 0; i < ArrayLength(gH264Types); ++i) {
    if (aType.EqualsASCII(gH264Types[i])) {
      return true;
    }
  }
  return false;
}
#endif

#ifdef MOZ_WIDGET_GONK
static const char gOmxTypes[5][16] = {
  "audio/mpeg",
  "audio/mp4",
  "video/mp4",
  "video/3gpp",
  "video/quicktime",
};

bool
DecoderTraits::IsOmxSupportedType(const nsACString& aType)
{
  if (!MediaDecoder::IsOmxEnabled()) {
    return false;
  }

  for (uint32_t i = 0; i < ArrayLength(gOmxTypes); ++i) {
    if (aType.EqualsASCII(gOmxTypes[i])) {
      return true;
    }
  }

  return false;
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
    "audio/mpeg", "audio/mp4", "video/mp4"
  };
  for (uint32_t i = 0; i < ArrayLength(supportedTypes); ++i) {
    if (aType.EqualsASCII(supportedTypes[i])) {
      return true;
    }
  }
  return false;
}
#endif

#ifdef MOZ_DASH

static const char gDASHMPDTypes[1][21] = {
  "application/dash+xml"
};


bool
DecoderTraits::IsDASHMPDType(const nsACString& aType)
{
  if (!MediaDecoder::IsDASHEnabled()) {
    return false;
  }

  for (uint32_t i = 0; i < ArrayLength(gDASHMPDTypes); ++i) {
    if (aType.EqualsASCII(gDASHMPDTypes[i])) {
      return true;
    }
  }

  return false;
}
#endif


bool DecoderTraits::ShouldHandleMediaType(const char* aMIMEType)
{
#ifdef MOZ_RAW
  if (IsRawType(nsDependentCString(aMIMEType)))
    return true;
#endif
#ifdef MOZ_OGG
  if (IsOggType(nsDependentCString(aMIMEType)))
    return true;
#endif
#ifdef MOZ_WEBM
  if (IsWebMType(nsDependentCString(aMIMEType)))
    return true;
#endif
#ifdef MOZ_GSTREAMER
  if (IsH264Type(nsDependentCString(aMIMEType)))
    return true;
#endif
#ifdef MOZ_WIDGET_GONK
  if (IsOmxSupportedType(nsDependentCString(aMIMEType))) {
    return true;
  }
#endif
#ifdef MOZ_MEDIA_PLUGINS
  if (MediaDecoder::IsMediaPluginsEnabled() && GetMediaPluginHost()->FindDecoder(nsDependentCString(aMIMEType), NULL))
    return true;
#endif
  
  
  
  
  
  return false;
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

