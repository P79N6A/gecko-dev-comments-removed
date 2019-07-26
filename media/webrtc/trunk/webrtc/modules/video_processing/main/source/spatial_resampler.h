













#ifndef VPM_SPATIAL_RESAMPLER_H
#define VPM_SPATIAL_RESAMPLER_H

#include "typedefs.h"

#include "module_common_types.h"
#include "video_processing_defines.h"

#include "common_video/libyuv/include/webrtc_libyuv.h"
#include "common_video/libyuv/include/scaler.h"

namespace webrtc {

class VPMSpatialResampler
{
public:
  virtual ~VPMSpatialResampler() {};
  virtual int32_t SetTargetFrameSize(int32_t width, int32_t height) = 0;
  virtual void SetInputFrameResampleMode(VideoFrameResampling
                                         resamplingMode) = 0;
  virtual void Reset() = 0;
  virtual int32_t ResampleFrame(const I420VideoFrame& inFrame,
                                I420VideoFrame* outFrame) = 0;
  virtual int32_t TargetWidth() = 0;
  virtual int32_t TargetHeight() = 0;
  virtual bool ApplyResample(int32_t width, int32_t height) = 0;
};

class VPMSimpleSpatialResampler : public VPMSpatialResampler
{
public:
  VPMSimpleSpatialResampler();
  ~VPMSimpleSpatialResampler();
  virtual int32_t SetTargetFrameSize(int32_t width, int32_t height);
  virtual void SetInputFrameResampleMode(VideoFrameResampling resamplingMode);
  virtual void Reset();
  virtual int32_t ResampleFrame(const I420VideoFrame& inFrame,
                                I420VideoFrame* outFrame);
  virtual int32_t TargetWidth();
  virtual int32_t TargetHeight();
  virtual bool ApplyResample(int32_t width, int32_t height);

private:

  VideoFrameResampling        _resamplingMode;
  int32_t                     _targetWidth;
  int32_t                     _targetHeight;
  Scaler                      _scaler;
};

} 

#endif
