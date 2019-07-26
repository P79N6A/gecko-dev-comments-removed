



#ifndef ExtVideoCodec_h__
#define ExtVideoCodec_h__

#include "MediaConduitInterface.h"

namespace mozilla {
class ExtVideoCodec {
public:
  static VideoEncoder* CreateEncoder();
  static VideoDecoder* CreateDecoder();
};

}

#endif 
