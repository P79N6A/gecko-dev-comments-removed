









#ifndef WEBRTC_VIDEO_ENGINE_TEST_COMMON_MAC_VIDEO_RENDERER_MAC_H_
#define WEBRTC_VIDEO_ENGINE_TEST_COMMON_MAC_VIDEO_RENDERER_MAC_H_

#include "webrtc/system_wrappers/interface/constructor_magic.h"
#include "webrtc/video_engine/test/common/gl/gl_renderer.h"

@class CocoaWindow;

namespace webrtc {
namespace test {

class MacRenderer : public GlRenderer {
 public:
  MacRenderer();
  virtual ~MacRenderer();

  bool Init(const char* window_title, int width, int height);

  
  virtual void RenderFrame(const I420VideoFrame& frame, int delta) OVERRIDE;

 private:
  CocoaWindow* window_;

  DISALLOW_COPY_AND_ASSIGN(MacRenderer);
};
}  
}  

#endif  
