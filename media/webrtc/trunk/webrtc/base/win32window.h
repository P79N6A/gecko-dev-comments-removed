









#ifndef WEBRTC_BASE_WIN32WINDOW_H_
#define WEBRTC_BASE_WIN32WINDOW_H_

#if defined(WEBRTC_WIN)

#include "webrtc/base/win32.h"

namespace rtc {





class Win32Window {
 public:
  Win32Window();
  virtual ~Win32Window();

  HWND handle() const { return wnd_; }

  bool Create(HWND parent, const wchar_t* title, DWORD style, DWORD exstyle,
              int x, int y, int cx, int cy);
  void Destroy();

  
  static void Shutdown();

 protected:
  virtual bool OnMessage(UINT uMsg, WPARAM wParam, LPARAM lParam,
                         LRESULT& result);

  virtual bool OnClose() { return true; }
  virtual void OnNcDestroy() { }

 private:
  static LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam,
                                  LPARAM lParam);

  HWND wnd_;
  static HINSTANCE instance_;
  static ATOM window_class_;
};



}  

#endif  

#endif  
