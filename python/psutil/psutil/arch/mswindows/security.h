










#include <windows.h>


BOOL SetPrivilege(HANDLE hToken, LPCTSTR Privilege, BOOL bEnablePrivilege);
int SetSeDebug();
int UnsetSeDebug();
HANDLE token_from_handle(HANDLE hProcess);
int HasSystemPrivilege(HANDLE hProcess);

