




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

  virtual MediaDecoder* Clone() override final;
  virtual MediaDecoderStateMachine* CreateStateMachine() override final;
  virtual void ApplyStateToStateMachine(PlayState aState)
                                        override final;
};

} 

#endif
