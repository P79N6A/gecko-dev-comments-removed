



#ifndef GMPVIDEOCODEC_H_
#define GMPVIDEOCODEC_H_

#include "MediaConduitInterface.h"

namespace mozilla {
class GmpVideoCodec {
 public:
  static VideoEncoder* CreateEncoder();
  static VideoDecoder* CreateDecoder();
};

}

#endif
