









#include "testsupport/frame_writer.h"

#include <cassert>

namespace webrtc {
namespace test {

FrameWriterImpl::FrameWriterImpl(std::string output_filename,
                                 int frame_length_in_bytes)
    : output_filename_(output_filename),
      frame_length_in_bytes_(frame_length_in_bytes),
      output_file_(NULL) {
}

FrameWriterImpl::~FrameWriterImpl() {
  Close();
}

bool FrameWriterImpl::Init() {
  if (frame_length_in_bytes_ <= 0) {
    fprintf(stderr, "Frame length must be >0, was %d\n",
            frame_length_in_bytes_);
    return false;
  }
  output_file_ = fopen(output_filename_.c_str(), "wb");
  if (output_file_ == NULL) {
    fprintf(stderr, "Couldn't open output file for writing: %s\n",
            output_filename_.c_str());
    return false;
  }
  return true;
}

void FrameWriterImpl::Close() {
  if (output_file_ != NULL) {
    fclose(output_file_);
    output_file_ = NULL;
  }
}

bool FrameWriterImpl::WriteFrame(WebRtc_UWord8* frame_buffer) {
  assert(frame_buffer);
  if (output_file_ == NULL) {
    fprintf(stderr, "FrameWriter is not initialized (output file is NULL)\n");
    return false;
  }
  int bytes_written = fwrite(frame_buffer, 1, frame_length_in_bytes_,
                             output_file_);
  if (bytes_written != frame_length_in_bytes_) {
    fprintf(stderr, "Failed to write %d bytes to file %s\n",
            frame_length_in_bytes_, output_filename_.c_str());
    return false;
  }
  return true;
}

}  
}  
