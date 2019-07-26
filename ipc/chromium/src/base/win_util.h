



#ifndef BASE_WIN_UTIL_H__
#define BASE_WIN_UTIL_H__

#include <windows.h>
#include <aclapi.h>

#include <string>

#include "base/tracked.h"

namespace win_util {




enum WinVersion {
  WINVERSION_PRE_2000 = 0,  
  WINVERSION_2000 = 1,
  WINVERSION_XP = 2,
  WINVERSION_SERVER_2003 = 3,
  WINVERSION_VISTA = 4,
  WINVERSION_2008 = 5,
  WINVERSION_WIN7 = 6
};

void GetNonClientMetrics(NONCLIENTMETRICS* metrics);


WinVersion GetWinVersion();


void GetServicePackLevel(int* major, int* minor);





bool AddAccessToKernelObject(HANDLE handle, WELL_KNOWN_SID_TYPE known_sid,
                             ACCESS_MASK access);


bool GetUserSidString(std::wstring* user_sid);


bool IsShiftPressed();


bool IsCtrlPressed();


bool IsAltPressed();



std::wstring FormatMessage(unsigned messageid);


std::wstring FormatLastWin32Error();

}  

#endif  
