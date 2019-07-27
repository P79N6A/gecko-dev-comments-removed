



#ifndef SANDBOX_SRC_WINDOW_H_
#define SANDBOX_SRC_WINDOW_H_

#include <windows.h>
#include <string>

#include "sandbox/win/src/sandbox_types.h"

namespace sandbox {

  
  
  
  ResultCode CreateAltWindowStation(HWINSTA* winsta);

  
  
  
  
  
  
  ResultCode CreateAltDesktop(HWINSTA winsta, HDESK* desktop);

  
  std::wstring GetWindowObjectName(HANDLE handle);

  
  
  
  
  std::wstring GetFullDesktopName(HWINSTA winsta, HDESK desktop);

}  

#endif  
