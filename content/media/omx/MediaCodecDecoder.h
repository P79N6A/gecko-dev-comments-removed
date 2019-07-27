





#ifndef MEDIA_CODEC_DECODER_H
#define MEDIA_CODEC_DECODER_H

#include "MediaDecoder.h"

namespace mozilla {


class MediaCodecDecoder : public MediaDecoder
{
public:

  virtual MediaDecoder* Clone();

  virtual MediaDecoderStateMachine* CreateStateMachine();
};

} 

#endif 
