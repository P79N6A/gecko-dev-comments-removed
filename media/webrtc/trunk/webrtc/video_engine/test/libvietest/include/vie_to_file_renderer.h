









#ifndef WEBRTC_VIDEO_ENGINE_TEST_LIBVIETEST_INCLUDE_VIE_TO_FILE_RENDERER_H_
#define WEBRTC_VIDEO_ENGINE_TEST_LIBVIETEST_INCLUDE_VIE_TO_FILE_RENDERER_H_

#include <stdio.h>
#include <string.h>

#include <list>
#include <string>

#include "webrtc/system_wrappers/interface/constructor_magic.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"
#include "webrtc/video_engine/include/vie_render.h"

namespace webrtc {
class CriticalSectionWrapper;
class EventWrapper;
class ThreadWrapper;
};  

namespace test {
struct Frame;
};  

class ViEToFileRenderer: public webrtc::ExternalRenderer {
 public:
  ViEToFileRenderer();
  virtual ~ViEToFileRenderer();

  
  bool PrepareForRendering(const std::string& output_path,
                           const std::string& output_filename);

  
  void StopRendering();

  
  
  
  bool DeleteOutputFile();

  
  
  
  bool SaveOutputFile(const std::string& prefix);

  
  int FrameSizeChange(unsigned int width, unsigned int height,
                      unsigned int number_of_streams) OVERRIDE;

  int DeliverFrame(unsigned char* buffer,
                   int buffer_size,
                   uint32_t time_stamp,
                   int64_t render_time,
                   void* handle) OVERRIDE;

  bool IsTextureSupported() OVERRIDE;

  const std::string GetFullOutputPath() const;

 private:
  typedef std::list<test::Frame*> FrameQueue;

  static bool RunRenderThread(void* obj);
  void ForgetOutputFile();
  bool ProcessRenderQueue();

  FILE* output_file_;
  std::string output_path_;
  std::string output_filename_;
  webrtc::scoped_ptr<webrtc::ThreadWrapper> thread_;
  webrtc::scoped_ptr<webrtc::CriticalSectionWrapper> frame_queue_cs_;
  webrtc::scoped_ptr<webrtc::EventWrapper> frame_render_event_;
  FrameQueue render_queue_;
  FrameQueue free_frame_queue_;
};

#endif  
