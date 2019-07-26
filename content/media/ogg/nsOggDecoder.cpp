





#include "nsBuiltinDecoderStateMachine.h"
#include "nsOggReader.h"
#include "nsOggDecoder.h"

namespace mozilla {

nsBuiltinDecoderStateMachine* nsOggDecoder::CreateStateMachine()
{
  return new nsBuiltinDecoderStateMachine(this, new nsOggReader(this));
}

} 
