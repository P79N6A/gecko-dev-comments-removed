









#ifndef WEBRTC_VIDEO_ENGINE_VIE_RENDER_MANAGER_H_
#define WEBRTC_VIDEO_ENGINE_VIE_RENDER_MANAGER_H_

#include "system_wrappers/interface/list_wrapper.h"
#include "system_wrappers/interface/map_wrapper.h"
#include "system_wrappers/interface/scoped_ptr.h"
#include "typedefs.h"  

#include "video_engine/vie_manager_base.h"

namespace webrtc {

class CriticalSectionWrapper;
class RWLockWrapper;
class VideoRender;
class VideoRenderCallback;
class ViERenderer;

class ViERenderManager : private ViEManagerBase {
  friend class ViERenderManagerScoped;
 public:
  explicit ViERenderManager(WebRtc_Word32 engine_id);
  ~ViERenderManager();

  WebRtc_Word32 RegisterVideoRenderModule(VideoRender* render_module);
  WebRtc_Word32 DeRegisterVideoRenderModule(VideoRender* render_module);

  ViERenderer* AddRenderStream(const WebRtc_Word32 render_id,
                               void* window,
                               const WebRtc_UWord32 z_order,
                               const float left,
                               const float top,
                               const float right,
                               const float bottom);

  WebRtc_Word32 RemoveRenderStream(WebRtc_Word32 render_id);

 private:
  
  
  VideoRender* FindRenderModule(void* window);

  
  ViERenderer* ViERenderPtr(WebRtc_Word32 render_id) const;

  scoped_ptr<CriticalSectionWrapper> list_cs_;
  WebRtc_Word32 engine_id_;
  MapWrapper stream_to_vie_renderer_;  
  ListWrapper render_list_;
  bool use_external_render_module_;
};

class ViERenderManagerScoped: private ViEManagerScopedBase {
 public:
  explicit ViERenderManagerScoped(const ViERenderManager& vie_render_manager);

  
  ViERenderer* Renderer(WebRtc_Word32 render_id) const;
};

}  

#endif  
