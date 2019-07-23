






































#ifndef _IFUNCNS_H_
#define _IFUNCNS_H_

#include "uninstall.h"

HRESULT     FileUncompress(LPSTR szFrom, LPSTR szTo);
HRESULT     FileMove(LPSTR szFrom, LPSTR szTo);
HRESULT     FileCopy(LPSTR szFrom, LPSTR szTo, BOOL bFailIfExists);
HRESULT     FileDelete(LPSTR szDestination);
HRESULT     DirectoryRemove(LPSTR szDestination, BOOL bRemoveSubdirs);
HRESULT     CreateDirectoriesAll(char* szPath);
HKEY        ParseRootKey(LPSTR szRootKey);
LPSTR       GetStringRootKey(HKEY hkRootKey, LPSTR szString, DWORD dwStringSize);
BOOL        SearchForUninstallKeys(char *szStringToMatch);
BOOL        WinRegKeyExists(HKEY hkRootKey, LPSTR szKey);

#endif
