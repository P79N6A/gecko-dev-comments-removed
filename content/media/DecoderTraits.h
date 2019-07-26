





#ifndef DecoderTraits_h_
#define DecoderTraits_h_

#include "nsCOMPtr.h"
#include "nsAString.h"

namespace mozilla
{

class MediaDecoder;
class MediaDecoderOwner;

enum CanPlayStatus {
  CANPLAY_NO,
  CANPLAY_MAYBE,
  CANPLAY_YES
};

class DecoderTraits {
public:
  
  
  
  
  
  
  static CanPlayStatus CanHandleMediaType(const char* aMIMEType,
                                          bool aHaveRequestedCodecs,
                                          const nsAString& aRequestedCodecs);

  
  
  
  
  static bool ShouldHandleMediaType(const char* aMIMEType);

#ifdef MOZ_RAW
  static bool IsRawType(const nsACString& aType);
#endif

#ifdef MOZ_OGG
  static bool IsOggType(const nsACString& aType);
#endif

#ifdef MOZ_WAVE
  static bool IsWaveType(const nsACString& aType);
#endif

#ifdef MOZ_WEBM
  static bool IsWebMType(const nsACString& aType);
#endif

#ifdef MOZ_GSTREAMER
  
  
  static bool IsGStreamerSupportedType(const nsACString& aType);
  static bool IsH264Type(const nsACString& aType);
#endif

#ifdef MOZ_WIDGET_GONK
  static bool IsOmxSupportedType(const nsACString& aType);
#endif

#ifdef MOZ_MEDIA_PLUGINS
  static bool IsMediaPluginsType(const nsACString& aType);
#endif

#ifdef MOZ_DASH
  static bool IsDASHMPDType(const nsACString& aType);
#endif

#ifdef MOZ_WMF
  static bool IsWMFSupportedType(const nsACString& aType);
#endif

  
  
  static already_AddRefed<MediaDecoder> CreateDecoder(const nsACString& aType,
                                                      MediaDecoderOwner* aOwner);

  
  
  
  static bool IsSupportedInVideoDocument(const nsACString& aType);
};

}

#endif

