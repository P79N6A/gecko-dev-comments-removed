









#ifndef WEBRTC_TEST_TESTSUPPORT_FRAME_READER_H_
#define WEBRTC_TEST_TESTSUPPORT_FRAME_READER_H_

#include <stdio.h>

#include <string>

#include "webrtc/typedefs.h"

namespace webrtc {
namespace test {


class FrameReader {
 public:
  virtual ~FrameReader() {}

  
  
  
  virtual bool Init() = 0;

  
  
  
  
  virtual bool ReadFrame(uint8_t* source_buffer) = 0;

  
  
  virtual void Close() = 0;

  
  virtual size_t FrameLength() = 0;
  
  virtual int NumberOfFrames() = 0;
};

class FrameReaderImpl : public FrameReader {
 public:
  
  
  
  
  
  FrameReaderImpl(std::string input_filename, size_t frame_length_in_bytes);
  virtual ~FrameReaderImpl();
  virtual bool Init() OVERRIDE;
  virtual bool ReadFrame(uint8_t* source_buffer) OVERRIDE;
  virtual void Close() OVERRIDE;
  virtual size_t FrameLength() OVERRIDE;
  virtual int NumberOfFrames() OVERRIDE;

 private:
  std::string input_filename_;
  size_t frame_length_in_bytes_;
  int number_of_frames_;
  FILE* input_file_;
};

}  
}  

#endif  
