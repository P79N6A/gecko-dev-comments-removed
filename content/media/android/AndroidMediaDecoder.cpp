





#include "MediaDecoderStateMachine.h"
#include "MediaPluginDecoder.h"
#include "MediaPluginReader.h"

namespace mozilla {

MediaPluginDecoder::MediaPluginDecoder(const nsACString& aType) : mType(aType)
{
}

MediaDecoderStateMachine* MediaPluginDecoder::CreateStateMachine()
{
  return new MediaDecoderStateMachine(this, new MediaPluginReader(this, mType));
}

} 

