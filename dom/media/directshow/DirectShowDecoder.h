





#if !defined(DirectShowDecoder_h_)
#define DirectShowDecoder_h_

#include "MediaDecoder.h"

namespace mozilla {


class DirectShowDecoder : public MediaDecoder
{
public:

  DirectShowDecoder();
  virtual ~DirectShowDecoder();

  MediaDecoder* Clone() MOZ_OVERRIDE {
    if (!IsEnabled()) {
      return nullptr;
    }
    return new DirectShowDecoder();
  }

  MediaDecoderStateMachine* CreateStateMachine() MOZ_OVERRIDE;

  
  
  
  
  
  static bool GetSupportedCodecs(const nsACString& aType,
                                 char const *const ** aCodecList);

  
  static bool IsEnabled();
};

} 

#endif
