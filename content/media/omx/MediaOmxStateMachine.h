#include "MediaDecoder.h"
#include "MediaDecoderReader.h"
#include "MediaDecoderStateMachine.h"

namespace mozilla {

class MediaOmxStateMachine : public MediaDecoderStateMachine
{
public:
  MediaOmxStateMachine(MediaDecoder *aDecoder,
                       MediaDecoderReader *aReader,
                       bool aRealTime = false)
    : MediaDecoderStateMachine(aDecoder, aReader, aRealTime) { }

protected:
  
  
  
  
  
  
  
  
  
  uint32_t GetAmpleVideoFrames() { return 3; }
};

} 
