














#ifndef WEBRTC_VIDEO_ENGINE_INCLUDE_VIE_RENDER_H_
#define WEBRTC_VIDEO_ENGINE_INCLUDE_VIE_RENDER_H_

#include "common_types.h"

namespace webrtc {

class VideoEngine;
class VideoRender;



class WEBRTC_DLLEXPORT ExternalRenderer {
 public:
  
  
  virtual int FrameSizeChange(unsigned int width,
                              unsigned int height,
                              unsigned int number_of_streams) = 0;

  
  virtual int DeliverFrame(unsigned char* buffer,
                           int buffer_size,
                           
                           uint32_t time_stamp,
                           
                           int64_t render_time) = 0;

 protected:
  virtual ~ExternalRenderer() {}
};

class WEBRTC_DLLEXPORT ViERender {
 public:
  
  
  
  static ViERender* GetInterface(VideoEngine* video_engine);

  
  
  
  virtual int Release() = 0;

  
  virtual int RegisterVideoRenderModule(VideoRender& render_module) = 0;

  
  virtual int DeRegisterVideoRenderModule(VideoRender& render_module) = 0;

  
  virtual int AddRenderer(const int render_id,
                          void* window,
                          const unsigned int z_order,
                          const float left,
                          const float top,
                          const float right,
                          const float bottom) = 0;

  
  virtual int RemoveRenderer(const int render_id) = 0;

  
  virtual int StartRender(const int render_id) = 0;

  
  virtual int StopRender(const int render_id) = 0;

  
  
  
  virtual int SetExpectedRenderDelay(int render_id, int render_delay) = 0;

  
  virtual int ConfigureRender(int render_id,
                              const unsigned int z_order,
                              const float left,
                              const float top,
                              const float right,
                              const float bottom) = 0;

  
  virtual int MirrorRenderStream(const int render_id,
                                 const bool enable,
                                 const bool mirror_xaxis,
                                 const bool mirror_yaxis) = 0;

  
  virtual int AddRenderer(const int render_id,
                          RawVideoType video_input_format,
                          ExternalRenderer* renderer) = 0;

 protected:
  ViERender() {}
  virtual ~ViERender() {}
};

}  

#endif  
