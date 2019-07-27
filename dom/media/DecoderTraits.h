





#ifndef DecoderTraits_h_
#define DecoderTraits_h_

#include "nsCOMPtr.h"

class nsAString;
class nsACString;

namespace mozilla {

class AbstractMediaDecoder;
class MediaDecoder;
class MediaDecoderOwner;
class MediaDecoderReader;

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

  
  
  static already_AddRefed<MediaDecoder> CreateDecoder(const nsACString& aType,
                                                      MediaDecoderOwner* aOwner);

  
  
  static MediaDecoderReader* CreateReader(const nsACString& aType,
                                          AbstractMediaDecoder* aDecoder);

  
  
  
  static bool IsSupportedInVideoDocument(const nsACString& aType);

  
  
  static bool DecoderWaitsForOnConnected(const nsACString& aType);

  static bool IsWebMType(const nsACString& aType);
};

}

#endif

