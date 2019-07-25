



































#if !defined(nsRawDecoder_h_)
#define nsRawDecoder_h_

#include "nsBuiltinDecoder.h"

class nsRawDecoder : public nsBuiltinDecoder
{
public:
  virtual nsMediaDecoder* Clone() { return new nsRawDecoder(); }
  virtual nsDecoderStateMachine* CreateStateMachine();
};

#endif
