





































#include "extern.h"
#include "parser.h"
#include "extra.h"
#include "ifuncns.h"

HKEY hkUnreadMailRootKey = HKEY_CURRENT_USER;
char szUnreadMailKey[] = "Software\\Microsoft\\Windows\\CurrentVersion\\UnreadMail";
char szMozillaDesktopKey[] = "Software\\Mozilla\\Desktop";
char szRDISection[] = "Restore Desktop Integration";


typedef struct sKeyNode skn;
struct sKeyNode
{
  char  szKey[MAX_BUF];
  skn   *Next;
  skn   *Prev;
};


skn *CreateSknNode()
{
  skn *sknNode;

  if((sknNode = NS_GlobalAlloc(sizeof(struct sKeyNode))) == NULL)
    exit(1);

  sknNode->Next      = sknNode;
  sknNode->Prev      = sknNode;

  return(sknNode);
}


void SknNodeInsert(skn **sknHead, skn *sknTemp)
{
  if(*sknHead == NULL)
  {
    *sknHead          = sknTemp;
    (*sknHead)->Next  = *sknHead;
    (*sknHead)->Prev  = *sknHead;
  }
  else
  {
    sknTemp->Next           = *sknHead;
    sknTemp->Prev           = (*sknHead)->Prev;
    (*sknHead)->Prev->Next  = sknTemp;
    (*sknHead)->Prev        = sknTemp;
  }
}



void SknNodeDelete(skn *sknTemp)
{
  if(sknTemp != NULL)
  {
    sknTemp->Next->Prev = sknTemp->Prev;
    sknTemp->Prev->Next = sknTemp->Next;
    sknTemp->Next       = NULL;
    sknTemp->Prev       = NULL;

    FreeMemory(&sknTemp);
  }
}


void DeInitSknList(skn **sknHeadNode)
{
  skn *sknTemp;
  
  if(*sknHeadNode == NULL)
    return;

  sknTemp = (*sknHeadNode)->Prev;

  while(sknTemp != *sknHeadNode)
  {
    SknNodeDelete(sknTemp);
    sknTemp = (*sknHeadNode)->Prev;
  }

  SknNodeDelete(sknTemp);
  *sknHeadNode = NULL;
}


int IsMapiMozMapi(BOOL *bIsMozMapi)
{
  HINSTANCE hLib;
  char szMapiFilePath[MAX_BUF];
  int iRv = WIZ_ERROR_UNDEFINED;
  int (PASCAL *GetMapiDllVersion)(void);
  char szMapiVersionKey[] = "MAPI version installed";
  char szBuf[MAX_BUF];
  int  iMapiVersionInstalled;

  

  GetPrivateProfileString(szRDISection, szMapiVersionKey, "", szBuf, sizeof(szBuf), szFileIniUninstall);
  if(*szBuf == '\0')
    return(iRv);

  iMapiVersionInstalled = atoi(szBuf);
  if(GetSystemDirectory(szMapiFilePath, sizeof(szMapiFilePath)) == 0)
    return(iRv);

  AppendBackSlash(szMapiFilePath, sizeof(szMapiFilePath));
  lstrcat(szMapiFilePath, "Mapi32.dll");
  if(!FileExists(szMapiFilePath))
    iRv = WIZ_FILE_NOT_FOUND;
  else if((hLib = LoadLibrary(szMapiFilePath)) != NULL)
  {
    iRv = WIZ_OK;
    *bIsMozMapi = FALSE;
    if(((FARPROC)GetMapiDllVersion = GetProcAddress(hLib, "GetMapiDllVersion")) != NULL)
    {
      if(iMapiVersionInstalled == GetMapiDllVersion())
        *bIsMozMapi = TRUE;
    }
    FreeLibrary(hLib);
  }
  return(iRv);
}






HKEY GetRootKeyAndSubKeyPath(char *szInKeyPath, char *szOutSubKeyPath, DWORD dwOutSubKeyPathSize)
{
  char *ptr      = szInKeyPath;
  HKEY hkRootKey = HKEY_CLASSES_ROOT;

  ZeroMemory(szOutSubKeyPath, dwOutSubKeyPathSize);
  if(ptr == NULL)
    return(hkRootKey);

  
  while(*ptr && (*ptr != '\\'))
    ++ptr;

  if((*ptr == '\0') ||
     (*ptr == '\\'))
  {
    BOOL bPtrModified = FALSE;

    if(*ptr == '\\')
    {
      *ptr = '\0';
      bPtrModified = TRUE;
    }
    hkRootKey = ParseRootKey(szInKeyPath);

    if(bPtrModified)
      *ptr = '\\';

    if((*ptr != '\0') &&
       ((unsigned)lstrlen(ptr + 1) + 1 <= dwOutSubKeyPathSize))
      
      lstrcpy(szOutSubKeyPath, ptr + 1);
  }
  return(hkRootKey);
}





