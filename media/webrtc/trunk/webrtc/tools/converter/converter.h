









#ifndef WEBRTC_TOOLS_CONVERTER_CONVERTER_H_
#define WEBRTC_TOOLS_CONVERTER_CONVERTER_H_

#include <string>

#include "libyuv/convert.h"
#include "libyuv/compare.h"

namespace webrtc {
namespace test {


class Converter {
 public:
  Converter(int width, int height);

  
  
  bool ConvertRGBAToI420Video(std::string frames_dir,
                              std::string output_file_name, bool delete_frames);

 private:
  int width_;  
  int height_;  

  
  int YPlaneSize() const {
    return width_*height_;
  }

  
  int UPlaneSize() const {
    return ((width_+1)/2)*((height_)/2);
  }

  
  int VPlaneSize() const {
    return ((width_+1)/2)*((height_)/2);
  }

  
  int SrcStrideFrame() const {
    return width_*4;
  }

  
  int DstStrideY() const {
    return width_;
  }

  
  int DstStrideU() const {
    return (width_+1)/2;
  }

  
  int DstStrideV() const {
    return (width_+1)/2;
  }

  
  int InputFrameSize() const {
    return width_*height_*4;
  }

  
  
  bool AddYUVToFile(uint8* y_plane, int y_plane_size,
                    uint8* u_plane, int u_plane_size,
                    uint8* v_plane, int v_plane_size,
                    FILE* output_file);

  
  bool AddYUVPlaneToFile(uint8* yuv_plane, int yuv_plane_size, FILE* file);

  
  
  bool ReadRGBAFrame(const char* input_file_name, int input_frame_size,
                     unsigned char* buffer);

  
  
  std::string FindFullFileName(std::string dir_name, std::string file_name);

  
  bool FileExists(std::string file_name_to_check);

  
    
  std::string FormFrameName(int width, int number);
};

}  
}  

#endif  
