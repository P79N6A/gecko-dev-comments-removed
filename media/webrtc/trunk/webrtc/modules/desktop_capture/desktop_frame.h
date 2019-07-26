









#ifndef WEBRTC_MODULES_DESKTOP_CAPTURE_DESKTOP_FRAME_H_
#define WEBRTC_MODULES_DESKTOP_CAPTURE_DESKTOP_FRAME_H_

#include "webrtc/modules/desktop_capture/desktop_geometry.h"
#include "webrtc/modules/desktop_capture/desktop_region.h"
#include "webrtc/modules/desktop_capture/shared_memory.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"
#include "webrtc/typedefs.h"

namespace webrtc {


class DesktopFrame {
 public:
  
  static const int kBytesPerPixel = 4;

  virtual ~DesktopFrame();

  
  const DesktopSize& size() const { return size_; }

  
  int stride() const { return stride_; }

  
  uint8_t* data() const { return data_; }

  
  
  
  SharedMemory* shared_memory() const { return shared_memory_; }

  
  const DesktopRegion& updated_region() const { return updated_region_; }
  DesktopRegion* mutable_updated_region() { return &updated_region_; }

  
  
  const DesktopVector& dpi() const { return dpi_; }
  void set_dpi(const DesktopVector& dpi) { dpi_ = dpi; }

  
  int32_t capture_time_ms() const { return capture_time_ms_; }
  void set_capture_time_ms(int32_t time_ms) { capture_time_ms_ = time_ms; }

  
  
  const DesktopRegion* shape() const { return shape_.get(); }
  void set_shape(DesktopRegion* shape) { shape_.reset(shape); }

  
  
  void CopyPixelsFrom(uint8_t* src_buffer, int src_stride,
                      const DesktopRect& dest_rect);
  void CopyPixelsFrom(const DesktopFrame& src_frame,
                      const DesktopVector& src_pos,
                      const DesktopRect& dest_rect);

 protected:
  DesktopFrame(DesktopSize size,
               int stride,
               uint8_t* data,
               SharedMemory* shared_memory);

  const DesktopSize size_;
  const int stride_;

  
  
  
  uint8_t* const data_;
  SharedMemory* const shared_memory_;

  DesktopRegion updated_region_;
  DesktopVector dpi_;
  int32_t capture_time_ms_;
  scoped_ptr<DesktopRegion> shape_;

 private:
  DISALLOW_COPY_AND_ASSIGN(DesktopFrame);
};


class BasicDesktopFrame : public DesktopFrame {
 public:
  explicit BasicDesktopFrame(DesktopSize size);
  virtual ~BasicDesktopFrame();

  
  static DesktopFrame* CopyOf(const DesktopFrame& frame);

 private:
  DISALLOW_COPY_AND_ASSIGN(BasicDesktopFrame);
};


class SharedMemoryDesktopFrame : public DesktopFrame {
 public:
  
  SharedMemoryDesktopFrame(DesktopSize size,
                           int stride,
                           SharedMemory* shared_memory);
  virtual ~SharedMemoryDesktopFrame();

 private:
  DISALLOW_COPY_AND_ASSIGN(SharedMemoryDesktopFrame);
};

}  

#endif  

