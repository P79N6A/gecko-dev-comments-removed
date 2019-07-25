






































#include "nsOggDecoderStateMachine.h"
#include "nsOggReader.h"
#include "nsOggDecoder.h"

nsOggDecoderStateMachine::nsOggDecoderStateMachine(nsBuiltinDecoder* aDecoder) :
  nsBuiltinDecoderStateMachine(aDecoder, new nsOggReader(aDecoder))
{
}

void nsOggDecoderStateMachine::LoadMetadata()
{
  nsBuiltinDecoderStateMachine::LoadMetadata();

  
  
  
  
  

  if (mState != DECODER_STATE_SHUTDOWN &&
      mDecoder->GetCurrentStream()->GetLength() >= 0 &&
      mSeekable &&
      mEndTime == -1)
  {
    mDecoder->StopProgressUpdates();
    FindEndTime();
    mDecoder->StartProgressUpdates();
  }
}
