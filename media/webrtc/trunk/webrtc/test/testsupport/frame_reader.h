









#ifndef WEBRTC_TEST_TESTSUPPORT_FRAME_READER_H_
#define WEBRTC_TEST_TESTSUPPORT_FRAME_READER_H_

#include <cstdio>
#include <string>

#include "typedefs.h"

namespace webrtc {
namespace test {


class FrameReader {
 public:
  virtual ~FrameReader() {}

  
  
  
  virtual bool Init() = 0;

  
  
  
  
  virtual bool ReadFrame(WebRtc_UWord8* source_buffer) = 0;

  
  
  virtual void Close() = 0;

  
  virtual int FrameLength() = 0;
  
  virtual int NumberOfFrames() = 0;
};

class FrameReaderImpl : public FrameReader {
 public:
  
  
  
  
  
  FrameReaderImpl(std::string input_filename, int frame_length_in_bytes);
  virtual ~FrameReaderImpl();
  bool Init();
  bool ReadFrame(WebRtc_UWord8* source_buffer);
  void Close();
  int FrameLength() { return frame_length_in_bytes_; }
  int NumberOfFrames() { return number_of_frames_; }

 private:
  std::string input_filename_;
  int frame_length_in_bytes_;
  int number_of_frames_;
  FILE* input_file_;
};

}  
}  

#endif  
