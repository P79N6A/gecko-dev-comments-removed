






































#include "nsOggDecoderStateMachine.h"
#include "nsOggReader.h"
#include "nsOggDecoder.h"

nsOggDecoderStateMachine::nsOggDecoderStateMachine(nsBuiltinDecoder* aDecoder) :
  nsBuiltinDecoderStateMachine(aDecoder, new nsOggReader(aDecoder))
{
}