BOOL CheckForNonPrintableChars(char *szInString)
{
  int i;
  int iLen;
  BOOL bFoundNonPrintableChar = FALSE;

  if(!szInString)
    return(TRUE);

  iLen = lstrlen(szInString);

  for(i = 0; i < iLen; i++)
  {
    if(!isprint(szInString[i]))
    {
      bFoundNonPrintableChar = TRUE;
      break;
    }
  }

  return(bFoundNonPrintableChar);
}



BOOL DdeexecCheck(char *szKey, char *szValue)
{
  char szKddeexec[] = "shell\\open\\ddeexec";
  char szKeyLower[MAX_BUF];
  BOOL bPass = TRUE;

  lstrcpy(szKeyLower, szKey);
  CharLowerBuff(szKeyLower, sizeof(szKeyLower));
  if(strstr(szKeyLower, szKddeexec) && CheckForNonPrintableChars(szValue))
    bPass = FALSE;

  return(bPass);
}







void RestoreDesktopIntegration()
{
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
      if(strnicmp(szVarName, szKHKEY, lstrlen(szKHKEY)) == 0)
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
                        lstrlen(szValue));
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
}

void RestoreMozMapi()
{
  char szMozMapiBackupFile[MAX_BUF];
  BOOL bFileIsMozMapi = FALSE;

  GetWinReg(HKEY_LOCAL_MACHINE,
            szMozillaDesktopKey,
            "Mapi_backup_dll",
            szMozMapiBackupFile,
            sizeof(szMozMapiBackupFile));

  




  if((*szMozMapiBackupFile == '\0') || !FileExists(szMozMapiBackupFile))
    return;

  











  if(IsMapiMozMapi(&bFileIsMozMapi) == WIZ_FILE_NOT_FOUND)
    bFileIsMozMapi = TRUE;

  if(bFileIsMozMapi)
  {
    char szDestinationFilename[MAX_BUF];

    
    if(GetSystemDirectory(szDestinationFilename, sizeof(szDestinationFilename)))
    {
      
      AppendBackSlash(szDestinationFilename, sizeof(szDestinationFilename));
      lstrcat(szDestinationFilename, "Mapi32.dll");
      CopyFile(szMozMapiBackupFile, szDestinationFilename, FALSE);
    }
  }
  
  FileDelete(szMozMapiBackupFile);
}

BOOL UndoDesktopIntegration(void)
{
  char szMozillaKey[] = "Software\\Mozilla";
  char szBuf[MAX_BUF];

  

  GetPrivateProfileString(szRDISection, "Enabled", "", szBuf, sizeof(szBuf), szFileIniUninstall);
  if(lstrcmpi(szBuf, "TRUE") == 0)
  {
    RestoreDesktopIntegration();
    RestoreMozMapi();

    DeleteWinRegKey(HKEY_LOCAL_MACHINE, szMozillaDesktopKey, TRUE);
    DeleteWinRegKey(HKEY_LOCAL_MACHINE, szMozillaKey, FALSE);
  }

  return(0);
}




int GetUninstallAppPathName(char *szAppPathName, DWORD dwAppPathNameSize)
{
  char szKey[MAX_BUF];
  HKEY hkRoot;

  if(*ugUninstall.szUserAgent != '\0')
  {
    hkRoot = ugUninstall.hWrMainRoot;
    lstrcpy(szKey, ugUninstall.szWrMainKey);
    AppendBackSlash(szKey, sizeof(szKey));
    lstrcat(szKey, ugUninstall.szUserAgent);
    AppendBackSlash(szKey, sizeof(szKey));
    lstrcat(szKey, "Main");
  }
  else
  {
    return(CMI_APP_PATHNAME_NOT_FOUND);
  }

  GetWinReg(hkRoot, szKey, "PathToExe", szAppPathName, dwAppPathNameSize);
  CharUpperBuff(szAppPathName, dwAppPathNameSize);
  return(CMI_OK);
}




void DeleteUnreadMailKeys(skn *sknHeadNode)
{
  skn   *sknTempNode;
  char  szUnreadMailDeleteKey[MAX_BUF];
  DWORD dwLength;
  
  if(sknHeadNode == NULL)
    return;

  sknTempNode = sknHeadNode;

  do
  {
    
    dwLength = sizeof(szUnreadMailDeleteKey) > lstrlen(szUnreadMailKey) ?
                      lstrlen(szUnreadMailKey) + 1: sizeof(szUnreadMailDeleteKey);
    lstrcpyn(szUnreadMailDeleteKey, szUnreadMailKey, dwLength);
    AppendBackSlash(szUnreadMailDeleteKey, sizeof(szUnreadMailDeleteKey));
    if((unsigned)(lstrlen(sknTempNode->szKey) + 1) <
       (sizeof(szUnreadMailDeleteKey) - lstrlen(szUnreadMailKey) + 1))
    {
      lstrcat(szUnreadMailDeleteKey, sknTempNode->szKey);

      
      DeleteWinRegKey(hkUnreadMailRootKey, szUnreadMailDeleteKey, TRUE);
    }

    
    sknTempNode = sknTempNode->Next;

  }while(sknTempNode != sknHeadNode);
}




