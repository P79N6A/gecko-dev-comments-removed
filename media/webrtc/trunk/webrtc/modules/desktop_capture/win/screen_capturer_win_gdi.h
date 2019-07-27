









#ifndef WEBRTC_MODULES_DESKTOP_CAPTURE_WIN_SCREEN_CAPTURER_WIN_GDI_H_
#define WEBRTC_MODULES_DESKTOP_CAPTURE_WIN_SCREEN_CAPTURER_WIN_GDI_H_

#include "webrtc/modules/desktop_capture/screen_capturer.h"

#include <windows.h>

#include "webrtc/modules/desktop_capture/screen_capture_frame_queue.h"
#include "webrtc/modules/desktop_capture/screen_capturer_helper.h"
#include "webrtc/modules/desktop_capture/win/scoped_thread_desktop.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"

namespace webrtc {

class Differ;




class ScreenCapturerWinGdi : public ScreenCapturer {
 public:
  explicit ScreenCapturerWinGdi(const DesktopCaptureOptions& options);
  virtual ~ScreenCapturerWinGdi();

  
  virtual void Start(Callback* callback) OVERRIDE;
  virtual void Capture(const DesktopRegion& region) OVERRIDE;
  virtual bool GetScreenList(ScreenList* screens) OVERRIDE;
  virtual bool SelectScreen(ScreenId id) OVERRIDE;

 private:
  typedef HRESULT (WINAPI * DwmEnableCompositionFunc)(UINT);

  
  void PrepareCaptureResources();

  
  
  bool CaptureImage();

  
  void CaptureCursor();

  Callback* callback_;
  ScreenId current_screen_id_;
  std::wstring current_device_key_;

  
  
  ScreenCapturerHelper helper_;

  ScopedThreadDesktop desktop_;

  
  HDC desktop_dc_;
  HDC memory_dc_;

  
  ScreenCaptureFrameQueue queue_;

  
  
  DesktopRect desktop_dc_rect_;

  
  scoped_ptr<Differ> differ_;

  HMODULE dwmapi_library_;
  DwmEnableCompositionFunc composition_func_;

  
  bool set_thread_execution_state_failed_;

  DISALLOW_COPY_AND_ASSIGN(ScreenCapturerWinGdi);
};

}  

#endif  
