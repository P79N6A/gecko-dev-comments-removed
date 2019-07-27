
































#ifndef GMP_VIDEO_FRAME_h_
#define GMP_VIDEO_FRAME_h_

#include "gmp-video-errors.h"
#include "gmp-video-plane.h"

enum GMPVideoFrameFormat {
  kGMPEncodedVideoFrame = 0,
  kGMPI420VideoFrame = 1
};

class GMPVideoFrame {
public:
  virtual GMPVideoFrameFormat GetFrameFormat() = 0;
  
  virtual void Destroy() = 0;
};

#endif 
