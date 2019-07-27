
































#ifndef GMP_VIDEO_HOST_h_
#define GMP_VIDEO_HOST_h_

#include "gmp-video-errors.h"
#include "gmp-video-frame-i420.h"
#include "gmp-video-frame-encoded.h"
#include "gmp-video-codec.h"

class GMPVideoHost
{
public:
  
  
  virtual GMPVideoErr CreateFrame(GMPVideoFrameFormat aFormat, GMPVideoFrame** aFrame) = 0;
  virtual GMPVideoErr CreatePlane(GMPPlane** aPlane) = 0;
};

#endif 
