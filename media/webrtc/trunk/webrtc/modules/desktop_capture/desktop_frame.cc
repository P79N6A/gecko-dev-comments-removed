









#include "webrtc/modules/desktop_capture/desktop_frame.h"

namespace webrtc {

DesktopFrame::DesktopFrame(DesktopSize size,
                           int stride,
                           uint8_t* data,
                           SharedMemory* shared_memory)
    : size_(size),
      stride_(stride),
      data_(data),
      shared_memory_(shared_memory),
      capture_time_ms_(0) {
}

DesktopFrame::~DesktopFrame() {}

BasicDesktopFrame::BasicDesktopFrame(DesktopSize size)
    : DesktopFrame(size, kBytesPerPixel * size.width(),
                   new uint8_t[kBytesPerPixel * size.width() * size.height()],
                   NULL) {
}

BasicDesktopFrame::~BasicDesktopFrame() {
  delete[] data_;
}

SharedMemoryDesktopFrame::SharedMemoryDesktopFrame(
    DesktopSize size,
    int stride,
    SharedMemory* shared_memory)
    : DesktopFrame(size, stride,
                   reinterpret_cast<uint8_t*>(shared_memory->data()),
                   shared_memory) {
}

SharedMemoryDesktopFrame::~SharedMemoryDesktopFrame() {
  delete shared_memory_;
}

}  
