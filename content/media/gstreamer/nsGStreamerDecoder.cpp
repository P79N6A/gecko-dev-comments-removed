





#include "nsBuiltinDecoderStateMachine.h"
#include "nsGStreamerReader.h"
#include "nsGStreamerDecoder.h"

nsBuiltinDecoderStateMachine* nsGStreamerDecoder::CreateStateMachine()
{
  return new nsBuiltinDecoderStateMachine(this, new nsGStreamerReader(this));
}
