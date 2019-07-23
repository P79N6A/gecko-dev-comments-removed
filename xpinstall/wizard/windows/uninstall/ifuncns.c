






































#include "ifuncns.h"
#include "extra.h"

BOOL SearchForUninstallKeys(char *szStringToMatch)
{
  char      szBuf[MAX_BUF];
  char      szStringToMatchLowerCase[MAX_BUF];
  char      szBufKey[MAX_BUF];
  char      szSubKey[MAX_BUF];
  HKEY      hkHandle;
  BOOL      bFound;
  DWORD     dwIndex;
  DWORD     dwSubKeySize;
  DWORD     dwTotalSubKeys;
  DWORD     dwTotalValues;
  FILETIME  ftLastWriteFileTime;
  char      szWRMSUninstallKeyPath[] = "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall";
  char      szWRMSUninstallName[] =  "UninstallString";

  lstrcpyn(szStringToMatchLowerCase, szStringToMatch, sizeof(szStringToMatchLowerCase));
  CharLower(szStringToMatchLowerCase);

  bFound = FALSE;
  if(RegOpenKeyEx(HKEY_LOCAL_MACHINE, szWRMSUninstallKeyPath, 0, KEY_READ, &hkHandle) != ERROR_SUCCESS)
    return(bFound);

  dwTotalSubKeys = 0;
  dwTotalValues  = 0;
  RegQueryInfoKey(hkHandle, NULL, NULL, NULL, &dwTotalSubKeys, NULL, NULL, &dwTotalValues, NULL, NULL, NULL, NULL);
  for(dwIndex = 0; dwIndex < dwTotalSubKeys; dwIndex++)
  {
    dwSubKeySize = sizeof(szSubKey);
    if(RegEnumKeyEx(hkHandle, dwIndex, szSubKey, &dwSubKeySize, NULL, NULL, NULL, &ftLastWriteFileTime) == ERROR_SUCCESS)
    {
      wsprintf(szBufKey, "%s\\%s", szWRMSUninstallKeyPath, szSubKey);
      GetWinReg(HKEY_LOCAL_MACHINE, szBufKey, szWRMSUninstallName, szBuf, sizeof(szBuf));
      CharLower(szBuf);
      if(strstr(szBuf, szStringToMatchLowerCase) != NULL)
      {
        bFound = TRUE;

        
        break;
      }
    }
  }

  RegCloseKey(hkHandle);
  return(bFound);
}

HRESULT FileMove(LPSTR szFrom, LPSTR szTo)
{
  HANDLE          hFile;
  WIN32_FIND_DATA fdFile;
  char            szFromDir[MAX_BUF];
  char            szFromTemp[MAX_BUF];
  char            szToTemp[MAX_BUF];
  char            szBuf[MAX_BUF];
  BOOL            bFound;

  
  if((FileExists(szFrom)) && (!FileExists(szTo)))
  {
    MoveFile(szFrom, szTo);
    return(FO_SUCCESS);
  }
  
  if(FileExists(szFrom) && FileExists(szTo))
  {
    
    
    
    lstrcpy(szToTemp, szTo);
    AppendBackSlash(szToTemp, sizeof(szToTemp));
    ParsePath(szFrom, szBuf, MAX_BUF, PP_FILENAME_ONLY);
    lstrcat(szToTemp, szBuf);
    MoveFile(szFrom, szToTemp);
    return(FO_SUCCESS);
  }

  ParsePath(szFrom, szFromDir, MAX_BUF, PP_PATH_ONLY);

  if((hFile = FindFirstFile(szFrom, &fdFile)) == INVALID_HANDLE_VALUE)
    bFound = FALSE;
  else
    bFound = TRUE;

  while(bFound)
  {
    if((lstrcmpi(fdFile.cFileName, ".") != 0) && (lstrcmpi(fdFile.cFileName, "..") != 0))
    {
      
      lstrcpy(szFromTemp, szFromDir);
      AppendBackSlash(szFromTemp, sizeof(szFromTemp));
      lstrcat(szFromTemp, fdFile.cFileName);

      
      lstrcpy(szToTemp, szTo);
      AppendBackSlash(szToTemp, sizeof(szToTemp));
      lstrcat(szToTemp, fdFile.cFileName);

      MoveFile(szFromTemp, szToTemp);
    }

    bFound = FindNextFile(hFile, &fdFile);
  }

  FindClose(hFile);
  return(FO_SUCCESS);
}

