









#ifndef WEBRTC_MODULES_DESKTOP_CAPTURE_X11_SHARED_X_DISPLAY_H_
#define WEBRTC_MODULES_DESKTOP_CAPTURE_X11_SHARED_X_DISPLAY_H_

#include <map>
#include <vector>

#include <assert.h>
#include <X11/Xlib.h>

#include <string>

#include "webrtc/system_wrappers/interface/atomic32.h"
#include "webrtc/system_wrappers/interface/scoped_refptr.h"

namespace webrtc {


class SharedXDisplay {
 public:
  class XEventHandler {
   public:
    virtual ~XEventHandler() {}

    
    virtual bool HandleXEvent(const XEvent& event) = 0;
  };

  
  explicit SharedXDisplay(Display* display);

  
  
  
  static scoped_refptr<SharedXDisplay> Create(const std::string& display_name);

  
  
  static scoped_refptr<SharedXDisplay> CreateDefault();

  void AddRef() { ++ref_count_; }
  void Release() {
    if (--ref_count_ == 0)
      delete this;
  }

  Display* display() { return display_; }

  
  void AddEventHandler(int type, XEventHandler* handler);

  
  
  void RemoveEventHandler(int type, XEventHandler* handler);

  
  void ProcessPendingXEvents();

 private:
  typedef std::map<int, std::vector<XEventHandler*> > EventHandlersMap;

  ~SharedXDisplay();

  Atomic32 ref_count_;
  Display* display_;

  EventHandlersMap event_handlers_;

  DISALLOW_COPY_AND_ASSIGN(SharedXDisplay);
};

}  

#endif  
