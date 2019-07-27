





#include "MediaDecoderStateMachine.h"
#include "AndroidMediaDecoder.h"
#include "AndroidMediaReader.h"

namespace mozilla {

AndroidMediaDecoder::AndroidMediaDecoder(const nsACString& aType) : mType(aType)
{
}

MediaDecoderStateMachine* AndroidMediaDecoder::CreateStateMachine()
{
  return new MediaDecoderStateMachine(this, new AndroidMediaReader(this, mType));
}

} 