HRESULT FileCopy(LPSTR szFrom, LPSTR szTo, BOOL bFailIfExists)
{
  HANDLE          hFile;
  WIN32_FIND_DATA fdFile;
  char            szFromDir[MAX_BUF];
  char            szFromTemp[MAX_BUF];
  char            szToTemp[MAX_BUF];
  char            szBuf[MAX_BUF];
  BOOL            bFound;

  if(FileExists(szFrom))
  {
    
    ParsePath(szFrom, szBuf, MAX_BUF, PP_FILENAME_ONLY);
    lstrcpy(szToTemp, szTo);
    AppendBackSlash(szToTemp, sizeof(szToTemp));
    lstrcat(szToTemp, szBuf);
    CopyFile(szFrom, szToTemp, bFailIfExists);
    return(FO_SUCCESS);
  }

  
  
  ParsePath(szFrom, szFromDir, MAX_BUF, PP_PATH_ONLY);

  if((hFile = FindFirstFile(szFrom, &fdFile)) == INVALID_HANDLE_VALUE)
    bFound = FALSE;
  else
    bFound = TRUE;

  while(bFound)
  {
    if((lstrcmpi(fdFile.cFileName, ".") != 0) && (lstrcmpi(fdFile.cFileName, "..") != 0))
    {
      
      lstrcpy(szFromTemp, szFromDir);
      AppendBackSlash(szFromTemp, sizeof(szFromTemp));
      lstrcat(szFromTemp, fdFile.cFileName);

      
      lstrcpy(szToTemp, szTo);
      AppendBackSlash(szToTemp, sizeof(szToTemp));
      lstrcat(szToTemp, fdFile.cFileName);

      CopyFile(szFromTemp, szToTemp, bFailIfExists);
    }

    bFound = FindNextFile(hFile, &fdFile);
  }

  FindClose(hFile);
  return(FO_SUCCESS);
}

HRESULT CreateDirectoriesAll(char* szPath)
{
  int     i;
  int     iLen = lstrlen(szPath);
  char    szCreatePath[MAX_BUF];
  HRESULT hrResult;

  ZeroMemory(szCreatePath, MAX_BUF);
  memcpy(szCreatePath, szPath, iLen);
  for(i = 0; i < iLen; i++)
  {
    if((iLen > 1) &&
      ((i != 0) && ((szPath[i] == '\\') || (szPath[i] == '/'))) &&
      (!((szPath[0] == '\\') && (i == 1)) && !((szPath[1] == ':') && (i == 2))))
    {
      szCreatePath[i] = '\0';
      hrResult        = CreateDirectory(szCreatePath, NULL);
      szCreatePath[i] = szPath[i];
    }
  }
  return(hrResult);
}

