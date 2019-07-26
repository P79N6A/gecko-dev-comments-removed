









#ifndef WEBRTC_MODULES_DESKTOP_CAPTURE_DIFFER_H_
#define WEBRTC_MODULES_DESKTOP_CAPTURE_DIFFER_H_

#include <vector>

#include "webrtc/modules/desktop_capture/desktop_region.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"

namespace webrtc {

typedef uint8_t DiffInfo;







class Differ {
 public:
  
  
  Differ(int width, int height, int bytes_per_pixel, int stride);
  ~Differ();

  int width() { return width_; }
  int height() { return height_; }
  int bytes_per_pixel() { return bytes_per_pixel_; }
  int bytes_per_row() { return bytes_per_row_; }

  
  
  void CalcDirtyRegion(const void* prev_buffer, const void* curr_buffer,
                       DesktopRegion* region);

 private:
  
  friend class DifferTest;

  
  void MarkDirtyBlocks(const void* prev_buffer, const void* curr_buffer);

  
  
  
  void MergeBlocks(DesktopRegion* region);

  
  
  
  
  DiffInfo DiffPartialBlock(const uint8_t* prev_buffer,
                            const uint8_t* curr_buffer,
                            int stride,
                            int width, int height);

  
  int width_;
  int height_;

  
  
  int bytes_per_pixel_;

  
  int bytes_per_row_;

  
  scoped_array<DiffInfo> diff_info_;

  
  int diff_info_width_;
  int diff_info_height_;
  int diff_info_size_;

  DISALLOW_COPY_AND_ASSIGN(Differ);
};

}  

#endif  
