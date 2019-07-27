








#include <windows.h>

BOOL psutil_set_privilege(HANDLE hToken, LPCTSTR Privilege, BOOL bEnablePrivilege);
HANDLE psutil_token_from_handle(HANDLE hProcess);
int psutil_has_system_privilege(HANDLE hProcess);
int psutil_set_se_debug();
int psutil_unset_se_debug();

