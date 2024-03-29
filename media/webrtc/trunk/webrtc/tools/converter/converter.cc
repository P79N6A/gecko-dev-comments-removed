








#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#include <iomanip>
#include <sstream>

#include "webrtc/tools/converter/converter.h"

#ifdef WIN32
#define SEPARATOR '\\'
#define STAT _stat
#else
#define SEPARATOR '/'
#define STAT stat
#endif

namespace webrtc {
namespace test {

Converter::Converter(int width, int height)
    : width_(width),
      height_(height) {
}

bool Converter::ConvertRGBAToI420Video(std::string frames_dir,
                                       std::string output_file_name,
                                       bool delete_frames) {
  FILE* output_file = fopen(output_file_name.c_str(), "wb");

  
  if (output_file == NULL) {
    fprintf(stderr, "Couldn't open input file for reading: %s\n",
            output_file_name.c_str());
    return false;
  }

  int input_frame_size = InputFrameSize();
  uint8* rgba_buffer = new uint8[input_frame_size];
  int y_plane_size = YPlaneSize();
  uint8* dst_y = new uint8[y_plane_size];
  int u_plane_size = UPlaneSize();
  uint8* dst_u = new uint8[u_plane_size];
  int v_plane_size = VPlaneSize();
  uint8* dst_v = new uint8[v_plane_size];

  int counter = 0;  
  bool success = false;  

  while (true) {
    std::string file_name = FormFrameName(4, counter);
    
    std::string input_file_name = FindFullFileName(frames_dir, file_name);

    if (FileExists(input_file_name)) {
      ++counter;  
    } else {
      fprintf(stdout, "Reached end of frames list\n");
      break;
    }

    
    ReadRGBAFrame(input_file_name.c_str(), input_frame_size, rgba_buffer);

    
    if (delete_frames) {
      if (remove(input_file_name.c_str()) != 0) {
        fprintf(stderr, "Cannot delete file %s\n", input_file_name.c_str());
      }
    }

    
    libyuv::ABGRToI420(rgba_buffer, SrcStrideFrame(),
                       dst_y, DstStrideY(),
                       dst_u, DstStrideU(),
                       dst_v, DstStrideV(),
                       width_, height_);

    
    success = AddYUVToFile(dst_y, y_plane_size, dst_u, u_plane_size,
                           dst_v, v_plane_size, output_file);


    if (!success) {
      fprintf(stderr, "LibYUV error during RGBA to I420 frame conversion\n");
      break;
    }
  }

  delete[] rgba_buffer;
  delete[] dst_y;
  delete[] dst_u;
  delete[] dst_v;

  fclose(output_file);

  return success;
}

bool Converter::AddYUVToFile(uint8* y_plane, int y_plane_size,
                             uint8* u_plane, int u_plane_size,
                             uint8* v_plane, int v_plane_size,
                             FILE* output_file) {
  bool success = AddYUVPlaneToFile(y_plane, y_plane_size, output_file) &&
                 AddYUVPlaneToFile(u_plane, u_plane_size, output_file) &&
                 AddYUVPlaneToFile(v_plane, v_plane_size, output_file);
  return success;
}

bool Converter::AddYUVPlaneToFile(uint8* yuv_plane, int yuv_plane_size,
                                  FILE* file) {
  size_t bytes_written = fwrite(yuv_plane, 1, yuv_plane_size, file);

  if (bytes_written != static_cast<size_t>(yuv_plane_size)) {
    fprintf(stderr, "Number of bytes written (%d) doesn't match size of y plane"
            " (%d)\n", static_cast<int>(bytes_written), yuv_plane_size);
    return false;
  }
  return true;
}

bool Converter::ReadRGBAFrame(const char* input_file_name, int input_frame_size,
                              unsigned char* buffer) {
  FILE* input_file = fopen(input_file_name, "rb");
  if (input_file == NULL) {
    fprintf(stderr, "Couldn't open input file for reading: %s\n",
            input_file_name);
    return false;
  }

  size_t nbr_read = fread(buffer, 1, input_frame_size, input_file);
  fclose(input_file);

  if (nbr_read != static_cast<size_t>(input_frame_size)) {
    fprintf(stderr, "Error reading from input file: %s\n", input_file_name);
    return false;
  }

  return true;
}

std::string Converter::FindFullFileName(std::string dir_name,
                                        std::string file_name) {
  return dir_name + SEPARATOR + file_name;
}

bool Converter:: FileExists(std::string file_name_to_check) {
  struct STAT file_info;
  int result = STAT(file_name_to_check.c_str(), &file_info);
  return (result == 0);
}

std::string Converter::FormFrameName(int width, int number) {
  std::stringstream tmp;

  
  tmp << std::setfill('0') << std::setw(width) << number;

  return "frame_" + tmp.str();
}

}  
}  
