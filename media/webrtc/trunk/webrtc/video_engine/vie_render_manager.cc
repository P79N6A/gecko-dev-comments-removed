









#include "webrtc/video_engine/vie_render_manager.h"

#include "webrtc/engine_configurations.h"
#include "webrtc/modules/video_render/include/video_render.h"
#include "webrtc/modules/video_render/include/video_render_defines.h"
#include "webrtc/system_wrappers/interface/critical_section_wrapper.h"
#include "webrtc/system_wrappers/interface/rw_lock_wrapper.h"
#include "webrtc/system_wrappers/interface/trace.h"
#include "webrtc/video_engine/vie_defines.h"
#include "webrtc/video_engine/vie_renderer.h"

namespace webrtc {

ViERenderManagerScoped::ViERenderManagerScoped(
    const ViERenderManager& vie_render_manager)
    : ViEManagerScopedBase(vie_render_manager) {
}

ViERenderer* ViERenderManagerScoped::Renderer(int32_t render_id) const {
  return static_cast<const ViERenderManager*>(vie_manager_)->ViERenderPtr(
           render_id);
}

ViERenderManager::ViERenderManager(int32_t engine_id)
    : list_cs_(CriticalSectionWrapper::CreateCriticalSection()),
      engine_id_(engine_id),
      use_external_render_module_(false) {
  WEBRTC_TRACE(webrtc::kTraceMemory, webrtc::kTraceVideo, ViEId(engine_id),
               "ViERenderManager::ViERenderManager(engine_id: %d) - "
               "Constructor", engine_id);
}

ViERenderManager::~ViERenderManager() {
  WEBRTC_TRACE(webrtc::kTraceMemory, webrtc::kTraceVideo, ViEId(engine_id_),
               "ViERenderManager Destructor, engine_id: %d", engine_id_);
  for (RendererMap::iterator it = stream_to_vie_renderer_.begin();
       it != stream_to_vie_renderer_.end();
       ++it) {
    
    RemoveRenderStream(it->first);
  }
}

int32_t ViERenderManager::RegisterVideoRenderModule(
    VideoRender* render_module) {
  
  
  VideoRender* current_module = FindRenderModule(render_module->Window());
  if (current_module) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideo, ViEId(engine_id_),
                 "A module is already registered for this window (window=%p, "
                 "current module=%p, registrant module=%p.",
                 render_module->Window(), current_module, render_module);
    return -1;
  }

  
  render_list_.PushBack(static_cast<void*>(render_module));
  use_external_render_module_ = true;
  return 0;
}

int32_t ViERenderManager::DeRegisterVideoRenderModule(
    VideoRender* render_module) {
  
  uint32_t n_streams = render_module->GetNumIncomingRenderStreams();
  if (n_streams != 0) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideo, ViEId(engine_id_),
                 "There are still %d streams in this module, cannot "
                 "de-register", n_streams);
    return -1;
  }

  
  ListItem* list_item = NULL;
  bool found = false;
  for (list_item = render_list_.First(); list_item != NULL;
       list_item = render_list_.Next(list_item)) {
    if (render_module == static_cast<VideoRender*>(list_item->GetItem())) {
      
      render_list_.Erase(list_item);
      found = true;
      break;
    }
  }
  if (!found) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideo, ViEId(engine_id_),
                 "Module not registered");
    return -1;
  }
  return 0;
}

ViERenderer* ViERenderManager::AddRenderStream(const int32_t render_id,
                                               void* window,
                                               const uint32_t z_order,
                                               const float left,
                                               const float top,
                                               const float right,
                                               const float bottom) {
  CriticalSectionScoped cs(list_cs_.get());

  if (stream_to_vie_renderer_.find(render_id) !=
      stream_to_vie_renderer_.end()) {
    
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideo, ViEId(engine_id_),
                 "Render stream already exists");
    return NULL;
  }

  
  VideoRender* render_module = FindRenderModule(window);
  if (render_module == NULL) {
    
    render_module = VideoRender::CreateVideoRender(ViEModuleId(engine_id_, -1),
                                                  window, false);
    if (!render_module) {
      WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideo, ViEId(engine_id_),
                   "Could not create new render module");
      return NULL;
    }
    render_list_.PushBack(static_cast<void*>(render_module));
  }

  ViERenderer* vie_renderer = ViERenderer::CreateViERenderer(render_id,
                                                             engine_id_,
                                                             *render_module,
                                                             *this, z_order,
                                                             left, top, right,
                                                             bottom);
  if (!vie_renderer) {
    WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideo,
                 ViEId(engine_id_, render_id),
                 "Could not create new render stream");
    return NULL;
  }
  stream_to_vie_renderer_[render_id] = vie_renderer;
  return vie_renderer;
}

int32_t ViERenderManager::RemoveRenderStream(
    const int32_t render_id) {
  
  
  ViEManagerWriteScoped scope(this);
  CriticalSectionScoped cs(list_cs_.get());
  RendererMap::iterator it = stream_to_vie_renderer_.find(render_id);
  if (it == stream_to_vie_renderer_.end()) {
    
    WEBRTC_TRACE(webrtc::kTraceWarning, webrtc::kTraceVideo, ViEId(engine_id_),
                 "No renderer for this stream found, channel_id");
    return 0;
  }

  
  VideoRender& renderer = it->second->RenderModule();

  
  
  delete it->second;

  
  stream_to_vie_renderer_.erase(it);

  
  if (!use_external_render_module_ &&
      renderer.GetNumIncomingRenderStreams() == 0) {
    
    ListItem* list_item = NULL;
    for (list_item = render_list_.First(); list_item != NULL;
         list_item = render_list_.Next(list_item)) {
      if (&renderer == static_cast<VideoRender*>(list_item->GetItem())) {
        
        render_list_.Erase(list_item);
        break;
      }
    }
    
    VideoRender::DestroyVideoRender(&renderer);
  }
  return 0;
}

VideoRender* ViERenderManager::FindRenderModule(void* window) {
  VideoRender* renderer = NULL;
  ListItem* list_item = NULL;
  for (list_item = render_list_.First(); list_item != NULL;
       list_item = render_list_.Next(list_item)) {
    renderer = static_cast<VideoRender*>(list_item->GetItem());
    if (renderer == NULL) {
      break;
    }
    if (renderer->Window() == window) {
      
      break;
    }
    renderer = NULL;
  }
  return renderer;
}

ViERenderer* ViERenderManager::ViERenderPtr(int32_t render_id) const {
  RendererMap::const_iterator it = stream_to_vie_renderer_.find(render_id);
  if (it == stream_to_vie_renderer_.end())
    return NULL;

  return it->second;
}

}  
