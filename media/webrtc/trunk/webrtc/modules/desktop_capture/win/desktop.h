









#ifndef WEBRTC_MODULES_DESKTOP_CAPTURE_WIN_DESKTOP_H_
#define WEBRTC_MODULES_DESKTOP_CAPTURE_WIN_DESKTOP_H_

#include <string>
#include <windows.h>

#include "webrtc/system_wrappers/interface/constructor_magic.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"

namespace webrtc {

class Desktop {
 public:
  ~Desktop();

  
  
  bool GetName(std::wstring* desktop_name_out) const;

  
  
  
  bool IsSame(const Desktop& other) const;

  
  
  bool SetThreadDesktop() const;

  
  static Desktop* GetDesktop(const wchar_t* desktop_name);

  
  
  static Desktop* GetInputDesktop();

  
  
  static Desktop* GetThreadDesktop();

 private:
  Desktop(HDESK desktop, bool own);

  
  HDESK desktop_;

  
  bool own_;

  DISALLOW_COPY_AND_ASSIGN(Desktop);
};

}  

#endif  
