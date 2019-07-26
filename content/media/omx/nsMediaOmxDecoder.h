




#if !defined(nsMediaOmxDecoder_h_)
#define nsMediaOmxDecoder_h_

#include "base/basictypes.h"
#include "nsBuiltinDecoder.h"

class nsMediaOmxDecoder : public nsBuiltinDecoder
{
public:
  nsMediaOmxDecoder();
  ~nsMediaOmxDecoder();

  virtual nsMediaDecoder* Clone();
  virtual nsDecoderStateMachine* CreateStateMachine();
};

#endif
