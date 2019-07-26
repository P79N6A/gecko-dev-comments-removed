





#include "MediaDecoderStateMachine.h"
#include "GStreamerReader.h"
#include "GStreamerDecoder.h"

namespace mozilla {

MediaDecoderStateMachine* GStreamerDecoder::CreateStateMachine()
{
  return new MediaDecoderStateMachine(this, new GStreamerReader(this));
}

} 

