



#include "sandbox/win/src/window.h"

#include <aclapi.h>

#include "base/logging.h"
#include "base/memory/scoped_ptr.h"
#include "sandbox/win/src/acl.h"
#include "sandbox/win/src/sid.h"

namespace {




bool GetSecurityAttributes(HANDLE handle, SECURITY_ATTRIBUTES* attributes) {
  attributes->bInheritHandle = FALSE;
  attributes->nLength = sizeof(SECURITY_ATTRIBUTES);

  PACL dacl = NULL;
  DWORD result = ::GetSecurityInfo(handle, SE_WINDOW_OBJECT,
                                   DACL_SECURITY_INFORMATION, NULL, NULL, &dacl,
                                   NULL, &attributes->lpSecurityDescriptor);
  if (ERROR_SUCCESS == result)
    return true;

  return false;
}

}

namespace sandbox {

ResultCode CreateAltWindowStation(HWINSTA* winsta) {
  
  
  SECURITY_ATTRIBUTES attributes = {0};
  if (!GetSecurityAttributes(::GetProcessWindowStation(), &attributes)) {
    return SBOX_ERROR_CANNOT_CREATE_WINSTATION;
  }

  
  
  *winsta = ::CreateWindowStationW(
      NULL, 0, GENERIC_READ | WINSTA_CREATEDESKTOP, &attributes);
  LocalFree(attributes.lpSecurityDescriptor);

  if (*winsta)
    return SBOX_ALL_OK;

  return SBOX_ERROR_CANNOT_CREATE_WINSTATION;
}

ResultCode CreateAltDesktop(HWINSTA winsta, HDESK* desktop) {
  base::string16 desktop_name = L"sbox_alternate_desktop_";

  
  wchar_t buffer[16];
  _snwprintf_s(buffer, sizeof(buffer) / sizeof(wchar_t), L"0x%X",
               ::GetCurrentProcessId());
  desktop_name += buffer;

  
  
  SECURITY_ATTRIBUTES attributes = {0};
  if (!GetSecurityAttributes(GetThreadDesktop(GetCurrentThreadId()),
                             &attributes)) {
    return SBOX_ERROR_CANNOT_CREATE_DESKTOP;
  }

  
  HWINSTA current_winsta = ::GetProcessWindowStation();

  if (winsta) {
    
    
    if (!::SetProcessWindowStation(winsta)) {
      ::LocalFree(attributes.lpSecurityDescriptor);
      return SBOX_ERROR_CANNOT_CREATE_DESKTOP;
    }
  }

  
  *desktop = ::CreateDesktop(desktop_name.c_str(),
                             NULL,
                             NULL,
                             0,
                             DESKTOP_CREATEWINDOW | DESKTOP_READOBJECTS |
                                 READ_CONTROL | WRITE_DAC | WRITE_OWNER,
                             &attributes);
  ::LocalFree(attributes.lpSecurityDescriptor);

  if (winsta) {
    
    if (!::SetProcessWindowStation(current_winsta)) {
      return SBOX_ERROR_FAILED_TO_SWITCH_BACK_WINSTATION;
    }
  }

  if (*desktop) {
    
    
    static const ACCESS_MASK kDesktopDenyMask = WRITE_DAC | WRITE_OWNER |
                                                DELETE |
                                                DESKTOP_CREATEMENU |
                                                DESKTOP_CREATEWINDOW |
                                                DESKTOP_HOOKCONTROL |
                                                DESKTOP_JOURNALPLAYBACK |
                                                DESKTOP_JOURNALRECORD |
                                                DESKTOP_SWITCHDESKTOP;
    AddKnownSidToObject(*desktop, SE_WINDOW_OBJECT, Sid(WinRestrictedCodeSid),
                        DENY_ACCESS, kDesktopDenyMask);
    return SBOX_ALL_OK;
  }

  return SBOX_ERROR_CANNOT_CREATE_DESKTOP;
}

base::string16 GetWindowObjectName(HANDLE handle) {
  
  DWORD size = 0;
  ::GetUserObjectInformation(handle, UOI_NAME, NULL, 0, &size);

  if (!size) {
    NOTREACHED();
    return base::string16();
  }

  
  scoped_ptr<wchar_t[]> name_buffer(new wchar_t[size]);

  
  if (!::GetUserObjectInformation(handle, UOI_NAME, name_buffer.get(), size,
                                  &size)) {
    NOTREACHED();
    return base::string16();
  }

  return base::string16(name_buffer.get());
}

base::string16 GetFullDesktopName(HWINSTA winsta, HDESK desktop) {
  if (!desktop) {
    NOTREACHED();
    return base::string16();
  }

  base::string16 name;
  if (winsta) {
    name = GetWindowObjectName(winsta);
    name += L'\\';
  }

  name += GetWindowObjectName(desktop);
  return name;
}

}  
