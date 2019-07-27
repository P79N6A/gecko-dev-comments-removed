





#ifndef MEDIA_CODEC_DECODER_H
#define MEDIA_CODEC_DECODER_H

#include "MediaOmxCommonDecoder.h"

namespace mozilla {


class MediaCodecDecoder : public MediaOmxCommonDecoder
{
public:

  virtual MediaDecoder* Clone();

  virtual MediaOmxCommonReader* CreateReader();

  virtual MediaDecoderStateMachine* CreateStateMachine(MediaOmxCommonReader* aReader);
};

} 

#endif 
