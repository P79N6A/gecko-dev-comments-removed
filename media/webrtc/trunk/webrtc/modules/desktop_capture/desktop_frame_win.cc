









#include "webrtc/modules/desktop_capture/desktop_frame_win.h"

#include "webrtc/system_wrappers/interface/logging.h"

namespace webrtc {

DesktopFrameWin::DesktopFrameWin(DesktopSize size,
                                 int stride,
                                 uint8_t* data,
                                 SharedMemory* shared_memory,
                                 HBITMAP bitmap)
    : DesktopFrame(size, stride, data, shared_memory),
      bitmap_(bitmap),
      owned_shared_memory_(shared_memory_) {
}

DesktopFrameWin::~DesktopFrameWin() {
  DeleteObject(bitmap_);
}


DesktopFrameWin* DesktopFrameWin::Create(DesktopSize size,
                                         SharedMemory* shared_memory,
                                         HDC hdc) {
  int bytes_per_row = size.width() * kBytesPerPixel;

  
  BITMAPINFO bmi = {0};
  bmi.bmiHeader.biHeight = -size.height();
  bmi.bmiHeader.biWidth = size.width();
  bmi.bmiHeader.biPlanes = 1;
  bmi.bmiHeader.biBitCount = DesktopFrameWin::kBytesPerPixel * 8;
  bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
  bmi.bmiHeader.biSizeImage = bytes_per_row * size.height();

  HANDLE section_handle = NULL;
  if (shared_memory)
    section_handle = shared_memory->handle();
  void* data = NULL;
  HBITMAP bitmap = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, &data,
                                    section_handle, 0);
  if (!bitmap) {
    LOG(LS_WARNING) << "Failed to allocate new window frame " << GetLastError();
    delete shared_memory;
    return NULL;
  }

  return new DesktopFrameWin(size, bytes_per_row,
                             reinterpret_cast<uint8_t*>(data),
                             shared_memory, bitmap);
}

}  
