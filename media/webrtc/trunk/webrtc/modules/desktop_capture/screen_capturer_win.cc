









#include "webrtc/modules/desktop_capture/screen_capturer.h"

#include <windows.h>

#include "webrtc/modules/desktop_capture/desktop_capture_options.h"
#include "webrtc/modules/desktop_capture/desktop_frame.h"
#include "webrtc/modules/desktop_capture/desktop_frame_win.h"
#include "webrtc/modules/desktop_capture/desktop_region.h"
#include "webrtc/modules/desktop_capture/differ.h"
#include "webrtc/modules/desktop_capture/mouse_cursor.h"
#include "webrtc/modules/desktop_capture/mouse_cursor_shape.h"
#include "webrtc/modules/desktop_capture/screen_capture_frame_queue.h"
#include "webrtc/modules/desktop_capture/screen_capturer_helper.h"
#include "webrtc/modules/desktop_capture/win/cursor.h"
#include "webrtc/modules/desktop_capture/win/desktop.h"
#include "webrtc/modules/desktop_capture/win/scoped_thread_desktop.h"
#include "webrtc/system_wrappers/interface/logging.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"
#include "webrtc/system_wrappers/interface/tick_util.h"

namespace webrtc {

namespace {


const UINT DWM_EC_DISABLECOMPOSITION = 0;
const UINT DWM_EC_ENABLECOMPOSITION = 1;

typedef HRESULT (WINAPI * DwmEnableCompositionFunc)(UINT);
typedef HRESULT (WINAPI * DwmIsCompositionEnabledFunc)(BOOL*);

const wchar_t kDwmapiLibraryName[] = L"dwmapi.dll";




class ScreenCapturerWin : public ScreenCapturer {
 public:
  ScreenCapturerWin(const DesktopCaptureOptions& options);
  virtual ~ScreenCapturerWin();

  
  virtual void Start(Callback* callback) OVERRIDE;
  virtual void Capture(const DesktopRegion& region) OVERRIDE;
  virtual void SetMouseShapeObserver(
      MouseShapeObserver* mouse_shape_observer) OVERRIDE;
  virtual bool GetScreenList(ScreenList* screens) OVERRIDE;
  virtual bool SelectScreen(ScreenId id) OVERRIDE;

 private:
  
  void PrepareCaptureResources();

  
  
  bool CaptureImage();

  
  void CaptureCursor();

  
  
  DesktopRect GetScreenRect();

  Callback* callback_;
  MouseShapeObserver* mouse_shape_observer_;
  ScreenId current_screen_id_;
  std::wstring current_device_key_;

  
  
  ScreenCapturerHelper helper_;

  
  
  
  MouseCursorShape last_cursor_;

  ScopedThreadDesktop desktop_;

  
  HDC desktop_dc_;
  HDC memory_dc_;

  
  ScreenCaptureFrameQueue queue_;

  
  DesktopRect desktop_dc_rect_;

  
  scoped_ptr<Differ> differ_;

  HMODULE dwmapi_library_;
  DwmEnableCompositionFunc composition_func_;
  DwmIsCompositionEnabledFunc composition_enabled_func_;

  bool disable_composition_;

  
  bool set_thread_execution_state_failed_;

