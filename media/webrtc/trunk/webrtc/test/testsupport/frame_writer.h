









#ifndef WEBRTC_TEST_TESTSUPPORT_FRAME_WRITER_H_
#define WEBRTC_TEST_TESTSUPPORT_FRAME_WRITER_H_

#include <cstdio>
#include <string>

#include "typedefs.h"

namespace webrtc {
namespace test {


class FrameWriter {
 public:
  virtual ~FrameWriter() {}

  
  
  
  virtual bool Init() = 0;

  
  
  virtual bool WriteFrame(WebRtc_UWord8* frame_buffer) = 0;

  
  
  virtual void Close() = 0;

  
  virtual int FrameLength() = 0;
};

class FrameWriterImpl : public FrameWriter {
 public:
  
  
  
  
  
  
  
  FrameWriterImpl(std::string output_filename, int frame_length_in_bytes);
  virtual ~FrameWriterImpl();
  bool Init();
  bool WriteFrame(WebRtc_UWord8* frame_buffer);
  void Close();
  int FrameLength() { return frame_length_in_bytes_; }

 private:
  std::string output_filename_;
  int frame_length_in_bytes_;
  FILE* output_file_;
};

}  
}  

#endif  
