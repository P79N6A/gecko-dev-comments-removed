









#ifndef WEBRTC_TEST_TESTSUPPORT_FRAME_WRITER_H_
#define WEBRTC_TEST_TESTSUPPORT_FRAME_WRITER_H_

#include <cstdio>
#include <string>

#include "webrtc/typedefs.h"

namespace webrtc {
namespace test {


class FrameWriter {
 public:
  virtual ~FrameWriter() {}

  
  
  
  virtual bool Init() = 0;

  
  
  virtual bool WriteFrame(uint8_t* frame_buffer) = 0;

  
  
  virtual void Close() = 0;

  
  virtual size_t FrameLength() = 0;
};

class FrameWriterImpl : public FrameWriter {
 public:
  
  
  
  
  
  
  
  FrameWriterImpl(std::string output_filename, size_t frame_length_in_bytes);
  virtual ~FrameWriterImpl();
  bool Init();
  bool WriteFrame(uint8_t* frame_buffer);
  void Close();
  size_t FrameLength() { return frame_length_in_bytes_; }

 private:
  std::string output_filename_;
  size_t frame_length_in_bytes_;
  FILE* output_file_;
};

}  
}  

#endif  
