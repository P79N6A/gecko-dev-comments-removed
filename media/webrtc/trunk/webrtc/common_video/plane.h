









#ifndef COMMON_VIDEO_PLANE_H
#define COMMON_VIDEO_PLANE_H

#include "system_wrappers/interface/aligned_malloc.h"
#include "typedefs.h"  

namespace webrtc {



class Plane {
 public:
  Plane();
  ~Plane();
  
  
  
  
  int CreateEmptyPlane(int allocated_size, int stride, int plane_size);

  
  
  int Copy(const Plane& plane);

  
  
  
  int Copy(int size, int stride, const uint8_t* buffer);

  
  void Swap(Plane& plane);

  
  int allocated_size() const {return allocated_size_;}

  
  void ResetSize() {plane_size_ = 0;}

  
  bool IsZeroSize() const {return plane_size_ == 0;}

  
  int stride() const {return stride_;}

  
  const uint8_t* buffer() const {return buffer_.get();}
  
  uint8_t* buffer() {return buffer_.get();}

 private:
  
  
  
  int MaybeResize(int new_size);

  Allocator<uint8_t>::scoped_ptr_aligned buffer_;
  int allocated_size_;
  int plane_size_;
  int stride_;
};  

}  

#endif  