BOOL GetUnreadMailKeyList(char *szUninstallAppPathName, skn **sknWinRegKeyList)
{
  HKEY  hkSubKeyHandle;
  HKEY  hkHandle;
  DWORD dwErr = ERROR_SUCCESS;
  DWORD dwTotalSubKeys;
  DWORD dwSubKeySize;
  DWORD dwIndex;
  DWORD dwBufSize;
  BOOL  bFoundAtLeastOne = FALSE;
  char  szSubKey[MAX_BUF];
  char  szBuf[MAX_BUF];
  char  szNewKey[MAX_BUF];
  skn   *sknTempNode;

  
  dwErr = RegOpenKeyEx(hkUnreadMailRootKey,
                       szUnreadMailKey,
                       0,
                       KEY_READ|KEY_QUERY_VALUE,
                       &hkHandle);
  if(dwErr == ERROR_SUCCESS)
  {
    dwTotalSubKeys = 0;
    RegQueryInfoKey(hkHandle,
                    NULL,
                    NULL,
                    NULL,
                    &dwTotalSubKeys,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    NULL);

    if(dwTotalSubKeys != 0)
    {
      dwIndex = 0;
      do
      {
        






        sknTempNode = NULL; 
        dwSubKeySize = sizeof(szSubKey);
        if((dwErr = RegEnumKeyEx(hkHandle,
                                 dwIndex,
                                 szSubKey,
                                 &dwSubKeySize,
                                 NULL,
                                 NULL,
                                 NULL,
                                 NULL)) == ERROR_SUCCESS)
        {
          lstrcpy(szNewKey, szUnreadMailKey);
          AppendBackSlash(szNewKey, sizeof(szNewKey));
          lstrcat(szNewKey, szSubKey);

          if(RegOpenKeyEx(hkUnreadMailRootKey,
                          szNewKey,
                          0,
                          KEY_READ,
                          &hkSubKeyHandle) == ERROR_SUCCESS)
          {
            dwBufSize = sizeof(szBuf);
            if(RegQueryValueEx(hkSubKeyHandle,
                               "Application",
                               NULL,
                               NULL,
                               szBuf,
                               &dwBufSize) == ERROR_SUCCESS)
            {
              CharUpperBuff(szBuf, sizeof(szBuf));
              if(strstr(szBuf, szUninstallAppPathName) != NULL)
              {
                bFoundAtLeastOne = TRUE;
                sknTempNode = CreateSknNode();
                lstrcpyn(sknTempNode->szKey, szSubKey, dwSubKeySize + 1);
              }
              else
              {
                char szUninstallAppPathNameShort[MAX_BUF];

                GetShortPathName(szUninstallAppPathName,
                                 szUninstallAppPathNameShort,
                                 sizeof(szUninstallAppPathNameShort));
                if(strstr(szBuf, szUninstallAppPathNameShort) != NULL)
                {
                  bFoundAtLeastOne = TRUE;
                  sknTempNode = CreateSknNode();
                  lstrcpyn(sknTempNode->szKey, szSubKey, dwSubKeySize + 1);
                }
              }
            }
            RegCloseKey(hkSubKeyHandle);
          }
        }

        if(sknTempNode)
          SknNodeInsert(sknWinRegKeyList, sknTempNode);

        ++dwIndex;
      } while(dwErr != ERROR_NO_MORE_ITEMS);
    }
    RegCloseKey(hkHandle);
  }
  return(bFoundAtLeastOne);
}



int CleanupMailIntegration(void)
{
  char szCMISection[] = "Cleanup Mail Integration";
  char szUninstallApp[MAX_BUF];
  char szBuf[MAX_BUF];
  skn  *sknWinRegKeyList = NULL;

  

  GetPrivateProfileString(szCMISection,
                          "Enabled",
                          "",
                          szBuf,
                          sizeof(szBuf),
                          szFileIniUninstall);
  if(lstrcmpi(szBuf, "TRUE") != 0)
    return(CMI_OK);

  
  if(GetUninstallAppPathName(szUninstallApp, sizeof(szUninstallApp)) != CMI_OK)
    return(CMI_APP_PATHNAME_NOT_FOUND);

  
  if(GetUnreadMailKeyList(szUninstallApp, &sknWinRegKeyList))
    

    DeleteUnreadMailKeys(sknWinRegKeyList);

  
  DeInitSknList(&sknWinRegKeyList);
  return(CMI_OK);
}

