






































#include "nsOggDecoderStateMachine.h"
#include "nsOggReader.h"
#include "nsOggDecoder.h"

nsDecoderStateMachine* nsOggDecoder::CreateStateMachine()
{
  return new nsOggDecoderStateMachine(this);
}
