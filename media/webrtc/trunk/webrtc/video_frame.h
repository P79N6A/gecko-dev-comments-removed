









#ifndef WEBRTC_VIDEO_FRAME_H_
#define WEBRTC_VIDEO_FRAME_H_

#include <assert.h>

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
  
  
  
  
  
  
  virtual int32_t AddRef() {
    assert(false);
    return -1;
  }
  virtual int32_t Release() {
    assert(false);
    return -1;
  }

  
  
  
  
  
  virtual int CreateEmptyFrame(int width,
                               int height,
                               int stride_y,
                               int stride_u,
                               int stride_v);

  
  
  
  virtual int CreateFrame(int size_y,
                          const uint8_t* buffer_y,
                          int size_u,
                          const uint8_t* buffer_u,
                          int size_v,
                          const uint8_t* buffer_v,
                          int width,
                          int height,
                          int stride_y,
                          int stride_u,
                          int stride_v);

  
  
  
  virtual int CopyFrame(const I420VideoFrame& videoFrame);

  
  
  virtual I420VideoFrame* CloneFrame() const;

  
  virtual void SwapFrame(I420VideoFrame* videoFrame);

  
  virtual uint8_t* buffer(PlaneType type);
  
  virtual const uint8_t* buffer(PlaneType type) const;

  
  virtual int allocated_size(PlaneType type) const;

  
  virtual int stride(PlaneType type) const;

  
  virtual int set_width(int width);

  
  virtual int set_height(int height);

  
  virtual int width() const { return width_; }

  
  virtual int height() const { return height_; }

  
  virtual void set_timestamp(uint32_t timestamp) { timestamp_ = timestamp; }

  
  virtual uint32_t timestamp() const { return timestamp_; }

  
  virtual void set_ntp_time_ms(int64_t ntp_time_ms) {
    ntp_time_ms_ = ntp_time_ms;
  }

  
  virtual int64_t ntp_time_ms() const { return ntp_time_ms_; }

  
  virtual void set_render_time_ms(int64_t render_time_ms) {
    render_time_ms_ = render_time_ms;
  }

  
  virtual int64_t render_time_ms() const { return render_time_ms_; }

  
  virtual bool IsZeroSize() const;

  
  
  virtual void ResetSize();

  
  
  
  virtual void* native_handle() const;

 protected:
  
  
  virtual int CheckDimensions(int width,
                              int height,
                              int stride_y,
                              int stride_u,
                              int stride_v);

 private:
  
  const Plane* GetPlane(PlaneType type) const;
  
  Plane* GetPlane(PlaneType type);

  Plane y_plane_;
  Plane u_plane_;
  Plane v_plane_;
  int width_;
  int height_;
  uint32_t timestamp_;
  int64_t ntp_time_ms_;
  int64_t render_time_ms_;
};

enum VideoFrameType {
  kKeyFrame = 0,
  kDeltaFrame = 1,
  kGoldenFrame = 2,
  kAltRefFrame = 3,
  kSkipFrame = 4
};


class EncodedImage {
 public:
  EncodedImage()
      : _encodedWidth(0),
        _encodedHeight(0),
        _timeStamp(0),
        capture_time_ms_(0),
        _frameType(kDeltaFrame),
        _buffer(NULL),
        _length(0),
        _size(0),
        _completeFrame(false) {}

  EncodedImage(uint8_t* buffer, uint32_t length, uint32_t size)
      : _encodedWidth(0),
        _encodedHeight(0),
        _timeStamp(0),
        ntp_time_ms_(0),
        capture_time_ms_(0),
        _frameType(kDeltaFrame),
        _buffer(buffer),
        _length(length),
        _size(size),
        _completeFrame(false) {}

  uint32_t _encodedWidth;
  uint32_t _encodedHeight;
  uint32_t _timeStamp;
  
  int64_t ntp_time_ms_;
  int64_t capture_time_ms_;
  VideoFrameType _frameType;
  uint8_t* _buffer;
  uint32_t _length;
  uint32_t _size;
  bool _completeFrame;
};

}  
#endif  

