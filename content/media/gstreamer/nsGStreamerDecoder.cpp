





#include "nsBuiltinDecoderStateMachine.h"
#include "nsGStreamerReader.h"
#include "nsGStreamerDecoder.h"

namespace mozilla {

nsBuiltinDecoderStateMachine* nsGStreamerDecoder::CreateStateMachine()
{
  return new nsBuiltinDecoderStateMachine(this, new nsGStreamerReader(this));
}

} 

