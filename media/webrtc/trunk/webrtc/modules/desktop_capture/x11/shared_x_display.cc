









#include "webrtc/modules/desktop_capture/x11/shared_x_display.h"

#include <algorithm>

#include "webrtc/system_wrappers/interface/logging.h"

namespace webrtc {

SharedXDisplay::SharedXDisplay(Display* display)
  : display_(display) {
  assert(display_);
}

SharedXDisplay::~SharedXDisplay() {
  assert(event_handlers_.empty());
  XCloseDisplay(display_);
}


scoped_refptr<SharedXDisplay> SharedXDisplay::Create(
    const std::string& display_name) {
  Display* display =
      XOpenDisplay(display_name.empty() ? NULL : display_name.c_str());
  if (!display) {
    LOG(LS_ERROR) << "Unable to open display";
    return NULL;
  }
  return new SharedXDisplay(display);
}


scoped_refptr<SharedXDisplay> SharedXDisplay::CreateDefault() {
  return Create(std::string());
}

void SharedXDisplay::AddEventHandler(int type, XEventHandler* handler) {
  event_handlers_[type].push_back(handler);
}

void SharedXDisplay::RemoveEventHandler(int type, XEventHandler* handler) {
  EventHandlersMap::iterator handlers = event_handlers_.find(type);
  if (handlers == event_handlers_.end())
    return;

  std::vector<XEventHandler*>::iterator new_end =
      std::remove(handlers->second.begin(), handlers->second.end(), handler);
  handlers->second.erase(new_end, handlers->second.end());

  
  if (handlers->second.empty())
    event_handlers_.erase(handlers);
}

void SharedXDisplay::ProcessPendingXEvents() {
  
  
  scoped_refptr<SharedXDisplay> self(this);

  
  
  int events_to_process = XPending(display());
  XEvent e;

  for (int i = 0; i < events_to_process; i++) {
    XNextEvent(display(), &e);
    EventHandlersMap::iterator handlers = event_handlers_.find(e.type);
    if (handlers == event_handlers_.end())
      continue;
    for (std::vector<XEventHandler*>::iterator it = handlers->second.begin();
         it != handlers->second.end(); ++it) {
      if ((*it)->HandleXEvent(e))
        break;
    }
  }
}

}  
