









#ifndef COMMON_VIDEO_INTERFACE_I420_VIDEO_FRAME_H
#define COMMON_VIDEO_INTERFACE_I420_VIDEO_FRAME_H





#include "webrtc/common_video/plane.h"
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

  
  
  
  
  
  int CreateEmptyFrame(int width, int height,
                       int stride_y, int stride_u, int stride_v);

  
  
  
  int CreateFrame(int size_y, const uint8_t* buffer_y,
                  int size_u, const uint8_t* buffer_u,
                  int size_v, const uint8_t* buffer_v,
                  int width, int height,
                  int stride_y, int stride_u, int stride_v);

  
  
  
  int CopyFrame(const I420VideoFrame& videoFrame);

  
  void SwapFrame(I420VideoFrame* videoFrame);

  
  uint8_t* buffer(PlaneType type);
  
  const uint8_t* buffer(PlaneType type) const;

  
  int allocated_size(PlaneType type) const;

  
  int stride(PlaneType type) const;

  
  int set_width(int width);

  
  int set_height(int height);

  
  int width() const {return width_;}

  
  int height() const {return height_;}

  
  void set_timestamp(const uint32_t timestamp) {timestamp_ = timestamp;}

  
  uint32_t timestamp() const {return timestamp_;}

  
  void set_render_time_ms(int64_t render_time_ms) {render_time_ms_ =
                                                   render_time_ms;}

  
  int64_t render_time_ms() const {return render_time_ms_;}

  
  bool IsZeroSize() const;

  
  
  void ResetSize();

 private:
  
  
  int CheckDimensions(int width, int height,
                      int stride_y, int stride_u, int stride_v);
  
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