  DISALLOW_COPY_AND_ASSIGN(ScreenCapturerWin);
};

ScreenCapturerWin::ScreenCapturerWin(const DesktopCaptureOptions& options)
    : callback_(NULL),
      mouse_shape_observer_(NULL),
      current_screen_id_(kFullDesktopScreenId),
      desktop_dc_(NULL),
      memory_dc_(NULL),
      dwmapi_library_(NULL),
      composition_func_(NULL),
      set_thread_execution_state_failed_(false) {
  
  if (!dwmapi_library_)
    dwmapi_library_ = LoadLibrary(kDwmapiLibraryName);

  if (dwmapi_library_) {
    composition_func_ = reinterpret_cast<DwmEnableCompositionFunc>(
      GetProcAddress(dwmapi_library_, "DwmEnableComposition"));
    composition_enabled_func_ = reinterpret_cast<DwmIsCompositionEnabledFunc>
      (GetProcAddress(dwmapi_library_, "DwmIsCompositionEnabled"));
  }

  disable_composition_ = options.disable_effects();
}

ScreenCapturerWin::~ScreenCapturerWin() {
  if (desktop_dc_)
    ReleaseDC(NULL, desktop_dc_);
  if (memory_dc_)
    DeleteDC(memory_dc_);

  if (disable_composition_) {
    
    if (composition_func_)
      (*composition_func_)(DWM_EC_ENABLECOMPOSITION);
  }

  if (dwmapi_library_)
    FreeLibrary(dwmapi_library_);
}

void ScreenCapturerWin::Capture(const DesktopRegion& region) {
  TickTime capture_start_time = TickTime::Now();

  queue_.MoveToNextFrame();

  
  if (!SetThreadExecutionState(ES_DISPLAY_REQUIRED | ES_SYSTEM_REQUIRED)) {
    if (!set_thread_execution_state_failed_) {
      set_thread_execution_state_failed_ = true;
      LOG_F(LS_WARNING) << "Failed to make system & display power assertion: "
                        << GetLastError();
    }
  }

  
  PrepareCaptureResources();

  
  if (!CaptureImage()) {
    callback_->OnCaptureCompleted(NULL);
    return;
  }

  const DesktopFrame* current_frame = queue_.current_frame();
  const DesktopFrame* last_frame = queue_.previous_frame();
  if (last_frame && last_frame->size().equals(current_frame->size())) {
    
    
    if (!differ_.get() ||
        (differ_->width() != current_frame->size().width()) ||
        (differ_->height() != current_frame->size().height()) ||
        (differ_->bytes_per_row() != current_frame->stride())) {
      differ_.reset(new Differ(current_frame->size().width(),
                               current_frame->size().height(),
                               DesktopFrame::kBytesPerPixel,
                               current_frame->stride()));
    }

    
    DesktopRegion region;
    differ_->CalcDirtyRegion(last_frame->data(), current_frame->data(),
                             &region);
    helper_.InvalidateRegion(region);
  } else {
    
    
    helper_.InvalidateScreen(current_frame->size());
  }

  helper_.set_size_most_recent(current_frame->size());

  
  DesktopFrame* frame = queue_.current_frame()->Share();
  frame->set_dpi(DesktopVector(
      GetDeviceCaps(desktop_dc_, LOGPIXELSX),
      GetDeviceCaps(desktop_dc_, LOGPIXELSY)));
  frame->mutable_updated_region()->Clear();
  helper_.TakeInvalidRegion(frame->mutable_updated_region());
  frame->set_capture_time_ms(
      (TickTime::Now() - capture_start_time).Milliseconds());
  callback_->OnCaptureCompleted(frame);

  
  CaptureCursor();
}

void ScreenCapturerWin::SetMouseShapeObserver(
      MouseShapeObserver* mouse_shape_observer) {
  assert(!mouse_shape_observer_);
  assert(mouse_shape_observer);

  mouse_shape_observer_ = mouse_shape_observer;
}

bool ScreenCapturerWin::GetScreenList(ScreenList* screens) {
  assert(screens->size() == 0);
  BOOL enum_result = TRUE;
  for (int device_index = 0; ; ++device_index) {
    DISPLAY_DEVICE device;
    device.cb = sizeof(device);
    enum_result = EnumDisplayDevices(NULL, device_index, &device, 0);
    
    if (!enum_result)
      break;

    
    if (!(device.StateFlags & DISPLAY_DEVICE_ACTIVE))
      continue;
    Screen screen;
    screen.id = device_index;
    screens->push_back(screen);
  }
  return true;
}

bool ScreenCapturerWin::SelectScreen(ScreenId id) {
  if (id == kFullDesktopScreenId) {
    current_screen_id_ = id;
    return true;
  }
  DISPLAY_DEVICE device;
  device.cb = sizeof(device);
  BOOL enum_result = EnumDisplayDevices(NULL, id, &device, 0);
  if (!enum_result)
    return false;

  current_device_key_ = device.DeviceKey;
  current_screen_id_ = id;
  return true;
}

void ScreenCapturerWin::Start(Callback* callback) {
  assert(!callback_);
  assert(callback);

  callback_ = callback;

  if (disable_composition_) {
    
    
    
    if (composition_func_)
      (*composition_func_)(DWM_EC_DISABLECOMPOSITION);
  }
}

void ScreenCapturerWin::PrepareCaptureResources() {
  
  
  scoped_ptr<Desktop> input_desktop(Desktop::GetInputDesktop());
  if (input_desktop.get() != NULL && !desktop_.IsSame(*input_desktop)) {
    
    if (desktop_dc_) {
      ReleaseDC(NULL, desktop_dc_);
      desktop_dc_ = NULL;
    }

    if (memory_dc_) {
      DeleteDC(memory_dc_);
      memory_dc_ = NULL;
    }

    
    
    desktop_.SetThreadDesktop(input_desktop.release());

    if (disable_composition_) {
      
      
      if (composition_func_ != NULL) {
        (*composition_func_)(DWM_EC_DISABLECOMPOSITION);
      }
    }
  }

  
  
  DesktopRect screen_rect(DesktopRect::MakeXYWH(
      GetSystemMetrics(SM_XVIRTUALSCREEN),
      GetSystemMetrics(SM_YVIRTUALSCREEN),
      GetSystemMetrics(SM_CXVIRTUALSCREEN),
      GetSystemMetrics(SM_CYVIRTUALSCREEN)));
  if (!screen_rect.equals(desktop_dc_rect_)) {
    if (desktop_dc_) {
      ReleaseDC(NULL, desktop_dc_);
      desktop_dc_ = NULL;
    }
    if (memory_dc_) {
      DeleteDC(memory_dc_);
      memory_dc_ = NULL;
    }
    desktop_dc_rect_ = DesktopRect();
  }

  if (desktop_dc_ == NULL) {
    assert(memory_dc_ == NULL);

    
    desktop_dc_ = GetDC(NULL);
    if (!desktop_dc_)
      abort();
    memory_dc_ = CreateCompatibleDC(desktop_dc_);
    if (!memory_dc_)
      abort();
    desktop_dc_rect_ = screen_rect;

    
    queue_.Reset();

    helper_.ClearInvalidRegion();
  }
}

bool ScreenCapturerWin::CaptureImage() {
  DesktopRect screen_rect = GetScreenRect();
  if (screen_rect.is_empty())
    return false;
  DesktopSize size = screen_rect.size();
  
  
  
  if (!queue_.current_frame() ||
      !queue_.current_frame()->size().equals(size)) {
    assert(desktop_dc_ != NULL);
    assert(memory_dc_ != NULL);

    size_t buffer_size = size.width() * size.height() *
        DesktopFrame::kBytesPerPixel;
    SharedMemory* shared_memory =
        callback_->CreateSharedMemory(buffer_size);
    scoped_ptr<DesktopFrameWin> buffer(
        DesktopFrameWin::Create(size, shared_memory, desktop_dc_));
    queue_.ReplaceCurrentFrame(buffer.release());
  }

  
  
  DesktopFrameWin* current = static_cast<DesktopFrameWin*>(
      queue_.current_frame()->GetUnderlyingFrame());
  HGDIOBJ previous_object = SelectObject(memory_dc_, current->bitmap());
  DWORD rop = SRCCOPY;
  if (composition_enabled_func_) {
    BOOL enabled;
    (*composition_enabled_func_)(&enabled);
    if (!enabled) {
      
      rop |= CAPTUREBLT;
    }
  } else {
    
    rop |= CAPTUREBLT;
  }
  if (previous_object != NULL) {
    BitBlt(memory_dc_,
           0, 0, screen_rect.width(), screen_rect.height(),
           desktop_dc_,
           screen_rect.left(), screen_rect.top(),
           rop);

    
    
    SelectObject(memory_dc_, previous_object);
  }
  return true;
}

void ScreenCapturerWin::CaptureCursor() {
  CURSORINFO cursor_info;
  cursor_info.cbSize = sizeof(CURSORINFO);
  if (!GetCursorInfo(&cursor_info)) {
    LOG_F(LS_ERROR) << "Unable to get cursor info. Error = " << GetLastError();
    return;
  }

  
  scoped_ptr<MouseCursor> cursor_image(
      CreateMouseCursorFromHCursor(desktop_dc_, cursor_info.hCursor));
  if (!cursor_image.get())
    return;

  scoped_ptr<MouseCursorShape> cursor(new MouseCursorShape);
  cursor->hotspot = cursor_image->hotspot();
  cursor->size = cursor_image->image()->size();
  uint8_t* current_row = cursor_image->image()->data();
  for (int y = 0; y < cursor_image->image()->size().height(); ++y) {
    cursor->data.append(current_row,
                        current_row + cursor_image->image()->size().width() *
                                        DesktopFrame::kBytesPerPixel);
    current_row += cursor_image->image()->stride();
  }

  
  
  if (last_cursor_.size.equals(cursor->size) &&
      last_cursor_.hotspot.equals(cursor->hotspot) &&
      last_cursor_.data == cursor->data) {
    return;
  }

  LOG(LS_VERBOSE) << "Sending updated cursor: " << cursor->size.width() << "x"
                  << cursor->size.height();

  
  last_cursor_ = *cursor;

  if (mouse_shape_observer_)
    mouse_shape_observer_->OnCursorShapeChanged(cursor.release());
}

DesktopRect ScreenCapturerWin::GetScreenRect() {
  DesktopRect rect = desktop_dc_rect_;
  if (current_screen_id_ == kFullDesktopScreenId)
    return rect;

  DISPLAY_DEVICE device;
  device.cb = sizeof(device);
  BOOL result = EnumDisplayDevices(NULL, current_screen_id_, &device, 0);
  if (!result)
    return DesktopRect();

  
  
  
  if (current_device_key_ != device.DeviceKey)
    return DesktopRect();

  DEVMODE device_mode;
  device_mode.dmSize = sizeof(device_mode);
  device_mode.dmDriverExtra = 0;
  result = EnumDisplaySettingsEx(
      device.DeviceName, ENUM_CURRENT_SETTINGS, &device_mode, 0);
  if (!result)
    return DesktopRect();

  rect = DesktopRect::MakeXYWH(
      rect.left() + device_mode.dmPosition.x,
      rect.top() + device_mode.dmPosition.y,
      device_mode.dmPelsWidth,
      device_mode.dmPelsHeight);
  return rect;
}
}  


ScreenCapturer* ScreenCapturer::Create(const DesktopCaptureOptions& options) {
  return new ScreenCapturerWin(options);
}

}  
