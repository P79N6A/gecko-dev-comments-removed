



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
  WINVERSION_WIN7 = 6,
};

void GetNonClientMetrics(NONCLIENTMETRICS* metrics);


WinVersion GetWinVersion();


void GetServicePackLevel(int* major, int* minor);





bool AddAccessToKernelObject(HANDLE handle, WELL_KNOWN_SID_TYPE known_sid,
                             ACCESS_MASK access);


bool GetUserSidString(std::wstring* user_sid);





bool GetLogonSessionOnlyDACL(SECURITY_DESCRIPTOR** security_descriptor);


WNDPROC SetWindowProc(HWND hwnd, WNDPROC wndproc);


bool IsSubclassed(HWND window, WNDPROC subclass_proc);





bool Subclass(HWND window, WNDPROC subclass_proc);





bool Unsubclass(HWND window, WNDPROC subclass_proc);



WNDPROC GetSuperclassWNDPROC(HWND window);



void* SetWindowUserData(HWND hwnd, void* user_data);
void* GetWindowUserData(HWND hwnd);


bool IsShiftPressed();


bool IsCtrlPressed();


bool IsAltPressed();



std::wstring GetClassName(HWND window);







bool UserAccountControlIsEnabled();



std::wstring FormatMessage(unsigned messageid);


std::wstring FormatLastWin32Error();




void NotifyHWNDCreation(const tracked_objects::Location& from_here, HWND hwnd);
void NotifyHWNDDestruction(const tracked_objects::Location& from_here,
                           HWND hwnd);

#define TRACK_HWND_CREATION(hwnd) win_util::NotifyHWNDCreation(FROM_HERE, hwnd)
#define TRACK_HWND_DESTRUCTION(hwnd) \
    win_util::NotifyHWNDDestruction(FROM_HERE, hwnd)

}  

#endif  
