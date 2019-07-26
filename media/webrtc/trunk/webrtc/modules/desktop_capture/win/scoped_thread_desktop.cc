









#include "webrtc/modules/desktop_capture/win/scoped_thread_desktop.h"

#include "webrtc/system_wrappers/interface/logging.h"

#include "webrtc/modules/desktop_capture/win/desktop.h"

namespace webrtc {

ScopedThreadDesktop::ScopedThreadDesktop()
    : initial_(Desktop::GetThreadDesktop()) {
}

ScopedThreadDesktop::~ScopedThreadDesktop() {
  Revert();
}

bool ScopedThreadDesktop::IsSame(const Desktop& desktop) {
  if (assigned_.get() != NULL) {
    return assigned_->IsSame(desktop);
  } else {
    return initial_->IsSame(desktop);
  }
}

void ScopedThreadDesktop::Revert() {
  if (assigned_.get() != NULL) {
    initial_->SetThreadDesktop();
    assigned_.reset();
  }
}

bool ScopedThreadDesktop::SetThreadDesktop(Desktop* desktop) {
  Revert();

  scoped_ptr<Desktop> scoped_desktop(desktop);

  if (initial_->IsSame(*desktop))
    return true;

  if (!desktop->SetThreadDesktop())
    return false;

  assigned_.reset(scoped_desktop.release());
  return true;
}

}  
