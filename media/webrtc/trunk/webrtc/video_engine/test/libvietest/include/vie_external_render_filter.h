








#ifndef WEBRTC_VIDEO_ENGINE_TEST_LIBVIETEST_INCLUDE_VIE_EXTERNAL_RENDER_H_
#define WEBRTC_VIDEO_ENGINE_TEST_LIBVIETEST_INCLUDE_VIE_EXTERNAL_RENDER_H_

#include "webrtc/system_wrappers/interface/tick_util.h"
#include "webrtc/video_engine/include/vie_render.h"

namespace webrtc {





class ExternalRendererEffectFilter : public webrtc::ViEEffectFilter {
 public:
  explicit ExternalRendererEffectFilter(webrtc::ExternalRenderer* renderer)
      : width_(0), height_(0), renderer_(renderer) {}
  virtual ~ExternalRendererEffectFilter() {}
  virtual int Transform(int size, unsigned char* frame_buffer,
                        unsigned int time_stamp90KHz, unsigned int width,
                        unsigned int height) {
    if (width != width_ || height_ != height) {
      renderer_->FrameSizeChange(width, height, 1);
      width_ = width;
      height_ = height;
    }
    return renderer_->DeliverFrame(frame_buffer,
                                   size,
                                   time_stamp90KHz,
                                   webrtc::TickTime::MillisecondTimestamp());
  }

 private:
  unsigned int width_;
  unsigned int height_;
  webrtc::ExternalRenderer* renderer_;
};

}  

#endif  