HRESULT FileDelete(LPSTR szDestination)
{
  HANDLE          hFile;
  WIN32_FIND_DATA fdFile;
  char            szBuf[MAX_BUF];
  char            szPathOnly[MAX_BUF];
  BOOL            bFound;

  if(FileExists(szDestination))
  {
    
    DeleteFile(szDestination);
    return(FO_SUCCESS);
  }

  
  
  ParsePath(szDestination, szPathOnly, MAX_BUF, PP_PATH_ONLY);

  if((hFile = FindFirstFile(szDestination, &fdFile)) == INVALID_HANDLE_VALUE)
    bFound = FALSE;
  else
    bFound = TRUE;

  while(bFound)
  {
    if(!(fdFile.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
    {
      lstrcpy(szBuf, szPathOnly);
      AppendBackSlash(szBuf, sizeof(szBuf));
      lstrcat(szBuf, fdFile.cFileName);

      DeleteFile(szBuf);
    }

    bFound = FindNextFile(hFile, &fdFile);
  }

  FindClose(hFile);
  return(FO_SUCCESS);
}

HRESULT DirectoryRemove(LPSTR szDestination, BOOL bRemoveSubdirs)
{
  HANDLE          hFile;
  WIN32_FIND_DATA fdFile;
  char            szDestTemp[MAX_BUF];
  BOOL            bFound;

  if(!FileExists(szDestination))
    return(FO_SUCCESS);

  if(bRemoveSubdirs == TRUE)
  {
    lstrcpy(szDestTemp, szDestination);
    AppendBackSlash(szDestTemp, sizeof(szDestTemp));
    lstrcat(szDestTemp, "*");

    bFound = TRUE;
    hFile = FindFirstFile(szDestTemp, &fdFile);
    while((hFile != INVALID_HANDLE_VALUE) && (bFound == TRUE))
    {
      if((lstrcmpi(fdFile.cFileName, ".") != 0) && (lstrcmpi(fdFile.cFileName, "..") != 0))
      {
        
        lstrcpy(szDestTemp, szDestination);
        AppendBackSlash(szDestTemp, sizeof(szDestTemp));
        lstrcat(szDestTemp, fdFile.cFileName);

        if(fdFile.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
          DirectoryRemove(szDestTemp, bRemoveSubdirs);
        }
        else
        {
          DeleteFile(szDestTemp);
        }
      }

      bFound = FindNextFile(hFile, &fdFile);
    }

    FindClose(hFile);
  }
  
  RemoveDirectory(szDestination);
  return(FO_SUCCESS);
}

HKEY ParseRootKey(LPSTR szRootKey)
{
  HKEY hkRootKey;

  if(lstrcmpi(szRootKey, "HKEY_CURRENT_CONFIG") == 0)
    hkRootKey = HKEY_CURRENT_CONFIG;
  else if(lstrcmpi(szRootKey, "HKEY_CURRENT_USER") == 0)
    hkRootKey = HKEY_CURRENT_USER;
  else if(lstrcmpi(szRootKey, "HKEY_LOCAL_MACHINE") == 0)
    hkRootKey = HKEY_LOCAL_MACHINE;
  else if(lstrcmpi(szRootKey, "HKEY_USERS") == 0)
    hkRootKey = HKEY_USERS;
  else if(lstrcmpi(szRootKey, "HKEY_PERFORMANCE_DATA") == 0)
    hkRootKey = HKEY_PERFORMANCE_DATA;
  else if(lstrcmpi(szRootKey, "HKEY_DYN_DATA") == 0)
    hkRootKey = HKEY_DYN_DATA;
  else 
    hkRootKey = HKEY_CLASSES_ROOT;

  return(hkRootKey);
}

LPSTR GetStringRootKey(HKEY hkRootKey, LPSTR szString, DWORD dwStringSize)
{
  if(hkRootKey == HKEY_CURRENT_CONFIG)
  {
    if(sizeof("HKEY_CURRENT_CONFIG") <= dwStringSize)
      lstrcpy(szString, "HKEY_CURRENT_CONFIG");
    else
      return(NULL);
  }
  else if(hkRootKey == HKEY_CURRENT_USER)
  {
    if(sizeof("HKEY_CURRENT_USER") <= dwStringSize)
      lstrcpy(szString, "HKEY_CURRENT_USER");
    else
      return(NULL);
  }
  else if(hkRootKey == HKEY_LOCAL_MACHINE)
  {
    if(sizeof("HKEY_LOCAL_MACHINE") <= dwStringSize)
      lstrcpy(szString, "HKEY_LOCAL_MACHINE");
    else
      return(NULL);
  }
  else if(hkRootKey == HKEY_USERS)
  {
    if(sizeof("HKEY_USERS") <= dwStringSize)
      lstrcpy(szString, "HKEY_USERS");
    else
      return(NULL);
  }
  else if(hkRootKey == HKEY_PERFORMANCE_DATA)
  {
    if(sizeof("HKEY_PERFORMANCE_DATA") <= dwStringSize)
      lstrcpy(szString, "HKEY_PERFORMANCE_DATA");
    else
      return(NULL);
  }
  else if(hkRootKey == HKEY_DYN_DATA)
  {
    if(sizeof("HKEY_DYN_DATA") <= dwStringSize)
      lstrcpy(szString, "HKEY_DYN_DATA");
    else
      return(NULL);
  }
  else
  {
    if(sizeof("HKEY_CLASSES_ROOT") <= dwStringSize)
      lstrcpy(szString, "HKEY_CLASSES_ROOT");
    else
      return(NULL);
  }

  return(szString);
}

BOOL WinRegKeyExists(HKEY hkRootKey, LPSTR szKey)
{
  HKEY  hkResult;
  BOOL  bKeyExists = FALSE;

  if(RegOpenKeyEx(hkRootKey, szKey, 0, KEY_READ, &hkResult) == ERROR_SUCCESS)
  {
    bKeyExists = TRUE;
    RegCloseKey(hkResult);
  }

  return(bKeyExists);
}

