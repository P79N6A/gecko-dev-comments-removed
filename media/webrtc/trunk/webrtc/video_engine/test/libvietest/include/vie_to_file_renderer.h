









#ifndef SRC_VIDEO_ENGINE_TEST_AUTO_TEST_HELPERS_VIE_TO_FILE_RENDERER_H_
#define SRC_VIDEO_ENGINE_TEST_AUTO_TEST_HELPERS_VIE_TO_FILE_RENDERER_H_

#include <cstdio>
#include <string>

#include "video_engine/include/vie_render.h"

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
                      unsigned int number_of_streams);

  int DeliverFrame(unsigned char* buffer,
                   int buffer_size,
                   uint32_t time_stamp,
                   int64_t render_time);

  const std::string GetFullOutputPath() const;

 private:
  void ForgetOutputFile();

  std::FILE* output_file_;
  std::string output_path_;
  std::string output_filename_;
};

#endif  
