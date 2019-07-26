




#if !defined(nsMediaOmxDecoder_h_)
#define nsMediaOmxDecoder_h_

#include "base/basictypes.h"
#include "nsBuiltinDecoder.h"

namespace mozilla {

class nsMediaOmxDecoder : public nsBuiltinDecoder
{
public:
  nsMediaOmxDecoder();
  ~nsMediaOmxDecoder();

  virtual nsBuiltinDecoder* Clone();
  virtual nsBuiltinDecoderStateMachine* CreateStateMachine();
};

} 

#endif
