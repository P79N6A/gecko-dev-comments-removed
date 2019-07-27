




#if !defined(MP4Decoder_h_)
#define MP4Decoder_h_

#include "MediaDecoder.h"

namespace mozilla {


class MP4Decoder : public MediaDecoder
{
public:

  virtual MediaDecoder* Clone() {
    if (!IsEnabled()) {
      return nullptr;
    }
    return new MP4Decoder();
  }

  virtual MediaDecoderStateMachine* CreateStateMachine();

#ifdef MOZ_EME
  virtual nsresult SetCDMProxy(CDMProxy* aProxy) MOZ_OVERRIDE;
#endif

  
  
  
  
  static bool GetSupportedCodecs(const nsACString& aType,
                                 char const *const ** aCodecList);

  
  
  static bool IsEnabled();
};

} 

#endif
