









#include "webrtc/common_video/interface/texture_video_frame.h"

#include <assert.h>

namespace webrtc {

TextureVideoFrame::TextureVideoFrame(NativeHandle* handle,
                                     int width,
                                     int height,
                                     uint32_t timestamp,
                                     int64_t render_time_ms)
    : handle_(handle) {
  set_width(width);
  set_height(height);
  set_timestamp(timestamp);
  set_render_time_ms(render_time_ms);
}

TextureVideoFrame::~TextureVideoFrame() {}

int TextureVideoFrame::CreateEmptyFrame(int width,
                                        int height,
                                        int stride_y,
                                        int stride_u,
                                        int stride_v) {
  assert(false);  
  return -1;
}

int TextureVideoFrame::CreateFrame(int size_y,
                                   const uint8_t* buffer_y,
                                   int size_u,
                                   const uint8_t* buffer_u,
                                   int size_v,
                                   const uint8_t* buffer_v,
                                   int width,
                                   int height,
                                   int stride_y,
                                   int stride_u,
                                   int stride_v) {
  assert(false);  
  return -1;
}

int TextureVideoFrame::CopyFrame(const I420VideoFrame& videoFrame) {
  assert(false);  
  return -1;
}

I420VideoFrame* TextureVideoFrame::CloneFrame() const {
  return new TextureVideoFrame(
      handle_, width(), height(), timestamp(), render_time_ms());
}

void TextureVideoFrame::SwapFrame(I420VideoFrame* videoFrame) {
  assert(false);  
}

uint8_t* TextureVideoFrame::buffer(PlaneType type) {
  assert(false);  
  return NULL;
}

const uint8_t* TextureVideoFrame::buffer(PlaneType type) const {
  assert(false);  
  return NULL;
}

int TextureVideoFrame::allocated_size(PlaneType type) const {
  assert(false);  
  return -1;
}

int TextureVideoFrame::stride(PlaneType type) const {
  assert(false);  
  return -1;
}

bool TextureVideoFrame::IsZeroSize() const {
  assert(false);  
  return true;
}

void TextureVideoFrame::ResetSize() {
  assert(false);  
}

void* TextureVideoFrame::native_handle() const { return handle_.get(); }

int TextureVideoFrame::CheckDimensions(
    int width, int height, int stride_y, int stride_u, int stride_v) {
  return 0;
}

}  
