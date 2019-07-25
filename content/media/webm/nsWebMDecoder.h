





































#if !defined(nsWebMDecoder_h_)
#define nsWebMDecoder_h_

#include "nsBuiltinDecoder.h"

class nsWebMDecoder : public nsBuiltinDecoder
{
public:
  virtual nsMediaDecoder* Clone() { return new nsWebMDecoder(); }
  virtual nsDecoderStateMachine* CreateStateMachine();
};

#endif
