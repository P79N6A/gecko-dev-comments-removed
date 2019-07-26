









#ifndef WEBRTC_MODULES_DESKTOP_CAPTURE_WIN_SCOPED_THREAD_DESKTOP_H_
#define WEBRTC_MODULES_DESKTOP_CAPTURE_WIN_SCOPED_THREAD_DESKTOP_H_

#include <windows.h>

#include "webrtc/system_wrappers/interface/constructor_magic.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"

namespace webrtc {

class Desktop;

class ScopedThreadDesktop {
 public:
  ScopedThreadDesktop();
  ~ScopedThreadDesktop();

  
  
  
  
  bool IsSame(const Desktop& desktop);

  
  void Revert();

  
  
  bool SetThreadDesktop(Desktop* desktop);

 private:
  
  scoped_ptr<Desktop> assigned_;

  
  scoped_ptr<Desktop> initial_;

  DISALLOW_COPY_AND_ASSIGN(ScopedThreadDesktop);
};

}  

#endif  
