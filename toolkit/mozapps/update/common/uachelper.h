



#ifndef _UACHELPER_H_
#define _UACHELPER_H_

class UACHelper
{
public:
  static HANDLE OpenUserToken(DWORD sessionID);
  static HANDLE OpenLinkedToken(HANDLE token);
  static BOOL DisablePrivileges(HANDLE token);
  static bool CanUserElevate();
  static bool IsDirectorySafe(LPCWSTR inputPath);
  static bool DenyWriteACLOnPath(LPCWSTR path, PACL *originalACL,
                                 PSECURITY_DESCRIPTOR *sd);

private:
  static BOOL SetPrivilege(HANDLE token, LPCTSTR privs, BOOL enable);
  static BOOL DisableUnneededPrivileges(HANDLE token,
                                        LPCTSTR *unneededPrivs, size_t count);
  static LPCTSTR PrivsToDisable[];
};

#endif
