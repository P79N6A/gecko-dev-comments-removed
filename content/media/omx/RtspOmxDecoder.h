




#if !defined(RtspOmxDecoder_h_)
#define RtspOmxDecoder_h_

#include "base/basictypes.h"
#include "MediaDecoder.h"

namespace mozilla {








class RtspOmxDecoder : public MediaDecoder
{
public:
  RtspOmxDecoder()
  : MediaDecoder() {
    MOZ_COUNT_CTOR(RtspOmxDecoder);
  }

  ~RtspOmxDecoder() {
    MOZ_COUNT_DTOR(RtspOmxDecoder);
  }

  virtual MediaDecoder* Clone() MOZ_OVERRIDE;
  virtual MediaDecoderStateMachine* CreateStateMachine() MOZ_OVERRIDE;
  
  
  
  virtual void ApplyStateToStateMachine(PlayState aState) MOZ_OVERRIDE;
};

} 

#endif
