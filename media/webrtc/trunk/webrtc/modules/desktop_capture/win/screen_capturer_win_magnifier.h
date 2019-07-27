









#ifndef WEBRTC_MODULES_DESKTOP_CAPTURE_WIN_SCREEN_CAPTURER_WIN_MAGNIFIER_H_
#define WEBRTC_MODULES_DESKTOP_CAPTURE_WIN_SCREEN_CAPTURER_WIN_MAGNIFIER_H_

#include <windows.h>
#include <magnification.h>
#include <wincodec.h>

#include "webrtc/base/constructormagic.h"
#include "webrtc/modules/desktop_capture/screen_capture_frame_queue.h"
#include "webrtc/modules/desktop_capture/screen_capturer.h"
#include "webrtc/modules/desktop_capture/screen_capturer_helper.h"
#include "webrtc/modules/desktop_capture/win/scoped_thread_desktop.h"
#include "webrtc/system_wrappers/interface/atomic32.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"

namespace webrtc {

class DesktopFrame;
class DesktopRect;
class Differ;





class ScreenCapturerWinMagnifier : public ScreenCapturer {
 public:
  
  
  
  explicit ScreenCapturerWinMagnifier(
      scoped_ptr<ScreenCapturer> fallback_capturer);
  virtual ~ScreenCapturerWinMagnifier();

  
  virtual void Start(Callback* callback) OVERRIDE;
  virtual void Capture(const DesktopRegion& region) OVERRIDE;
  virtual bool GetScreenList(ScreenList* screens) OVERRIDE;
  virtual bool SelectScreen(ScreenId id) OVERRIDE;
  virtual void SetExcludedWindow(WindowId window) OVERRIDE;

 private:
  typedef BOOL(WINAPI* MagImageScalingCallback)(HWND hwnd,
                                                void* srcdata,
                                                MAGIMAGEHEADER srcheader,
                                                void* destdata,
                                                MAGIMAGEHEADER destheader,
                                                RECT unclipped,
                                                RECT clipped,
                                                HRGN dirty);
  typedef BOOL(WINAPI* MagInitializeFunc)(void);
  typedef BOOL(WINAPI* MagUninitializeFunc)(void);
  typedef BOOL(WINAPI* MagSetWindowSourceFunc)(HWND hwnd, RECT rect);
  typedef BOOL(WINAPI* MagSetWindowFilterListFunc)(HWND hwnd,
                                                   DWORD dwFilterMode,
                                                   int count,
                                                   HWND* pHWND);
  typedef BOOL(WINAPI* MagSetImageScalingCallbackFunc)(
      HWND hwnd,
      MagImageScalingCallback callback);

  static BOOL WINAPI OnMagImageScalingCallback(HWND hwnd,
                                               void* srcdata,
                                               MAGIMAGEHEADER srcheader,
                                               void* destdata,
                                               MAGIMAGEHEADER destheader,
                                               RECT unclipped,
                                               RECT clipped,
                                               HRGN dirty);

  
  
  
  
  
  
  bool CaptureImage(const DesktopRect& rect);

  
  
  bool InitializeMagnifier();

  
  void OnCaptured(void* data, const MAGIMAGEHEADER& header);

  
  void CreateCurrentFrameIfNecessary(const DesktopSize& size);

  
  bool IsCapturingPrimaryScreenOnly() const;

  
  void StartFallbackCapturer();

  static Atomic32 tls_index_;

  scoped_ptr<ScreenCapturer> fallback_capturer_;
  bool fallback_capturer_started_;
  Callback* callback_;
  ScreenId current_screen_id_;
  std::wstring current_device_key_;
  HWND excluded_window_;

  
  
  ScreenCapturerHelper helper_;

  
  ScreenCaptureFrameQueue queue_;

  
  scoped_ptr<Differ> differ_;

  
  bool set_thread_execution_state_failed_;

  ScopedThreadDesktop desktop_;

  
  HDC desktop_dc_;

  HMODULE mag_lib_handle_;
  MagInitializeFunc mag_initialize_func_;
  MagUninitializeFunc mag_uninitialize_func_;
  MagSetWindowSourceFunc set_window_source_func_;
  MagSetWindowFilterListFunc set_window_filter_list_func_;
  MagSetImageScalingCallbackFunc set_image_scaling_callback_func_;

  
  HWND host_window_;
  
  HWND magnifier_window_;

  
  bool magnifier_initialized_;

  
  
  bool magnifier_capture_succeeded_;

  DISALLOW_COPY_AND_ASSIGN(ScreenCapturerWinMagnifier);
};

}  

#endif  
