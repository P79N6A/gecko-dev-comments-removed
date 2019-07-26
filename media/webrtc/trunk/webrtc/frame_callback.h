









#ifndef WEBRTC_VIDEO_ENGINE_NEW_INCLUDE_FRAME_CALLBACK_H_
#define WEBRTC_VIDEO_ENGINE_NEW_INCLUDE_FRAME_CALLBACK_H_

#include <stddef.h>

#include "webrtc/common_types.h"

namespace webrtc {

class I420VideoFrame;

struct EncodedFrame {
 public:
  EncodedFrame() : data_(NULL), length_(0), frame_type_(kFrameEmpty) {}
  EncodedFrame(const uint8_t* data, size_t length, FrameType frame_type)
    : data_(data), length_(length), frame_type_(frame_type) {}

  const uint8_t* data_;
  const size_t length_;
  const FrameType frame_type_;
};

class I420FrameCallback {
 public:
  
  
  virtual void FrameCallback(I420VideoFrame* video_frame) = 0;

 protected:
  virtual ~I420FrameCallback() {}
};

class EncodedFrameObserver {
 public:
  virtual void EncodedFrameCallback(const EncodedFrame& encoded_frame) = 0;

 protected:
  virtual ~EncodedFrameObserver() {}
};

}  

#endif  
