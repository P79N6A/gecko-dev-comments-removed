





































#include "extern.h"
#include "extra.h"
#include "parser.h"
#include "ifuncns.h"

char szUnreadMailKey[] = "Software\\Microsoft\\Windows\\CurrentVersion\\UnreadMail";
char szMozillaDesktopKey[] = "Software\\Mozilla\\Desktop";
char szRDISection[] = "Restore Desktop Integration";







void RestoreDesktopIntegration()
{
#ifdef OLDCODE
  char      szVarName[MAX_BUF];
  char      szValue[MAX_BUF];
  char      szSubKey[MAX_BUF];
  HKEY      hkHandle;
  DWORD     dwIndex;
  DWORD     dwSubKeySize;
  DWORD     dwTotalValues;
  char      szKHKEY[]               = "HKEY";
  char      szKisHandling[]         = "isHandling";

  if(RegOpenKeyEx(HKEY_LOCAL_MACHINE, szMozillaDesktopKey, 0, KEY_READ|KEY_WRITE, &hkHandle) != ERROR_SUCCESS)
    return;

  dwTotalValues  = 0;
  RegQueryInfoKey(hkHandle, NULL, NULL, NULL, NULL, NULL, NULL, &dwTotalValues, NULL, NULL, NULL, NULL);
  for(dwIndex = 0; dwIndex < dwTotalValues; dwIndex++)
  {
    
    dwSubKeySize = sizeof(szVarName);
    if(RegEnumValue(hkHandle, dwIndex, szVarName, &dwSubKeySize, NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
    {
      if(strnicmp(szVarName, szKHKEY, strlen(szKHKEY)) == 0)
      {
        HKEY hkRootKey;

        hkRootKey = GetRootKeyAndSubKeyPath(szVarName, szSubKey, sizeof(szSubKey));
        if(*szSubKey != '\0')
        {
          GetWinReg(HKEY_LOCAL_MACHINE, szMozillaDesktopKey, szVarName, szValue, sizeof(szValue));
          if(*szValue != '\0')
          {
            



            if(DdeexecCheck(szSubKey, szValue))
            {
              
              SetWinReg(hkRootKey,
                        szSubKey,
                        NULL,
                        REG_SZ,
                        szValue,
                        strlen(szValue));
            }
          }
          else
            

            DeleteWinRegValue(hkRootKey,
                              szSubKey,
                              szValue);
        }
      }
    }
  }
  RegCloseKey(hkHandle);
  return;
#endif
}

BOOL UndoDesktopIntegration(void)
{
#ifdef OLDCODE
  char szMozillaKey[] = "Software\\Mozilla";
  char szBuf[MAX_BUF];

  

  GetPrivateProfileString(szRDISection, "Enabled", "", szBuf, sizeof(szBuf), szFileIniUninstall);
  if(stricmp(szBuf, "TRUE") == 0)
  {
    RestoreDesktopIntegration();

    DeleteWinRegKey(HKEY_LOCAL_MACHINE, szMozillaDesktopKey, TRUE);
    DeleteWinRegKey(HKEY_LOCAL_MACHINE, szMozillaKey, FALSE);
  }
#endif
  return(0);
}

