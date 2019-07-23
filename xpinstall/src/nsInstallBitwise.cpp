






































#include "nscore.h"
#include "nsInstall.h" 
#include "prmem.h"
#include "nsInstallBitwise.h"

#ifdef _WINDOWS
#include <windows.h>
#include <winreg.h>
#endif

#define KEY_SHARED_DLLS "Software\\Microsoft\\Windows\\CurrentVersion\\SharedDlls"

PRInt32 RegisterSharedFile(const char *file, PRBool bAlreadyExists)
{
  PRInt32 rv = nsInstall::SUCCESS;

#ifdef WIN32
  HKEY     root;
  HKEY     keyHandle = 0;
  LONG     result;
  DWORD    type = REG_DWORD;
  DWORD    dwDisposition;
  PRUint32 valbuf = 0;
  PRUint32 valbufsize;

  valbufsize = sizeof(PRUint32);
  root       = HKEY_LOCAL_MACHINE;
  result     = RegCreateKeyEx(root, KEY_SHARED_DLLS, 0, nsnull, REG_OPTION_NON_VOLATILE, KEY_READ | KEY_WRITE, nsnull, &keyHandle, &dwDisposition);
  if(ERROR_SUCCESS == result)
  {
    result = RegQueryValueEx(keyHandle, file, nsnull, &type, (LPBYTE)&valbuf, (LPDWORD)&valbufsize);

    if((ERROR_SUCCESS == result) && (type == REG_DWORD))
      ++valbuf;
    else
    {
      valbuf = 1;
      if(bAlreadyExists == PR_TRUE)
        ++valbuf;
    }

    RegSetValueEx(keyHandle, file, 0, REG_DWORD, (LPBYTE)&valbuf, valbufsize);
    RegCloseKey(keyHandle);
  }
  else
    rv = nsInstall::UNEXPECTED_ERROR;

#endif

  return(rv);
}

