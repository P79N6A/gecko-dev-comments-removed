









#ifndef WEBRTC_BASE_LINUXWINDOWPICKER_H_
#define WEBRTC_BASE_LINUXWINDOWPICKER_H_

#include "webrtc/base/basictypes.h"
#include "webrtc/base/scoped_ptr.h"
#include "webrtc/base/windowpicker.h"


struct _XDisplay;
typedef unsigned long Window;

namespace rtc {

class XWindowEnumerator;

class X11WindowPicker : public WindowPicker {
 public:
  X11WindowPicker();
  ~X11WindowPicker();

  static bool IsDesktopElement(_XDisplay* display, Window window);

  virtual bool Init();
  virtual bool IsVisible(const WindowId& id);
  virtual bool MoveToFront(const WindowId& id);
  virtual bool GetWindowList(WindowDescriptionList* descriptions);
  virtual bool GetDesktopList(DesktopDescriptionList* descriptions);
  virtual bool GetDesktopDimensions(const DesktopId& id, int* width,
                                    int* height);
  uint8* GetWindowIcon(const WindowId& id, int* width, int* height);
  uint8* GetWindowThumbnail(const WindowId& id, int width, int height);
  int GetNumDesktops();
  uint8* GetDesktopThumbnail(const DesktopId& id, int width, int height);

 private:
  scoped_ptr<XWindowEnumerator> enumerator_;
};

}  

#endif  
