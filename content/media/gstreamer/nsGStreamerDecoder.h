





#if !defined(nsGStreamerDecoder_h_)
#define nsGStreamerDecoder_h_

#include "nsBuiltinDecoder.h"

class nsGStreamerDecoder : public nsBuiltinDecoder
{
public:
  virtual nsMediaDecoder* Clone() { return new nsGStreamerDecoder(); }
  virtual nsDecoderStateMachine* CreateStateMachine();
};

#endif
