









#ifndef WEBRTC_VIDEO_ENGINE_NEW_INCLUDE_FRAME_CALLBACK_H_
#define WEBRTC_VIDEO_ENGINE_NEW_INCLUDE_FRAME_CALLBACK_H_

namespace webrtc {

class I420VideoFrame;

struct EncodedFrame;

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
