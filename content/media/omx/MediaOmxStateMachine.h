#include "MediaDecoder.h"
#include "MediaDecoderReader.h"
#include "MediaDecoderStateMachine.h"

namespace mozilla {

class MediaOmxStateMachine : public MediaDecoderStateMachine
{
public:
  MediaOmxStateMachine(MediaDecoder *aDecoder,
                       MediaDecoderReader *aReader)
    : MediaDecoderStateMachine(aDecoder, aReader) { }

protected:
  
  
  
  
  
  
  
  
  
  uint32_t GetAmpleVideoFrames() { return 2; }
};

} 
