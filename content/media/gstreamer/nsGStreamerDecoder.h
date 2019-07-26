





#if !defined(nsGStreamerDecoder_h_)
#define nsGStreamerDecoder_h_

#include "nsBuiltinDecoder.h"

namespace mozilla {

class nsGStreamerDecoder : public nsBuiltinDecoder
{
public:
  virtual nsBuiltinDecoder* Clone() { return new nsGStreamerDecoder(); }
  virtual nsBuiltinDecoderStateMachine* CreateStateMachine();
};

} 

#endif
