



#ifndef SANDBOX_SRC_WINDOW_H_
#define SANDBOX_SRC_WINDOW_H_

#include <windows.h>
#include <string>

#include "base/strings/string16.h"
#include "sandbox/win/src/sandbox_types.h"

namespace sandbox {

  
  
  
  ResultCode CreateAltWindowStation(HWINSTA* winsta);

  
  
  
  
  
  
  ResultCode CreateAltDesktop(HWINSTA winsta, HDESK* desktop);

  
  base::string16 GetWindowObjectName(HANDLE handle);

  
  
  
  
  base::string16 GetFullDesktopName(HWINSTA winsta, HDESK desktop);

}  

#endif  
