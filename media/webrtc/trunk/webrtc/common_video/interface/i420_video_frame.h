









#ifndef COMMON_VIDEO_INTERFACE_I420_VIDEO_FRAME_H
#define COMMON_VIDEO_INTERFACE_I420_VIDEO_FRAME_H





#include "webrtc/common_video/plane.h"
#include "webrtc/system_wrappers/interface/scoped_refptr.h"
#include "webrtc/typedefs.h"





namespace webrtc {

enum PlaneType {
  kYPlane = 0,
  kUPlane = 1,
  kVPlane = 2,
  kNumOfPlanes = 3
};

class I420VideoFrame {
 public:
  I420VideoFrame();
  virtual ~I420VideoFrame();
  
  
  
  
  
  
  virtual int32_t AddRef() {assert(false); return -1;}
  virtual int32_t Release() {assert(false); return -1;}

  
  
  
  
  
  virtual int CreateEmptyFrame(int width, int height,
                               int stride_y, int stride_u, int stride_v);

  
  
  
  virtual int CreateFrame(int size_y, const uint8_t* buffer_y,
                          int size_u, const uint8_t* buffer_u,
                          int size_v, const uint8_t* buffer_v,
                          int width, int height,
                          int stride_y, int stride_u, int stride_v);

  
  
  
  virtual int CopyFrame(const I420VideoFrame& videoFrame);

  
  virtual void SwapFrame(I420VideoFrame* videoFrame);

  
  virtual uint8_t* buffer(PlaneType type);
  
  virtual const uint8_t* buffer(PlaneType type) const;

  
  virtual int allocated_size(PlaneType type) const;

  
  virtual int stride(PlaneType type) const;

  
  virtual int set_width(int width);

  
  virtual int set_height(int height);

  
  virtual int width() const {return width_;}

  
  virtual int height() const {return height_;}

  
  virtual void set_timestamp(uint32_t timestamp) {timestamp_ = timestamp;}

  
  virtual uint32_t timestamp() const {return timestamp_;}

  
  virtual void set_render_time_ms(int64_t render_time_ms) {render_time_ms_ =
                                                   render_time_ms;}

  
  virtual int64_t render_time_ms() const {return render_time_ms_;}

  
  virtual bool IsZeroSize() const;

  
  
  virtual void ResetSize();

  
  
  
  virtual void* native_handle() const;

 protected:
  
  
  virtual int CheckDimensions(int width, int height,
                              int stride_y, int stride_u, int stride_v);

 private:
  
  const Plane* GetPlane(PlaneType type) const;
  
  Plane* GetPlane(PlaneType type);

  Plane y_plane_;
  Plane u_plane_;
  Plane v_plane_;
  int width_;
  int height_;
  uint32_t timestamp_;
  int64_t render_time_ms_;
};  

}  

#endif  
