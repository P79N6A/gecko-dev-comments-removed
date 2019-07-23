



#ifndef CHROME_COMMON_NATIVE_WEB_KEYBOARD_EVENT_H_
#define CHROME_COMMON_NATIVE_WEB_KEYBOARD_EVENT_H_

#include "base/basictypes.h"
#include "third_party/WebKit/WebKit/chromium/public/WebInputEvent.h"

#if defined(OS_WIN)
#include <windows.h>
#elif defined(OS_MACOSX)
#ifdef __OBJC__
@class NSEvent;
#else
class NSEvent;
#endif  
#elif defined(OS_LINUX)
#include <gdk/gdk.h>
#endif



struct NativeWebKeyboardEvent : public WebKit::WebKeyboardEvent {
  NativeWebKeyboardEvent();

#if defined(OS_WIN)
  NativeWebKeyboardEvent(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);
#elif defined(OS_MACOSX)
  explicit NativeWebKeyboardEvent(NSEvent *event);
#elif defined(OS_LINUX)
  explicit NativeWebKeyboardEvent(const GdkEventKey* event);
#endif

  NativeWebKeyboardEvent(const NativeWebKeyboardEvent& event);
  ~NativeWebKeyboardEvent();

  NativeWebKeyboardEvent& operator=(const NativeWebKeyboardEvent& event);

#if defined(OS_WIN)
  MSG os_event;
#elif defined(OS_MACOSX)
  NSEvent* os_event;
#elif defined(OS_LINUX)
  GdkEventKey* os_event;
#endif
};

#endif  
