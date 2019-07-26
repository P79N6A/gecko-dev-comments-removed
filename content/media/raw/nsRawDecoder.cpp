



#include "nsBuiltinDecoderStateMachine.h"
#include "nsRawReader.h"
#include "nsRawDecoder.h"

namespace mozilla {

nsBuiltinDecoderStateMachine* nsRawDecoder::CreateStateMachine()
{
  return new nsBuiltinDecoderStateMachine(this, new nsRawReader(this), true);
}

} 

