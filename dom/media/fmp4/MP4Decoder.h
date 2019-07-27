




#if !defined(MP4Decoder_h_)
#define MP4Decoder_h_

#include "MediaDecoder.h"

namespace mozilla {


class MP4Decoder : public MediaDecoder
{
public:
  MP4Decoder();

  virtual MediaDecoder* Clone() override {
    if (!IsEnabled()) {
      return nullptr;
    }
    return new MP4Decoder();
  }

  virtual MediaDecoderStateMachine* CreateStateMachine() override;

#ifdef MOZ_EME
  virtual nsresult SetCDMProxy(CDMProxy* aProxy) override;
#endif

  
  
  
  
  static bool CanHandleMediaType(const nsACString& aMIMEType,
                                 const nsAString& aCodecs,
                                 bool& aOutContainsAAC,
                                 bool& aOutContainsH264,
                                 bool& aOutContainsMP3);

  
  static bool IsEnabled();

  static bool IsVideoAccelerated(layers::LayersBackend aBackend);
  static bool CanCreateAACDecoder();
  static bool CanCreateH264Decoder();
};

} 

#endif
