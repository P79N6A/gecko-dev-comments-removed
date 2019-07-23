







































#include "extern.h"
#include "extra.h"
#include "dialogs.h"
#include "shortcut.h"
#include "ifuncns.h"
#include "wizverreg.h"
#include "logging.h"
#include <logkeys.h>

#define  SIZEOFNOTSTRING 4

HRESULT TimingCheck(DWORD dwTiming, LPSTR szSection, LPSTR szFile)
{
  char szBuf[MAX_BUF_TINY];

  GetPrivateProfileString(szSection, "Timing", "", szBuf, sizeof(szBuf), szFile);
  if(*szBuf != '\0')
  {
    switch(dwTiming)
    {
      case T_PRE_DOWNLOAD:
        if(lstrcmpi(szBuf, "pre download") == 0)
          return(TRUE);
        break;

      case T_POST_DOWNLOAD:
        if(lstrcmpi(szBuf, "post download") == 0)
          return(TRUE);
        break;

      case T_PRE_XPCOM:
        if(lstrcmpi(szBuf, "pre xpcom") == 0)
          return(TRUE);
        break;

      case T_POST_XPCOM:
        if(lstrcmpi(szBuf, "post xpcom") == 0)
          return(TRUE);
        break;

      case T_PRE_SMARTUPDATE:
        if(lstrcmpi(szBuf, "pre smartupdate") == 0)
          return(TRUE);
        break;

      case T_POST_SMARTUPDATE:
        if(lstrcmpi(szBuf, "post smartupdate") == 0)
          return(TRUE);
        break;

      case T_PRE_LAUNCHAPP:
        if(lstrcmpi(szBuf, "pre launchapp") == 0)
          return(TRUE);
        break;

      case T_POST_LAUNCHAPP:
        if(lstrcmpi(szBuf, "post launchapp") == 0)
          return(TRUE);
        break;

      case T_PRE_ARCHIVE:
        if(lstrcmpi(szBuf, "pre archive") == 0)
          return(TRUE);
        break;

      case T_POST_ARCHIVE:
        if(lstrcmpi(szBuf, "post archive") == 0)
          return(TRUE);
        break;

      case T_DEPEND_REBOOT:
        if(lstrcmpi(szBuf, "depend reboot") == 0)
          return(TRUE);
        break;
    }
  }
  return(FALSE);
}

HRESULT MeetCondition(LPSTR szSection)
{
  char szBuf[MAX_BUF_TINY];
  BOOL bResult = FALSE; 
  BOOL bNegateTheResult = FALSE;

  char *pszCondition = szBuf;

  GetPrivateProfileString(szSection, "Condition", "", szBuf, sizeof(szBuf), szFileIniConfig);

  
  if(pszCondition[0] == '\0')
    return TRUE;

  
  if(strncmp(pszCondition, "not ", SIZEOFNOTSTRING) == 0)
  {
    bNegateTheResult = TRUE;
    pszCondition = pszCondition + SIZEOFNOTSTRING;
  }

  
  
  if(strcmp(pszCondition, "DefaultApp") == 0)
  {
    GetPrivateProfileString("General", "Default AppID", "", szBuf, sizeof(szBuf), szFileIniConfig);
    if(strcmp(szBuf, sgProduct.szAppID) == 0)
      bResult = TRUE;
  }
  
  
  else if(strcmp(pszCondition, "RecaptureHPChecked") == 0)
  {
    bResult = diAdditionalOptions.bRecaptureHomepage;
  }

  if(bNegateTheResult)
    return !bResult;

  return bResult;
}

char *BuildNumberedString(DWORD dwIndex, char *szInputStringPrefix, char *szInputString, char *szOutBuf, DWORD dwOutBufSize)
{
  if((szInputStringPrefix) && (*szInputStringPrefix != '\0'))
    wsprintf(szOutBuf, "%s-%s%d", szInputStringPrefix, szInputString, dwIndex);
  else
    wsprintf(szOutBuf, "%s%d", szInputString, dwIndex);

  return(szOutBuf);
}

void GetUserAgentShort(char *szUserAgent, char *szOutUAShort, DWORD dwOutUAShortSize)
{
  char *ptrFirstSpace = NULL;

  ZeroMemory(szOutUAShort, dwOutUAShortSize);
  if((szUserAgent == NULL) || (*szUserAgent == '\0'))
    return;

  ptrFirstSpace = strstr(szUserAgent, " ");
  if(ptrFirstSpace != NULL)
  {
    *ptrFirstSpace = '\0';
    lstrcpy(szOutUAShort, szUserAgent);
    *ptrFirstSpace = ' ';
  }
}

DWORD GetWinRegSubKeyProductPath(HKEY hkRootKey, char *szInKey, char *szReturnSubKey, DWORD dwReturnSubKeySize, char *szInSubSubKey, char *szInName, char *szCompare, char *szInCurrentVersion)
{
  char      *szRv = NULL;
  char      szKey[MAX_BUF];
  char      szBuf[MAX_BUF];
  HKEY      hkHandle;
  DWORD     dwIndex;
  DWORD     dwBufSize;
  DWORD     dwTotalSubKeys;
  DWORD     dwTotalValues;
  FILETIME  ftLastWriteFileTime;
  BOOL      bFoundSubKey;

  bFoundSubKey = FALSE;

  if(RegOpenKeyEx(hkRootKey, szInKey, 0, KEY_READ, &hkHandle) != ERROR_SUCCESS)
  {
    *szReturnSubKey = '\0';
    return(0);
  }

  dwTotalSubKeys = 0;
  dwTotalValues  = 0;
  RegQueryInfoKey(hkHandle, NULL, NULL, NULL, &dwTotalSubKeys, NULL, NULL, &dwTotalValues, NULL, NULL, NULL, NULL);
  for(dwIndex = 0; dwIndex < dwTotalSubKeys; dwIndex++)
  {
    dwBufSize = dwReturnSubKeySize;
    if(RegEnumKeyEx(hkHandle, dwIndex, szReturnSubKey, &dwBufSize, NULL, NULL, NULL, &ftLastWriteFileTime) == ERROR_SUCCESS)
    {
      if(  (*szInCurrentVersion != '\0') && (lstrcmpi(szInCurrentVersion, szReturnSubKey) != 0)  )
      {
        








        if(szInSubSubKey && (*szInSubSubKey != '\0'))
          wsprintf(szKey, "%s\\%s\\%s", szInKey, szReturnSubKey, szInSubSubKey);
        else
          wsprintf(szKey, "%s\\%s", szInKey, szReturnSubKey);

        GetWinReg(hkRootKey, szKey, szInName, szBuf, sizeof(szBuf));
        AppendBackSlash(szBuf, sizeof(szBuf));
        if(lstrcmpi(szBuf, szCompare) == 0)
        {
          bFoundSubKey = TRUE;
          
          break;
        }
      }
    }
  }

  RegCloseKey(hkHandle);
  if(!bFoundSubKey)
    *szReturnSubKey = '\0';
  return(dwTotalSubKeys);
}

void CleanupPreviousVersionRegKeys(void)
{
  DWORD dwIndex = 0;
  DWORD dwSubKeyCount;
  char  szBufTiny[MAX_BUF_TINY];
  char  szKeyRoot[MAX_BUF_TINY];
  char  szCurrentVersion[MAX_BUF_TINY];
  char  szUAShort[MAX_BUF_TINY];
  char  szRvSubKey[MAX_PATH + 1];
  char  szPath[MAX_BUF];
  char  szKey[MAX_BUF];
  char  szCleanupProduct[MAX_BUF];
  HKEY  hkeyRoot;
  char  szSubSubKey[] = "Main";
  char  szName[] = "Install Directory";
  char  szWRMSUninstall[] = "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall";
  char  szSection[] = "Cleanup Previous Product RegKeys";

  lstrcpy(szPath, sgProduct.szPath);
  if(*sgProduct.szSubPath != '\0')
  {
    AppendBackSlash(szPath, sizeof(szPath));
    lstrcat(szPath, sgProduct.szSubPath);
  }
  AppendBackSlash(szPath, sizeof(szPath));

  wsprintf(szBufTiny, "Product Reg Key%d", dwIndex);        
  GetPrivateProfileString(szSection, szBufTiny, "", szKey, sizeof(szKey), szFileIniConfig);

  while(*szKey != '\0')
  {
    wsprintf(szBufTiny, "Reg Key Root%d",dwIndex);
    GetPrivateProfileString(szSection, szBufTiny, "", szKeyRoot, sizeof(szKeyRoot), szFileIniConfig);
    hkeyRoot = ParseRootKey(szKeyRoot);

    wsprintf(szBufTiny, "Product Name%d", dwIndex);        
    GetPrivateProfileString(szSection, szBufTiny, "", szCleanupProduct, sizeof(szCleanupProduct), szFileIniConfig);
    
    if(*szCleanupProduct == '\0')
      return;

    wsprintf(szBufTiny, "Current Version%d", dwIndex);        
    GetPrivateProfileString(szSection, szBufTiny, "", szCurrentVersion, sizeof(szCurrentVersion), szFileIniConfig);

    do
    {
      
      dwSubKeyCount = GetWinRegSubKeyProductPath(hkeyRoot, szKey, szRvSubKey, sizeof(szRvSubKey), szSubSubKey, szName, szPath, szCurrentVersion);
  	  
      if(*szRvSubKey != '\0')
      {
        if(dwSubKeyCount > 1)
        {
          AppendBackSlash(szKey, sizeof(szKey));
          lstrcat(szKey, szRvSubKey);
        }
        DeleteWinRegKey(hkeyRoot, szKey, TRUE);

        GetUserAgentShort(szRvSubKey, szUAShort, sizeof(szUAShort));
        if(*szUAShort != '\0')
        {
          



          wsprintf(szKey, "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\%s (%s)", szCleanupProduct, szUAShort);
          DeleteWinRegKey(hkeyRoot, szKey, TRUE);

          



          wsprintf(szKey, "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\%s %s", szCleanupProduct, szUAShort);
          DeleteWinRegKey(hkeyRoot, szKey, TRUE);

          



          wsprintf(szKey, "Software\\Mozilla\\%s %s", szCleanupProduct, szUAShort);
          DeleteWinRegKey(hkeyRoot, szKey, TRUE);

          







        }
        
        wsprintf(szBufTiny, "Product Reg Key%d", dwIndex);        
        GetPrivateProfileString(szSection, szBufTiny, "", szKey, sizeof(szKey), szFileIniConfig);
      }
    }  while (*szRvSubKey != '\0');
    wsprintf(szBufTiny, "Product Reg Key%d", ++dwIndex);        
    GetPrivateProfileString(szSection, szBufTiny, "", szKey, sizeof(szKey), szFileIniConfig);
  } 

}

void ProcessFileOps(DWORD dwTiming, char *szSectionPrefix)
{
  if(sgProduct.bInstallFiles)
  {
    ProcessUncompressFile(dwTiming, szSectionPrefix);
    ProcessCreateDirectory(dwTiming, szSectionPrefix);
    ProcessMoveFile(dwTiming, szSectionPrefix);
    ProcessCopyFile(dwTiming, szSectionPrefix);
    ProcessCopyFileSequential(dwTiming, szSectionPrefix);
    ProcessSelfRegisterFile(dwTiming, szSectionPrefix);
    ProcessDeleteFile(dwTiming, szSectionPrefix);
    ProcessRemoveDirectory(dwTiming, szSectionPrefix);
    if(!gbIgnoreRunAppX)
      ProcessRunApp(dwTiming, szSectionPrefix);
  }

  
  ProcessWinReg(dwTiming, szSectionPrefix);

  if(sgProduct.bInstallFiles)
  {
    ProcessProgramFolder(dwTiming, szSectionPrefix);
    ProcessSetVersionRegistry(dwTiming, szSectionPrefix);
  }
}

void ProcessFileOpsForSelectedComponents(DWORD dwTiming)
{
  DWORD dwIndex0;
  siC   *siCObject = NULL;

  dwIndex0  = 0;
  siCObject = SiCNodeGetObject(dwIndex0, TRUE, AC_ALL);
  while(siCObject)
  {
    if(siCObject->dwAttributes & SIC_SELECTED)
      
      ProcessFileOps(dwTiming, siCObject->szReferenceName);

    ++dwIndex0;
    siCObject = SiCNodeGetObject(dwIndex0, TRUE, AC_ALL);
  } 
}

void ProcessFileOpsForAll(DWORD dwTiming)
{
  ProcessFileOps(dwTiming, NULL);
  if(sgProduct.bInstallFiles)
  {
    ProcessFileOpsForSelectedComponents(dwTiming);
    ProcessCreateCustomFiles(dwTiming);
  }
}

int VerifyArchive(LPSTR szArchive)
{
  void *vZip;
  int  iTestRv;

  
  if(!FileExists(szArchive))
    return(FO_ERROR_FILE_NOT_FOUND);

  if((iTestRv = ZIP_OpenArchive(szArchive, &vZip)) == ZIP_OK)
  {
    
    
    iTestRv = ZIP_TestArchive(vZip);
    ZIP_CloseArchive(&vZip);
  }
  return(iTestRv);
}

HRESULT ProcessSetVersionRegistry(DWORD dwTiming, char *szSectionPrefix)
{
  DWORD   dwIndex;
  BOOL    bIsDirectory;
  char    szBuf[MAX_BUF];
  char    szSection[MAX_BUF_TINY];
  char    szRegistryKey[MAX_BUF];
  char    szPath[MAX_BUF];
  char    szVersion[MAX_BUF_TINY];

  dwIndex = 0;
  BuildNumberedString(dwIndex, szSectionPrefix, "Version Registry", szSection, sizeof(szSection));
  GetPrivateProfileString(szSection, "Registry Key", "", szRegistryKey, sizeof(szRegistryKey), szFileIniConfig);
  while(*szRegistryKey != '\0')
  {
    if(TimingCheck(dwTiming, szSection, szFileIniConfig))
    {
      GetPrivateProfileString(szSection, "Version", "", szVersion, sizeof(szVersion), szFileIniConfig);
      GetPrivateProfileString(szSection, "Path",    "", szBuf,     sizeof(szBuf),     szFileIniConfig);
      DecryptString(szPath, szBuf);
      if(FileExists(szPath) & FILE_ATTRIBUTE_DIRECTORY)
        bIsDirectory = TRUE;
      else
        bIsDirectory = FALSE;

      lstrcpy(szBuf, sgProduct.szPath);
      if(sgProduct.szSubPath != '\0')
      {
        AppendBackSlash(szBuf, sizeof(szBuf));
        lstrcat(szBuf, sgProduct.szSubPath);
      }

      VR_CreateRegistry(VR_DEFAULT_PRODUCT_NAME, szBuf, NULL);
      VR_Install(szRegistryKey, szPath, szVersion, bIsDirectory);
      VR_Close();
    }

    ++dwIndex;
    BuildNumberedString(dwIndex, szSectionPrefix, "Version Registry", szSection, sizeof(szSection));
    GetPrivateProfileString(szSection, "Registry Key", "", szRegistryKey, sizeof(szRegistryKey), szFileIniConfig);
  }
  return(FO_SUCCESS);
}

HRESULT FileUncompress(LPSTR szFrom, LPSTR szTo)
{
  char  szBuf[MAX_BUF];
  DWORD dwReturn;
  void  *vZip;

  dwReturn = FO_SUCCESS;
  
  if(!FileExists(szFrom))
    return(FO_ERROR_FILE_NOT_FOUND);

  
  dwReturn = FileExists(szTo);
  if(dwReturn && !(dwReturn & FILE_ATTRIBUTE_DIRECTORY))
  {
    
    return(FO_ERROR_DESTINATION_CONFLICT);
  }
  else if(!dwReturn)
  {
    lstrcpy(szBuf, szTo);
    AppendBackSlash(szBuf, sizeof(szBuf));
    CreateDirectoriesAll(szBuf, DO_NOT_ADD_TO_UNINSTALL_LOG);
  }

  GetCurrentDirectory(MAX_BUF, szBuf);
  if(SetCurrentDirectory(szTo) == FALSE)
    return(FO_ERROR_CHANGE_DIR);

  if((dwReturn = ZIP_OpenArchive(szFrom, &vZip)) != ZIP_OK)
    return(dwReturn);

  
  
  dwReturn = ExtractDirEntries(NULL, vZip);
  ZIP_CloseArchive(&vZip);

  if(SetCurrentDirectory(szBuf) == FALSE)
    return(FO_ERROR_CHANGE_DIR);

  return(dwReturn);
}

HRESULT ProcessXpcomFile()
{
  char szSource[MAX_BUF];
  char szDestination[MAX_BUF];
  DWORD dwErr;

  if((dwErr = FileUncompress(siCFXpcomFile.szSource, siCFXpcomFile.szDestination)) != FO_SUCCESS)
  {
    char szMsg[MAX_BUF];
    char szErrorString[MAX_BUF];

    LogISProcessXpcomFile(LIS_FAILURE, dwErr);
    GetPrivateProfileString("Strings", "Error File Uncompress", "", szErrorString, sizeof(szErrorString), szFileIniConfig);
    wsprintf(szMsg, szErrorString, siCFXpcomFile.szSource, dwErr);
    PrintError(szMsg, ERROR_CODE_HIDE);
    return(dwErr);
  }
  LogISProcessXpcomFile(LIS_SUCCESS, dwErr);

  


  lstrcpy(szSource, siCFXpcomFile.szDestination);
  AppendBackSlash(szSource, sizeof(szSource));
  lstrcat(szSource, "ms*.dll");

  lstrcpy(szDestination, siCFXpcomFile.szDestination);
  AppendBackSlash(szDestination, sizeof(szDestination));
  lstrcat(szDestination, "bin");

  FileCopy(szSource, szDestination, TRUE, FALSE);

  return(FO_SUCCESS);
}

void CleanupXpcomFile()
{
  

  if(siCFXpcomFile.bStatus != STATUS_ENABLED)
    return;

  if(siCFXpcomFile.bCleanup == TRUE)
    DirectoryRemove(siCFXpcomFile.szDestination, TRUE);

  return;
}

#define SETUP_STATE_REG_KEY "Software\\%s\\%s\\%s\\Setup"

HRESULT CleanupArgsRegistry()
{
  char  szKey[MAX_BUF];

  wsprintf(szKey, SETUP_STATE_REG_KEY, sgProduct.szCompanyName, sgProduct.szProductNameInternal,
    sgProduct.szUserAgent);
  DeleteWinRegValue(HKEY_CURRENT_USER, szKey, "browserargs");
  return(FO_SUCCESS);
}

HRESULT ProcessUncompressFile(DWORD dwTiming, char *szSectionPrefix)
{
  DWORD   dwIndex;
  BOOL    bOnlyIfExists;
  char    szBuf[MAX_BUF];
  char    szSection[MAX_BUF];
  char    szSource[MAX_BUF];
  char    szDestination[MAX_BUF];

  dwIndex = 0;
  BuildNumberedString(dwIndex, szSectionPrefix, "Uncompress File", szSection, sizeof(szSection));
  GetPrivateProfileString(szSection, "Source", "", szBuf, sizeof(szBuf), szFileIniConfig);
  while(*szBuf != '\0')
  {
    if(TimingCheck(dwTiming, szSection, szFileIniConfig))
    {
      DecryptString(szSource, szBuf);
      GetPrivateProfileString(szSection, "Destination", "", szBuf, sizeof(szBuf), szFileIniConfig);
      DecryptString(szDestination, szBuf);
      GetPrivateProfileString(szSection, "Only If Exists", "", szBuf, sizeof(szBuf), szFileIniConfig);
      if(lstrcmpi(szBuf, "TRUE") == 0)
        bOnlyIfExists = TRUE;
      else
        bOnlyIfExists = FALSE;

      if((!bOnlyIfExists) || (bOnlyIfExists && FileExists(szDestination)))
      {
        DWORD dwErr;

        GetPrivateProfileString(szSection, "Message",     "", szBuf, sizeof(szBuf), szFileIniConfig);
        ShowMessage(szBuf, TRUE);
        if((dwErr = FileUncompress(szSource, szDestination)) != FO_SUCCESS)
        {
          char szMsg[MAX_BUF];
          char szErrorString[MAX_BUF];

          GetPrivateProfileString("Strings", "Error File Uncompress", "", szErrorString, sizeof(szErrorString), szFileIniConfig);
          wsprintf(szMsg, szErrorString, szSource, dwErr);
          PrintError(szMsg, ERROR_CODE_HIDE);
          return(dwErr);
        }
      }
    }

    ++dwIndex;
    BuildNumberedString(dwIndex, szSectionPrefix, "Uncompress File", szSection, sizeof(szSection));
    GetPrivateProfileString(szSection, "Source", "", szBuf, sizeof(szBuf), szFileIniConfig);
  }
  return(FO_SUCCESS);
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

    
    wsprintf(szBuf, "%s to %s", szFrom, szTo);
    UpdateInstallLog(KEY_MOVE_FILE, szBuf, FALSE);

    return(FO_SUCCESS);
  }
  
  if(FileExists(szFrom) && FileExists(szTo))
  {
    
    
    
    lstrcpy(szToTemp, szTo);
    AppendBackSlash(szToTemp, sizeof(szToTemp));
    ParsePath(szFrom, szBuf, sizeof(szBuf), FALSE, PP_FILENAME_ONLY);
    lstrcat(szToTemp, szBuf);
    MoveFile(szFrom, szToTemp);

    
    wsprintf(szBuf, "%s to %s", szFrom, szToTemp);
    UpdateInstallLog(KEY_MOVE_FILE, szBuf, FALSE);

    return(FO_SUCCESS);
  }

  ParsePath(szFrom, szFromDir, sizeof(szFromDir), FALSE, PP_PATH_ONLY);

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

      
      wsprintf(szBuf, "%s to %s", szFromTemp, szToTemp);
      UpdateInstallLog(KEY_MOVE_FILE, szBuf, FALSE);
    }

    bFound = FindNextFile(hFile, &fdFile);
  }

  FindClose(hFile);
  return(FO_SUCCESS);
}

HRESULT ProcessMoveFile(DWORD dwTiming, char *szSectionPrefix)
{
  DWORD dwIndex;
  char  szBuf[MAX_BUF];
  char  szSection[MAX_BUF];
  char  szSource[MAX_BUF];
  char  szDestination[MAX_BUF];

  dwIndex = 0;
  BuildNumberedString(dwIndex, szSectionPrefix, "Move File", szSection, sizeof(szSection));
  GetPrivateProfileString(szSection, "Source", "", szBuf, sizeof(szBuf), szFileIniConfig);
  while(*szBuf != '\0')
  {
    if(TimingCheck(dwTiming, szSection, szFileIniConfig))
    {
      DecryptString(szSource, szBuf);
      GetPrivateProfileString(szSection, "Destination", "", szBuf, sizeof(szBuf), szFileIniConfig);
      DecryptString(szDestination, szBuf);
      FileMove(szSource, szDestination);
    }

    ++dwIndex;
    BuildNumberedString(dwIndex, szSectionPrefix, "Move File", szSection, sizeof(szSection));
    GetPrivateProfileString(szSection, "Source", "", szBuf, sizeof(szBuf), szFileIniConfig);
  }
  return(FO_SUCCESS);
}

HRESULT FileCopy(LPSTR szFrom, LPSTR szTo, BOOL bFailIfExists, BOOL bDnu)
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
    
    CreateDirectoriesAll(szTo, !bDnu);
    ParsePath(szFrom, szBuf, sizeof(szBuf), FALSE, PP_FILENAME_ONLY);
    lstrcpy(szToTemp, szTo);
    AppendBackSlash(szToTemp, sizeof(szToTemp));
    lstrcat(szToTemp, szBuf);
    CopyFile(szFrom, szToTemp, bFailIfExists);
    wsprintf(szBuf, "%s to %s", szFrom, szToTemp);
    UpdateInstallLog(KEY_COPY_FILE, szBuf, bDnu);

    return(FO_SUCCESS);
  }

  
  
  ParsePath(szFrom, szFromDir, sizeof(szFromDir), FALSE, PP_PATH_ONLY);

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

      
      wsprintf(szBuf, "%s to %s", szFromTemp, szToTemp);
      UpdateInstallLog(KEY_COPY_FILE, szBuf, bDnu);
    }

    bFound = FindNextFile(hFile, &fdFile);
  }

  FindClose(hFile);
  return(FO_SUCCESS);
}

HRESULT FileCopySequential(LPSTR szSourcePath, LPSTR szDestPath, LPSTR szFilename)
{
  int             iFilenameOnlyLen;
  char            szDestFullFilename[MAX_BUF];
  char            szSourceFullFilename[MAX_BUF];
  char            szSearchFilename[MAX_BUF];
  char            szSearchDestFullFilename[MAX_BUF];
  char            szFilenameOnly[MAX_BUF];
  char            szFilenameExtensionOnly[MAX_BUF];
  char            szNumber[MAX_BUF];
  long            dwNumber;
  long            dwMaxNumber;
  LPSTR           szDotPtr;
  HANDLE          hFile;
  WIN32_FIND_DATA fdFile;
  BOOL            bFound;

  lstrcpy(szSourceFullFilename, szSourcePath);
  AppendBackSlash(szSourceFullFilename, sizeof(szSourceFullFilename));
  lstrcat(szSourceFullFilename, szFilename);

  if(FileExists(szSourceFullFilename))
  {
    
    ZeroMemory(szSearchFilename,        sizeof(szSearchFilename));
    ZeroMemory(szFilenameOnly,          sizeof(szFilenameOnly));
    ZeroMemory(szFilenameExtensionOnly, sizeof(szFilenameExtensionOnly));

    
    if((szDotPtr = strstr(szFilename, ".")) != NULL)
    {
      *szDotPtr = '\0';
      lstrcpy(szSearchFilename, szFilename);
      lstrcpy(szFilenameOnly, szFilename);
      lstrcpy(szFilenameExtensionOnly, &szDotPtr[1]);
      *szDotPtr = '.';
    }
    else
    {
      lstrcpy(szFilenameOnly, szFilename);
      lstrcpy(szSearchFilename, szFilename);
    }

    
    lstrcat(szSearchFilename, "*.*");
    lstrcpy(szSearchDestFullFilename, szDestPath);
    AppendBackSlash(szSearchDestFullFilename, sizeof(szSearchDestFullFilename));
    lstrcat(szSearchDestFullFilename, szSearchFilename);

    iFilenameOnlyLen = lstrlen(szFilenameOnly);
    dwNumber         = 0;
    dwMaxNumber      = 0;

    
    if((hFile = FindFirstFile(szSearchDestFullFilename, &fdFile)) == INVALID_HANDLE_VALUE)
      bFound = FALSE;
    else
      bFound = TRUE;

    while(bFound)
    {
       ZeroMemory(szNumber, sizeof(szNumber));
      if((lstrcmpi(fdFile.cFileName, ".") != 0) && (lstrcmpi(fdFile.cFileName, "..") != 0))
      {
        lstrcpy(szNumber, &fdFile.cFileName[iFilenameOnlyLen]);
        dwNumber = atoi(szNumber);
        if(dwNumber > dwMaxNumber)
          dwMaxNumber = dwNumber;
      }

      bFound = FindNextFile(hFile, &fdFile);
    }

    FindClose(hFile);

    lstrcpy(szDestFullFilename, szDestPath);
    AppendBackSlash(szDestFullFilename, sizeof(szDestFullFilename));
    lstrcat(szDestFullFilename, szFilenameOnly);
    itoa(dwMaxNumber + 1, szNumber, 10);
    lstrcat(szDestFullFilename, szNumber);

    if(*szFilenameExtensionOnly != '\0')
    {
      lstrcat(szDestFullFilename, ".");
      lstrcat(szDestFullFilename, szFilenameExtensionOnly);
    }

    CopyFile(szSourceFullFilename, szDestFullFilename, TRUE);
  }

  return(FO_SUCCESS);
}

HRESULT ProcessCopyFile(DWORD dwTiming, char *szSectionPrefix)
{
  DWORD dwIndex;
  char  szBuf[MAX_BUF];
  char  szSection[MAX_BUF];
  char  szSource[MAX_BUF];
  char  szDestination[MAX_BUF];
  BOOL  bFailIfExists;
  BOOL  bDnu;

  dwIndex = 0;
  BuildNumberedString(dwIndex, szSectionPrefix, "Copy File", szSection, sizeof(szSection));
  GetPrivateProfileString(szSection, "Source", "", szBuf, sizeof(szBuf), szFileIniConfig);
  while(*szBuf != '\0')
  {
    if(TimingCheck(dwTiming, szSection, szFileIniConfig))
    {
      DecryptString(szSource, szBuf);
      GetPrivateProfileString(szSection, "Destination", "", szBuf, sizeof(szBuf), szFileIniConfig);
      DecryptString(szDestination, szBuf);

      GetPrivateProfileString(szSection, "Do Not Uninstall", "", szBuf, sizeof(szBuf), szFileIniConfig);
      if(lstrcmpi(szBuf, "TRUE") == 0)
        bDnu = TRUE;
      else
        bDnu = FALSE;

      GetPrivateProfileString(szSection, "Fail If Exists", "", szBuf, sizeof(szBuf), szFileIniConfig);
      if(lstrcmpi(szBuf, "TRUE") == 0)
        bFailIfExists = TRUE;
      else
        bFailIfExists = FALSE;

      FileCopy(szSource, szDestination, bFailIfExists, bDnu);
    }

    ++dwIndex;
    BuildNumberedString(dwIndex, szSectionPrefix, "Copy File", szSection, sizeof(szSection));
    GetPrivateProfileString(szSection, "Source", "", szBuf, sizeof(szBuf), szFileIniConfig);
  }
  return(FO_SUCCESS);
}

HRESULT ProcessCopyFileSequential(DWORD dwTiming, char *szSectionPrefix)
{
  DWORD dwIndex;
  char  szBuf[MAX_BUF];
  char  szSection[MAX_BUF];
  char  szSource[MAX_BUF];
  char  szDestination[MAX_BUF];
  char  szFilename[MAX_BUF];

  dwIndex = 0;
  BuildNumberedString(dwIndex, szSectionPrefix, "Copy File Sequential", szSection, sizeof(szSection));
  GetPrivateProfileString(szSection, "Filename", "", szFilename, sizeof(szFilename), szFileIniConfig);
  while(*szFilename != '\0')
  {
    if(TimingCheck(dwTiming, szSection, szFileIniConfig))
    {
      GetPrivateProfileString(szSection, "Source", "", szBuf, sizeof(szBuf), szFileIniConfig);
      DecryptString(szSource, szBuf);

      GetPrivateProfileString(szSection, "Destination", "", szBuf, sizeof(szBuf), szFileIniConfig);
      DecryptString(szDestination, szBuf);

      FileCopySequential(szSource, szDestination, szFilename);
    }

    ++dwIndex;
    BuildNumberedString(dwIndex, szSectionPrefix, "Copy File Sequential", szSection, sizeof(szSection));
    GetPrivateProfileString(szSection, "Filename", "", szFilename, sizeof(szFilename), szFileIniConfig);
  }
  return(FO_SUCCESS);
}

int RegisterDll32(char *File)
{
  FARPROC   DllReg;
  HINSTANCE hLib;

  if((hLib = LoadLibraryEx(File, NULL, LOAD_WITH_ALTERED_SEARCH_PATH)) != NULL)
  {
    if((DllReg = GetProcAddress(hLib, "DllRegisterServer")) != NULL)
      DllReg();

    FreeLibrary(hLib);
    return(0);
  }

  return(1);
}


HRESULT FileSelfRegister(LPSTR szFilename, LPSTR szDestination)
{
  char            szFullFilenamePath[MAX_BUF];
  DWORD           dwRv;
  HANDLE          hFile;
  WIN32_FIND_DATA fdFile;
  BOOL            bFound;

  lstrcpy(szFullFilenamePath, szDestination);
  AppendBackSlash(szFullFilenamePath, sizeof(szFullFilenamePath));
  lstrcat(szFullFilenamePath, szFilename);

  
  if(FileExists(szFullFilenamePath))
  {
    RegisterDll32(szFullFilenamePath);
    return(FO_SUCCESS);
  }

  lstrcpy(szFullFilenamePath, szDestination);
  AppendBackSlash(szFullFilenamePath, sizeof(szFullFilenamePath));
  lstrcat(szFullFilenamePath, szFilename);

  if((hFile = FindFirstFile(szFullFilenamePath, &fdFile)) == INVALID_HANDLE_VALUE)
    bFound = FALSE;
  else
    bFound = TRUE;

  while(bFound)
  {
    if((lstrcmpi(fdFile.cFileName, ".") != 0) && (lstrcmpi(fdFile.cFileName, "..") != 0))
    {
      
      lstrcpy(szFullFilenamePath, szDestination);
      AppendBackSlash(szFullFilenamePath, sizeof(szFullFilenamePath));
      lstrcat(szFullFilenamePath, fdFile.cFileName);

      if((dwRv = FileExists(szFullFilenamePath)) && (dwRv != FILE_ATTRIBUTE_DIRECTORY))
        RegisterDll32(szFullFilenamePath);
    }

    bFound = FindNextFile(hFile, &fdFile);
  }

  FindClose(hFile);
  return(FO_SUCCESS);
}

HRESULT ProcessSelfRegisterFile(DWORD dwTiming, char *szSectionPrefix)
{
  DWORD dwIndex;
  char  szBuf[MAX_BUF];
  char  szSection[MAX_BUF];
  char  szFilename[MAX_BUF];
  char  szDestination[MAX_BUF];

  dwIndex = 0;
  BuildNumberedString(dwIndex, szSectionPrefix, "Self Register File", szSection, sizeof(szSection));
  GetPrivateProfileString(szSection, "Destination", "", szBuf, sizeof(szBuf), szFileIniConfig);
  while(*szBuf != '\0')
  {
    if(TimingCheck(dwTiming, szSection, szFileIniConfig))
    {
      DecryptString(szDestination, szBuf);
      GetPrivateProfileString(szSection, "Filename", "", szFilename, sizeof(szFilename), szFileIniConfig);
      FileSelfRegister(szFilename, szDestination);
    }

    ++dwIndex;
    BuildNumberedString(dwIndex, szSectionPrefix, "Self Register File", szSection, sizeof(szSection));
    GetPrivateProfileString(szSection, "Destination", "", szBuf, sizeof(szBuf), szFileIniConfig);
  }
  return(FO_SUCCESS);
}

void UpdateInstallLog(LPSTR szKey, LPSTR szString, BOOL bDnu)
{
  FILE *fInstallLog;
  char szBuf[MAX_BUF];
  char szFileInstallLog[MAX_BUF];

  if(gbILUseTemp)
  {
    lstrcpy(szFileInstallLog, szTempDir);
    AppendBackSlash(szFileInstallLog, sizeof(szFileInstallLog));
  }
  else
  {
    lstrcpy(szFileInstallLog, sgProduct.szPath);
    AppendBackSlash(szFileInstallLog, sizeof(szFileInstallLog));
    lstrcat(szFileInstallLog, sgProduct.szSubPath);
    AppendBackSlash(szFileInstallLog, sizeof(szFileInstallLog));
  }

  CreateDirectoriesAll(szFileInstallLog, !bDnu);
  lstrcat(szFileInstallLog, FILE_INSTALL_LOG);

  if((fInstallLog = fopen(szFileInstallLog, "a+t")) != NULL)
  {
    if(bDnu)
      wsprintf(szBuf, "     ** (*dnu*) %s%s\n", szKey, szString);
    else
      wsprintf(szBuf, "     ** %s%s\n", szKey, szString);

    fwrite(szBuf, sizeof(char), lstrlen(szBuf), fInstallLog);
    fclose(fInstallLog);
  }
}

void UpdateInstallStatusLog(LPSTR szString)
{
  FILE *fInstallLog;
  char szFileInstallStatusLog[MAX_BUF];

  if(gbILUseTemp)
  {
    lstrcpy(szFileInstallStatusLog, szTempDir);
    AppendBackSlash(szFileInstallStatusLog, sizeof(szFileInstallStatusLog));
  }
  else
  {
    lstrcpy(szFileInstallStatusLog, sgProduct.szPath);
    AppendBackSlash(szFileInstallStatusLog, sizeof(szFileInstallStatusLog));
    lstrcat(szFileInstallStatusLog, sgProduct.szSubPath);
    AppendBackSlash(szFileInstallStatusLog, sizeof(szFileInstallStatusLog));
  }

  CreateDirectoriesAll(szFileInstallStatusLog, DO_NOT_ADD_TO_UNINSTALL_LOG);
  lstrcat(szFileInstallStatusLog, FILE_INSTALL_STATUS_LOG);

  if((fInstallLog = fopen(szFileInstallStatusLog, "a+t")) != NULL)
  {
    fwrite(szString, sizeof(char), lstrlen(szString), fInstallLog);
    fclose(fInstallLog);
  }
}

void UpdateJSProxyInfo()
{
  FILE *fJSFile;
  char szBuf[MAX_BUF];
  char szJSFile[MAX_BUF];

  if((*diAdvancedSettings.szProxyServer != '\0') || (*diAdvancedSettings.szProxyPort != '\0'))
  {
    lstrcpy(szJSFile, sgProduct.szPath);
    if(*sgProduct.szSubPath != '\0')
    {
      AppendBackSlash(szJSFile, sizeof(szJSFile));
      lstrcat(szJSFile, sgProduct.szSubPath);
    }
    AppendBackSlash(szJSFile, sizeof(szJSFile));
    lstrcat(szJSFile, "defaults\\pref\\");
    CreateDirectoriesAll(szJSFile, ADD_TO_UNINSTALL_LOG);
    lstrcat(szJSFile, FILE_ALL_JS);

    if((fJSFile = fopen(szJSFile, "a+t")) != NULL)
    {
      ZeroMemory(szBuf, sizeof(szBuf));
      if(*diAdvancedSettings.szProxyServer != '\0')
      {
        if(diAdditionalOptions.dwUseProtocol == UP_FTP)
          wsprintf(szBuf,
                   "pref(\"network.proxy.ftp\", \"%s\");\n",
                   diAdvancedSettings.szProxyServer);
        else
          wsprintf(szBuf,
                   "pref(\"network.proxy.http\", \"%s\");\n",
                   diAdvancedSettings.szProxyServer);
      }

      if(*diAdvancedSettings.szProxyPort != '\0')
      {
        if(diAdditionalOptions.dwUseProtocol == UP_FTP)
          wsprintf(szBuf,
                   "pref(\"network.proxy.ftp_port\", %s);\n",
                   diAdvancedSettings.szProxyPort);
        else
          wsprintf(szBuf,
                   "pref(\"network.proxy.http_port\", %s);\n",
                   diAdvancedSettings.szProxyPort);
      }

      lstrcat(szBuf, "pref(\"network.proxy.type\", 1);\n");

      fwrite(szBuf, sizeof(char), lstrlen(szBuf), fJSFile);
      fclose(fJSFile);
    }
  }
}










HRESULT DirHasWriteAccess(char *aPath)
{
  int     i;
  int     iLen = lstrlen(aPath);
  char    szCreatePath[MAX_BUF];

  ZeroMemory(szCreatePath, sizeof(szCreatePath));
  memcpy(szCreatePath, aPath, iLen);
  for(i = 0; i < iLen; i++)
  {
    if((iLen > 1) &&
      ((i != 0) && ((aPath[i] == '\\') || (aPath[i] == '/'))) &&
      (!((aPath[0] == '\\') && (i == 1)) && !((aPath[1] == ':') && (i == 2))))
    {
      szCreatePath[i] = '\0';
      if(FileExists(szCreatePath) == FALSE)
      {
        if(!CreateDirectory(szCreatePath, NULL))
          return(WIZ_ERROR_CREATE_DIRECTORY);

        RemoveDirectory(szCreatePath);
        return(WIZ_OK);
      }
      szCreatePath[i] = aPath[i];
    }
  }

  

  AppendBackSlash(szCreatePath, sizeof(szCreatePath));
  lstrcat(szCreatePath, "testdir");
  if(!CreateDirectory(szCreatePath, NULL))
    return(WIZ_ERROR_CREATE_DIRECTORY);

  RemoveDirectory(szCreatePath);
  return(WIZ_OK);
}

HRESULT CreateDirectoriesAll(char* szPath, BOOL bLogForUninstall)
{
  int     i;
  int     iLen = lstrlen(szPath);
  char    szCreatePath[MAX_BUF];
  HRESULT hrResult = WIZ_OK;

  ZeroMemory(szCreatePath, MAX_BUF);
  memcpy(szCreatePath, szPath, iLen);
  for(i = 0; i < iLen; i++)
  {
    if((iLen > 1) &&
      ((i != 0) && ((szPath[i] == '\\') || (szPath[i] == '/'))) &&
      (!((szPath[0] == '\\') && (i == 1)) && !((szPath[1] == ':') && (i == 2))))
    {
      szCreatePath[i] = '\0';
      if(FileExists(szCreatePath) == FALSE)
      {
        if(!CreateDirectory(szCreatePath, NULL))
          return(WIZ_ERROR_CREATE_DIRECTORY);

        if(bLogForUninstall)
          UpdateInstallLog(KEY_CREATE_FOLDER, szCreatePath, FALSE);
      }
      szCreatePath[i] = szPath[i];
    }
  }
  return(hrResult);
}

HRESULT ProcessCreateDirectory(DWORD dwTiming, char *szSectionPrefix)
{
  DWORD dwIndex;
  char  szBuf[MAX_BUF];
  char  szSection[MAX_BUF];
  char  szDestination[MAX_BUF];

  dwIndex = 0;
  BuildNumberedString(dwIndex, szSectionPrefix, "Create Directory", szSection, sizeof(szSection));
  GetPrivateProfileString(szSection, "Destination", "", szBuf, sizeof(szBuf), szFileIniConfig);
  while(*szBuf != '\0')
  {
    if(TimingCheck(dwTiming, szSection, szFileIniConfig))
    {
      DecryptString(szDestination, szBuf);
      AppendBackSlash(szDestination, sizeof(szDestination));
      CreateDirectoriesAll(szDestination, ADD_TO_UNINSTALL_LOG);
    }

    ++dwIndex;
    BuildNumberedString(dwIndex, szSectionPrefix, "Create Directory", szSection, sizeof(szSection));
    GetPrivateProfileString(szSection, "Destination", "", szBuf, sizeof(szBuf), szFileIniConfig);
  }
  return(FO_SUCCESS);
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

  
  
  ParsePath(szDestination, szPathOnly, sizeof(szPathOnly), FALSE, PP_PATH_ONLY);

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

HRESULT ProcessDeleteFile(DWORD dwTiming, char *szSectionPrefix)
{
  DWORD dwIndex;
  char  szBuf[MAX_BUF];
  char  szSection[MAX_BUF];
  char  szDestination[MAX_BUF];

  dwIndex = 0;
  BuildNumberedString(dwIndex, szSectionPrefix, "Delete File", szSection, sizeof(szSection));
  GetPrivateProfileString(szSection, "Destination", "", szBuf, sizeof(szBuf), szFileIniConfig);
  while(*szBuf != '\0')
  {
    if(TimingCheck(dwTiming, szSection, szFileIniConfig))
    {
      DecryptString(szDestination, szBuf);
      FileDelete(szDestination);
    }

    ++dwIndex;
    BuildNumberedString(dwIndex, szSectionPrefix, "Delete File", szSection, sizeof(szSection));
    GetPrivateProfileString(szSection, "Destination", "", szBuf, sizeof(szBuf), szFileIniConfig);
  }
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

HRESULT ProcessRemoveDirectory(DWORD dwTiming, char *szSectionPrefix)
{
  DWORD dwIndex;
  char  szBuf[MAX_BUF];
  char  szSection[MAX_BUF];
  char  szDestination[MAX_BUF];
  BOOL  bRemoveSubdirs;

  dwIndex = 0;
  BuildNumberedString(dwIndex, szSectionPrefix, "Remove Directory", szSection, sizeof(szSection));
  GetPrivateProfileString(szSection, "Destination", "", szBuf, sizeof(szBuf), szFileIniConfig);
  while(*szBuf != '\0')
  {
    if(TimingCheck(dwTiming, szSection, szFileIniConfig))
    {
      DecryptString(szDestination, szBuf);
      GetPrivateProfileString(szSection, "Remove subdirs", "", szBuf, sizeof(szBuf), szFileIniConfig);
      bRemoveSubdirs = FALSE;
      if(lstrcmpi(szBuf, "TRUE") == 0)
        bRemoveSubdirs = TRUE;

      DirectoryRemove(szDestination, bRemoveSubdirs);
    }

    ++dwIndex;
    BuildNumberedString(dwIndex, szSectionPrefix, "Remove Directory", szSection, sizeof(szSection));
    GetPrivateProfileString(szSection, "Destination", "", szBuf, sizeof(szBuf), szFileIniConfig);
  }
  return(FO_SUCCESS);
}

HRESULT ProcessRunApp(DWORD dwTiming, char *szSectionPrefix)
{
  DWORD dwIndex;
  char  szBuf[MAX_BUF];
  char  szSection[MAX_BUF];
  char  szTarget[MAX_BUF];
  char  szParameters[MAX_BUF];
  char  szWorkingDir[MAX_BUF];
  BOOL  bRunApp;
  BOOL  bWait;

  dwIndex = 0;
  BuildNumberedString(dwIndex, szSectionPrefix, "RunApp", szSection, sizeof(szSection));
  GetPrivateProfileString(szSection, "Target", "", szBuf, sizeof(szBuf), szFileIniConfig);
  while(*szBuf != '\0')
  {
    if(TimingCheck(dwTiming, szSection, szFileIniConfig))
    {
      DecryptString(szTarget, szBuf);
      GetPrivateProfileString(szSection, "Parameters", "", szBuf, sizeof(szBuf), szFileIniConfig);
      DecryptString(szParameters, szBuf);

      bRunApp = MeetCondition(szSection);

      GetPrivateProfileString(szSection, "WorkingDir", "", szBuf, sizeof(szBuf), szFileIniConfig);
      DecryptString(szWorkingDir, szBuf);

      GetPrivateProfileString(szSection, "Wait", "", szBuf, sizeof(szBuf), szFileIniConfig);
      if(lstrcmpi(szBuf, "FALSE") == 0)
        bWait = FALSE;
      else
        bWait = TRUE;

      if ((bRunApp == TRUE) && FileExists(szTarget))
      {
        if((dwTiming == T_DEPEND_REBOOT) && (NeedReboot() == TRUE))
        {
          lstrcat(szTarget, " ");
          lstrcat(szTarget, szParameters);
          SetWinReg(HKEY_CURRENT_USER,
                    "Software\\Microsoft\\Windows\\CurrentVersion\\RunOnce",
                    TRUE,
                    "Netscape",
                    TRUE,
                    REG_SZ,
                    szTarget,
                    lstrlen(szTarget),
                    FALSE,
                    FALSE);
        }
        else
        {
          GetPrivateProfileString(szSection, "Message", "", szBuf, sizeof(szBuf), szFileIniConfig);
          ShowMessage(szBuf, TRUE);  
          WinSpawn(szTarget, szParameters, szWorkingDir, SW_SHOWNORMAL, bWait);
        }
      }
    }

    ++dwIndex;
    BuildNumberedString(dwIndex, szSectionPrefix, "RunApp", szSection, sizeof(szSection));
    GetPrivateProfileString(szSection, "Target", "", szBuf, sizeof(szBuf), szFileIniConfig);
  }
  return(FO_SUCCESS);
}

DWORD ParseRestrictedAccessKey(LPSTR szKey)
{
  DWORD dwKey;

  if(lstrcmpi(szKey, "ONLY_RESTRICTED") == 0)
    dwKey = RA_ONLY_RESTRICTED;
  else if(lstrcmpi(szKey, "ONLY_NONRESTRICTED") == 0)
    dwKey = RA_ONLY_NONRESTRICTED;
  else
    dwKey = RA_IGNORE;

  return(dwKey);
}









LPSTR GetKeyInfo(LPSTR aKey, LPSTR aOut, DWORD aOutBufSize, DWORD aInfoType)
{
  LPSTR keyCopy = NULL;
  LPSTR key = NULL;

  *aOut = '\0';
  if((keyCopy = strdup(aKey)) == NULL)
    return NULL;

  switch(aInfoType)
  {
    case KEY_INFO_ROOT:
      key = MozStrChar(keyCopy, '\\');
      if(key == keyCopy)
      {
        
        free(keyCopy);
        return NULL;
      }
      else if(key)
        
        
        *key = '\0';

      if(MozCopyStr(keyCopy, aOut, aOutBufSize))
      {
        free(keyCopy);
        return NULL;
      }
      break;

    case KEY_INFO_SUBKEY:
      key = MozStrChar(keyCopy, '\\');
      if(key != NULL)
        ++key;

      if(!key)
        
        key = keyCopy;

      if(MozCopyStr(key, aOut, aOutBufSize))
      {
        free(keyCopy);
        return NULL;
      }
      break;
  }

  free(keyCopy);
  return(aOut);
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

char *ParseRootKeyString(HKEY hkKey, LPSTR szRootKey, DWORD dwRootKeyBufSize)
{
  if(!szRootKey)
    return(NULL);

  ZeroMemory(szRootKey, dwRootKeyBufSize);
  if((hkKey == HKEY_CURRENT_CONFIG) &&
    ((long)dwRootKeyBufSize > lstrlen("HKEY_CURRENT_CONFIG")))
    lstrcpy(szRootKey, "HKEY_CURRENT_CONFIG");
  else if((hkKey == HKEY_CURRENT_USER) &&
         ((long)dwRootKeyBufSize > lstrlen("HKEY_CURRENT_USER")))
    lstrcpy(szRootKey, "HKEY_CURRENT_USER");
  else if((hkKey == HKEY_LOCAL_MACHINE) &&
         ((long)dwRootKeyBufSize > lstrlen("HKEY_LOCAL_MACHINE")))
    lstrcpy(szRootKey, "HKEY_LOCAL_MACHINE");
  else if((hkKey == HKEY_USERS) &&
         ((long)dwRootKeyBufSize > lstrlen("HKEY_USERS")))
    lstrcpy(szRootKey, "HKEY_USERS");
  else if((hkKey == HKEY_PERFORMANCE_DATA) &&
         ((long)dwRootKeyBufSize > lstrlen("HKEY_PERFORMANCE_DATA")))
    lstrcpy(szRootKey, "HKEY_PERFORMANCE_DATA");
  else if((hkKey == HKEY_DYN_DATA) &&
         ((long)dwRootKeyBufSize > lstrlen("HKEY_DYN_DATA")))
    lstrcpy(szRootKey, "HKEY_DYN_DATA");
  else if((long)dwRootKeyBufSize > lstrlen("HKEY_CLASSES_ROOT"))
    lstrcpy(szRootKey, "HKEY_CLASSES_ROOT");

  return(szRootKey);
}

BOOL ParseRegType(LPSTR szType, DWORD *dwType)
{
  BOOL bSZ;

  if(lstrcmpi(szType, "REG_SZ") == 0)
  {
    
    *dwType = REG_SZ;
    bSZ     = TRUE;
  }
  else if(lstrcmpi(szType, "REG_EXPAND_SZ") == 0)
  {
    

    *dwType = REG_EXPAND_SZ;
    bSZ     = TRUE;
  }
  else if(lstrcmpi(szType, "REG_BINARY") == 0)
  {
    
    *dwType = REG_BINARY;
    bSZ     = FALSE;
  }
  else if(lstrcmpi(szType, "REG_DWORD") == 0)
  {
    
    *dwType = REG_DWORD;
    bSZ     = FALSE;
  }
  else if(lstrcmpi(szType, "REG_DWORD_LITTLE_ENDIAN") == 0)
  {
    

    *dwType = REG_DWORD_LITTLE_ENDIAN;
    bSZ     = FALSE;
  }
  else if(lstrcmpi(szType, "REG_DWORD_BIG_ENDIAN") == 0)
  {
    
    *dwType = REG_DWORD_BIG_ENDIAN;
    bSZ     = FALSE;
  }
  else if(lstrcmpi(szType, "REG_LINK") == 0)
  {
    
    *dwType = REG_LINK;
    bSZ     = TRUE;
  }
  else if(lstrcmpi(szType, "REG_MULTI_SZ") == 0)
  {
    
    *dwType = REG_MULTI_SZ;
    bSZ     = TRUE;
  }
  else 
  {
    
    *dwType = REG_NONE;
    bSZ     = TRUE;
  }

  return(bSZ);
}

BOOL WinRegKeyExists(HKEY hkRootKey, LPSTR szKey)
{
  HKEY  hkResult;
  DWORD dwErr;
  BOOL  bKeyExists = FALSE;

  if((dwErr = RegOpenKeyEx(hkRootKey, szKey, 0, KEY_READ, &hkResult)) == ERROR_SUCCESS)
  {
    bKeyExists = TRUE;
    RegCloseKey(hkResult);
  }

  return(bKeyExists);
}

BOOL WinRegNameExists(HKEY hkRootKey, LPSTR szKey, LPSTR szName)
{
  HKEY  hkResult;
  DWORD dwErr;
  DWORD dwSize;
  char  szBuf[MAX_BUF];
  BOOL  bNameExists = FALSE;

  ZeroMemory(szBuf, sizeof(szBuf));
  if((dwErr = RegOpenKeyEx(hkRootKey, szKey, 0, KEY_READ, &hkResult)) == ERROR_SUCCESS)
  {
    dwSize = sizeof(szBuf);
    dwErr  = RegQueryValueEx(hkResult, szName, 0, NULL, szBuf, &dwSize);

    if((*szBuf != '\0') && (dwErr == ERROR_SUCCESS))
      bNameExists = TRUE;

    RegCloseKey(hkResult);
  }

  return(bNameExists);
}

void DeleteWinRegKey(HKEY hkRootKey, LPSTR szKey, BOOL bAbsoluteDelete)
{
  HKEY      hkResult;
  DWORD     dwErr;
  DWORD     dwTotalSubKeys;
  DWORD     dwTotalValues;
  DWORD     dwSubKeySize;
  FILETIME  ftLastWriteFileTime;
  char      szSubKey[MAX_BUF_TINY];
  char      szNewKey[MAX_BUF];
  long      lRv;

  dwErr = RegOpenKeyEx(hkRootKey, szKey, 0, KEY_QUERY_VALUE, &hkResult);
  if(dwErr == ERROR_SUCCESS)
  {
    dwTotalSubKeys = 0;
    dwTotalValues  = 0;
    RegQueryInfoKey(hkResult, NULL, NULL, NULL, &dwTotalSubKeys, NULL, NULL, &dwTotalValues, NULL, NULL, NULL, NULL);
    RegCloseKey(hkResult);

    if(((dwTotalSubKeys == 0) && (dwTotalValues == 0)) || bAbsoluteDelete)
    {
      if(dwTotalSubKeys && bAbsoluteDelete)
      {
        do
        {
          dwSubKeySize = sizeof(szSubKey);
          lRv = 0;
          if(RegOpenKeyEx(hkRootKey, szKey, 0, KEY_READ, &hkResult) == ERROR_SUCCESS)
          {
            if((lRv = RegEnumKeyEx(hkResult, 0, szSubKey, &dwSubKeySize, NULL, NULL, NULL, &ftLastWriteFileTime)) == ERROR_SUCCESS)
            {
              RegCloseKey(hkResult);
              lstrcpy(szNewKey, szKey);
              AppendBackSlash(szNewKey, sizeof(szNewKey));
              lstrcat(szNewKey, szSubKey);
              DeleteWinRegKey(hkRootKey, szNewKey, bAbsoluteDelete);
            }
            else
              RegCloseKey(hkResult);
          }
        } while(lRv != ERROR_NO_MORE_ITEMS);
      }

      dwErr = RegDeleteKey(hkRootKey, szKey);
    }
  }
}

void DeleteWinRegValue(HKEY hkRootKey, LPSTR szKey, LPSTR szName)
{
  HKEY    hkResult;
  DWORD   dwErr;

  dwErr = RegOpenKeyEx(hkRootKey, szKey, 0, KEY_WRITE, &hkResult);
  if(dwErr == ERROR_SUCCESS)
  {
    if(*szName == '\0')
      dwErr = RegDeleteValue(hkResult, NULL);
    else
      dwErr = RegDeleteValue(hkResult, szName);

    RegCloseKey(hkResult);
  }
}

DWORD GetWinReg(HKEY hkRootKey, LPSTR szKey, LPSTR szName, LPSTR szReturnValue, DWORD dwReturnValueSize)
{
  HKEY  hkResult;
  DWORD dwErr;
  DWORD dwSize;
  DWORD dwType;
  char  szBuf[MAX_BUF];

  ZeroMemory(szBuf, sizeof(szBuf));
  ZeroMemory(szReturnValue, dwReturnValueSize);

  if((dwErr = RegOpenKeyEx(hkRootKey, szKey, 0, KEY_READ, &hkResult)) == ERROR_SUCCESS)
  {
    dwSize = sizeof(szBuf);
    dwErr  = RegQueryValueEx(hkResult, szName, 0, &dwType, szBuf, &dwSize);

    if((dwType == REG_MULTI_SZ) && (*szBuf != '\0'))
    {
      DWORD dwCpSize;

      dwCpSize = dwReturnValueSize < dwSize ? (dwReturnValueSize - 1) : dwSize;
      memcpy(szReturnValue, szBuf, dwCpSize);
    }
    else if((*szBuf != '\0') && (dwErr == ERROR_SUCCESS))
      ExpandEnvironmentStrings(szBuf, szReturnValue, dwReturnValueSize);
    else
      *szReturnValue = '\0';

    RegCloseKey(hkResult);
  }

  return(dwType);
}

LONG _CreateWinRegKey(HKEY hkRootKey,
                       LPSTR szKey,
                       BOOL bLogForUninstall,
                       BOOL bDnu,
                       BOOL bForceCreate)
{
  HKEY    hkResult;
  LONG    err = ERROR_SUCCESS;
  DWORD   dwDisp;
  char    szBuf[MAX_BUF];
  char    szRootKey[MAX_BUF_TINY];

  if(!WinRegKeyExists(hkRootKey, szKey) || bForceCreate)
  {
    err = RegCreateKeyEx(hkRootKey, szKey, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hkResult, &dwDisp);
    
    if(((err == ERROR_SUCCESS) &&
       bLogForUninstall &&
       ParseRootKeyString(hkRootKey, szRootKey, sizeof(szRootKey))) || bForceCreate)
    {
      wsprintf(szBuf, "%s\\%s []", szRootKey, szKey);
      UpdateInstallLog(KEY_CREATE_REG_KEY, szBuf, bDnu);
    }
    RegCloseKey(hkResult);
  }

  return(err);
}

LONG CreateWinRegKey(HKEY hkRootKey,
                      LPSTR szKey,
                      BOOL bLogForUninstall,
                      BOOL bDnu)
{
  char    szTempKeyPath[MAX_BUF];
  char    saveChar;
  LPSTR   pointerToBackslashChar = NULL;
  LPSTR   pointerToStrWalker = NULL;

  if(MozCopyStr(szKey, szTempKeyPath, sizeof(szTempKeyPath)))
    return(ERROR_BUFFER_OVERFLOW);

  
  
  
  
  RemoveBackSlash(szTempKeyPath);
  pointerToStrWalker = szTempKeyPath;
  while((pointerToBackslashChar = strstr(pointerToStrWalker, "\\")) != NULL)
  {
    saveChar = *pointerToBackslashChar;
    *pointerToBackslashChar = '\0';
    
    _CreateWinRegKey(hkRootKey, szTempKeyPath, bLogForUninstall, bDnu, DO_NOT_FORCE_ADD_TO_UNINSTALL_LOG);
    *pointerToBackslashChar = saveChar;
    pointerToStrWalker = &pointerToBackslashChar[1];
  }

  
  
  
  
  
  return(_CreateWinRegKey(hkRootKey, szKey, bLogForUninstall, bDnu, FORCE_ADD_TO_UNINSTALL_LOG));
}

void SetWinReg(HKEY hkRootKey,
               LPSTR szKey,
               BOOL bOverwriteKey,
               LPSTR szName,
               BOOL bOverwriteName,
               DWORD dwType,
               LPBYTE lpbData,
               DWORD dwSize,
               BOOL bLogForUninstall,
               BOOL bDnu)
{
  HKEY    hkResult;
  DWORD   dwErr;
  BOOL    bNameExists;
  char    szBuf[MAX_BUF];
  char    szRootKey[MAX_BUF_TINY];

  
  CreateWinRegKey(hkRootKey, szKey, bLogForUninstall, bDnu);

  dwErr = RegOpenKeyEx(hkRootKey, szKey, 0, KEY_WRITE, &hkResult);
  if(dwErr == ERROR_SUCCESS)
  {
    bNameExists = WinRegNameExists(hkRootKey, szKey, szName);
    if((bNameExists == FALSE) ||
      ((bNameExists == TRUE) && (bOverwriteName == TRUE)))
    {
      dwErr = RegSetValueEx(hkResult, szName, 0, dwType, lpbData, dwSize);
      
      if(bLogForUninstall &&
         ParseRootKeyString(hkRootKey, szRootKey, sizeof(szRootKey)))
      {
        if(ParseRegType(szBuf, &dwType))
        {
          wsprintf(szBuf, "%s\\%s [%s]", szRootKey, szKey, szName);
          UpdateInstallLog(KEY_STORE_REG_STRING, szBuf, bDnu);
        }
        else
        {
          wsprintf(szBuf, "%s\\%s [%s]", szRootKey, szKey, szName);
          UpdateInstallLog(KEY_STORE_REG_NUMBER, szBuf, bDnu);
        }
      }
    }

    RegCloseKey(hkResult);
  }
}





























void AppendWinReg(HKEY hkRootKey,
               LPSTR szKey,
               LPSTR szName,
               DWORD dwType,
               LPBYTE lpbData,
               BYTE delimiter,
               DWORD dwSize,
               BOOL bLogForUninstall,
               BOOL bDnu)
{
  HKEY    hkResult;
  DWORD   dwErr;
  DWORD   dwDisp;
  BOOL    bKeyExists;
  BOOL    bNameExists;
  char    szBuf[MAX_BUF];
  char    szRootKey[MAX_BUF_TINY]; 

  bKeyExists  = WinRegKeyExists(hkRootKey, szKey);
  bNameExists = WinRegNameExists(hkRootKey, szKey, szName);
  dwErr       = RegOpenKeyEx(hkRootKey, szKey, 0, KEY_WRITE, &hkResult);

  if (dwType != REG_SZ) 
      return;

  if(dwErr != ERROR_SUCCESS)
  {
    dwErr = RegCreateKeyEx(hkRootKey, szKey, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hkResult, &dwDisp);
    
    if(bLogForUninstall &&
       ParseRootKeyString(hkRootKey, szRootKey, sizeof(szRootKey)))
    {
      wsprintf(szBuf, "%s\\%s []", szRootKey, szKey);
      UpdateInstallLog(KEY_CREATE_REG_KEY, szBuf, bDnu);
    }
  }

  if(dwErr == ERROR_SUCCESS)
  {
    if((bNameExists == FALSE))
    {
      

      dwErr = RegSetValueEx(hkResult, szName, 0, dwType, lpbData, dwSize);
      
      if(bLogForUninstall &&
         ParseRootKeyString(hkRootKey, szRootKey, sizeof(szRootKey)))
      {
        if(ParseRegType(szBuf, &dwType))
        {
          wsprintf(szBuf, "%s\\%s [%s]", szRootKey, szKey, szName);
          UpdateInstallLog(KEY_STORE_REG_STRING, szBuf, bDnu);
        }
        else
        {
          wsprintf(szBuf, "%s\\%s [%s]", szRootKey, szKey, szName);
          UpdateInstallLog(KEY_STORE_REG_NUMBER, szBuf, bDnu);
        }
      }
    } else {
      

      
      GetWinReg(hkRootKey, szKey, szName, szBuf, sizeof(szBuf));  
      if ( delimiter != 0 ) {
          char delim[ 2 ];
          delim[0] = delimiter;
          delim[1] = '\0';
          strcat( szBuf, delim );
      }
      strcat( szBuf, lpbData );
      RegCloseKey(hkResult);
      SetWinReg(hkRootKey, szKey, TRUE, szName, TRUE, dwType, szBuf, strlen( szBuf ) + 1, bLogForUninstall, bDnu);
      return;
    }

    RegCloseKey(hkResult);
  }
}

HRESULT ProcessWinReg(DWORD dwTiming, char *szSectionPrefix)
{
  char    szBuf[MAX_BUF];
  char    szKey[MAX_BUF];
  char    szName[MAX_BUF];
  char    szShortName[MAX_BUF];
  char    szValue[MAX_BUF];
  char    szDecrypt[MAX_BUF];
  char    szOverwriteKey[MAX_BUF];
  char    szOverwriteName[MAX_BUF];
  char    szSection[MAX_BUF];
  HKEY    hRootKey;
  BOOL    bDone;
  BOOL    bDnu;
  BOOL    bOverwriteKey;
  BOOL    bOverwriteName;
  BOOL    bOSDetected;
  DWORD   dwIndex;
  DWORD   dwNameIndex = 1;
  DWORD   dwType;
  DWORD   dwSize;
  const DWORD   dwUpperLimit = 100;
  __int64 iiNum;

  dwIndex = 0;
  BuildNumberedString(dwIndex, szSectionPrefix, "Windows Registry", szSection, sizeof(szSection));
  GetPrivateProfileString(szSection, "Root Key", "", szBuf, sizeof(szBuf), szFileIniConfig);
  while(*szBuf != '\0')
  {
    if(TimingCheck(dwTiming, szSection, szFileIniConfig) && MeetCondition(szSection))
    {
      hRootKey = ParseRootKey(szBuf);

      GetPrivateProfileString(szSection, "Key",                 "", szBuf,           sizeof(szBuf),          szFileIniConfig);
      GetPrivateProfileString(szSection, "Decrypt Key",         "", szDecrypt,       sizeof(szDecrypt),      szFileIniConfig);
      GetPrivateProfileString(szSection, "Overwrite Key",       "", szOverwriteKey,  sizeof(szOverwriteKey), szFileIniConfig);
      ZeroMemory(szKey, sizeof(szKey));
      if(lstrcmpi(szDecrypt, "TRUE") == 0)
        DecryptString(szKey, szBuf);
      else
        lstrcpy(szKey, szBuf);

      if(lstrcmpi(szOverwriteKey, "FALSE") == 0)
        bOverwriteKey = FALSE;
      else
        bOverwriteKey = TRUE;

      GetPrivateProfileString(szSection, "Name",                "", szBuf,           sizeof(szBuf),           szFileIniConfig);
      GetPrivateProfileString(szSection, "Decrypt Name",        "", szDecrypt,       sizeof(szDecrypt),       szFileIniConfig);
      GetPrivateProfileString(szSection, "Overwrite Name",      "", szOverwriteName, sizeof(szOverwriteName), szFileIniConfig);
      ZeroMemory(szName, sizeof(szName));
      if(lstrcmpi(szDecrypt, "TRUE") == 0)
        DecryptString(szName, szBuf);
      else
        lstrcpy(szName, szBuf);

      if(lstrcmpi(szOverwriteName, "ENUMERATE") == 0)
      {
        bOverwriteName = FALSE;
        lstrcpy(szShortName, szName);
        wsprintf(szName, "%s%02d", szShortName, dwNameIndex++);

        bDone = FALSE;
        while(!bDone && (dwNameIndex < dwUpperLimit))
        {
          if(WinRegNameExists(hRootKey, szKey, szName))
          {
            GetWinReg(hRootKey, szKey, szName, szBuf, sizeof(szBuf));
            if(lstrcmpi(szBuf, sgProduct.szAppPath) == 0)
              bDone = TRUE;
            else
              wsprintf(szName, "%s%02d", szShortName, dwNameIndex++);
          }
          else
            bDone = TRUE;
        }
        if(dwNameIndex >= dwUpperLimit)
          return FO_ERROR_INCR_EXCEEDS_LIMIT;
      }
      else if(lstrcmpi(szOverwriteName, "FALSE") == 0)
        bOverwriteName = FALSE;
      else
        bOverwriteName = TRUE;

      GetPrivateProfileString(szSection, "Name Value",          "", szBuf,           sizeof(szBuf), szFileIniConfig);
      GetPrivateProfileString(szSection, "Decrypt Name Value",  "", szDecrypt,       sizeof(szDecrypt), szFileIniConfig);
      ZeroMemory(szValue, sizeof(szValue));
      if(lstrcmpi(szDecrypt, "TRUE") == 0)
        DecryptString(szValue, szBuf);
      else
        lstrcpy(szValue, szBuf);

      GetPrivateProfileString(szSection, "Size",                "", szBuf,           sizeof(szBuf), szFileIniConfig);
      if(*szBuf != '\0')
        dwSize = atoi(szBuf);
      else
        dwSize = 0;

      GetPrivateProfileString(szSection,
                              "Do Not Uninstall",
                              "",
                              szBuf,
                              sizeof(szBuf),
                              szFileIniConfig);

      if(lstrcmpi(szBuf, "TRUE") == 0)
        bDnu = TRUE;
      else
        bDnu = FALSE;

      

      GetPrivateProfileString(szSection,
                              "OS",
                              "",
                              szBuf,
                              sizeof(szBuf),
                              szFileIniConfig);
      


      bOSDetected = TRUE;
      if( (*szBuf != '\0') && ((gSystemInfo.dwOSType & ParseOSType(szBuf)) == 0) )
        bOSDetected = FALSE;

      if(bOSDetected)
      {
        ZeroMemory(szBuf, sizeof(szBuf));
        GetPrivateProfileString(szSection,
                                "Type",
                                "",
                                szBuf,
                                sizeof(szBuf),
                                szFileIniConfig);

        if(ParseRegType(szBuf, &dwType))
        {
          
          SetWinReg(hRootKey, szKey, bOverwriteKey, szName, bOverwriteName,
                  dwType, (CONST LPBYTE)szValue, lstrlen(szValue), TRUE, bDnu);
        }
        else
        {
          iiNum = _atoi64(szValue);
          
          SetWinReg(hRootKey, szKey, bOverwriteKey, szName, bOverwriteName,
                  dwType, (CONST LPBYTE)&iiNum, dwSize, TRUE, bDnu);
        }
      }
    }

    ++dwIndex;
    BuildNumberedString(dwIndex, szSectionPrefix, "Windows Registry", szSection, sizeof(szSection));
    GetPrivateProfileString(szSection, "Root Key", "", szBuf, sizeof(szBuf), szFileIniConfig);
  }
  return(FO_SUCCESS);
}

HRESULT ProcessProgramFolder(DWORD dwTiming, char *szSectionPrefix)
{
  DWORD dwIndex0;
  DWORD dwIndex1;
  DWORD dwIconId;
  DWORD dwRestrictedAccess;
  char  szIndex1[MAX_BUF];
  char  szBuf[MAX_BUF];
  char  szSection0[MAX_BUF];
  char  szSection1[MAX_BUF];
  char  szProgramFolder[MAX_BUF];
  char  szFile[MAX_BUF];
  char  szArguments[MAX_BUF];
  char  szWorkingDir[MAX_BUF];
  char  szDescription[MAX_BUF];
  char  szIconPath[MAX_BUF];

  dwIndex0 = 0;
  BuildNumberedString(dwIndex0, szSectionPrefix, "Program Folder", szSection0, sizeof(szSection0));
  GetPrivateProfileString(szSection0, "Program Folder", "", szBuf, sizeof(szBuf), szFileIniConfig);
  while(*szBuf != '\0')
  {
    if(TimingCheck(dwTiming, szSection0, szFileIniConfig))
    {
      DecryptString(szProgramFolder, szBuf);

      dwIndex1 = 0;
      itoa(dwIndex1, szIndex1, 10);
      lstrcpy(szSection1, szSection0);
      lstrcat(szSection1, "-Shortcut");
      lstrcat(szSection1, szIndex1);
      GetPrivateProfileString(szSection1, "File", "", szBuf, sizeof(szBuf), szFileIniConfig);
      while(*szBuf != '\0')
      {
        DecryptString(szFile, szBuf);
        GetPrivateProfileString(szSection1, "Arguments",    "", szBuf, sizeof(szBuf), szFileIniConfig);
        DecryptString(szArguments, szBuf);
        GetPrivateProfileString(szSection1, "Working Dir",  "", szBuf, sizeof(szBuf), szFileIniConfig);
        DecryptString(szWorkingDir, szBuf);
        GetPrivateProfileString(szSection1, "Description",  "", szBuf, sizeof(szBuf), szFileIniConfig);
        DecryptString(szDescription, szBuf);
        GetPrivateProfileString(szSection1, "Icon Path",    "", szBuf, sizeof(szBuf), szFileIniConfig);
        DecryptString(szIconPath, szBuf);
        GetPrivateProfileString(szSection1, "Icon Id",      "", szBuf, sizeof(szBuf), szFileIniConfig);
        if(*szBuf != '\0')
          dwIconId = atol(szBuf);
        else
          dwIconId = 0;

        GetPrivateProfileString(szSection1, "Restricted Access",    "", szBuf, sizeof(szBuf), szFileIniConfig);
        dwRestrictedAccess = ParseRestrictedAccessKey(szBuf);
        if((dwRestrictedAccess == RA_IGNORE) ||
          ((dwRestrictedAccess == RA_ONLY_RESTRICTED) && gbRestrictedAccess) ||
          ((dwRestrictedAccess == RA_ONLY_NONRESTRICTED) && !gbRestrictedAccess))
        {
          CreateALink(szFile, szProgramFolder, szDescription, szWorkingDir, szArguments, szIconPath, dwIconId);
          lstrcpy(szBuf, szProgramFolder);
          AppendBackSlash(szBuf, sizeof(szBuf));
          lstrcat(szBuf, szDescription);
          UpdateInstallLog(KEY_WINDOWS_SHORTCUT, szBuf, FALSE);
        }

        ++dwIndex1;
        itoa(dwIndex1, szIndex1, 10);
        lstrcpy(szSection1, szSection0);
        lstrcat(szSection1, "-Shortcut");
        lstrcat(szSection1, szIndex1);
        GetPrivateProfileString(szSection1, "File", "", szBuf, sizeof(szBuf), szFileIniConfig);
      }
    }

    ++dwIndex0;
    BuildNumberedString(dwIndex0, szSectionPrefix, "Program Folder", szSection0, sizeof(szSection0));
    GetPrivateProfileString(szSection0, "Program Folder", "", szBuf, sizeof(szBuf), szFileIniConfig);
  }
  return(FO_SUCCESS);
}

HRESULT ProcessProgramFolderShowCmd()
{
  DWORD dwIndex0;
  int   iShowFolder;
  char  szBuf[MAX_BUF];
  char  szSection0[MAX_BUF];
  char  szProgramFolder[MAX_BUF];

  dwIndex0 = 0;
  BuildNumberedString(dwIndex0, NULL, "Program Folder", szSection0, sizeof(szSection0));
  GetPrivateProfileString(szSection0, "Program Folder", "", szBuf, sizeof(szBuf), szFileIniConfig);
  while(*szBuf != '\0')
  {
    DecryptString(szProgramFolder, szBuf);
    GetPrivateProfileString(szSection0, "Show Folder", "", szBuf, sizeof(szBuf), szFileIniConfig);

    if(strcmpi(szBuf, "HIDE") == 0)
      iShowFolder = SW_HIDE;
    else if(strcmpi(szBuf, "MAXIMIZE") == 0)
      iShowFolder = SW_MAXIMIZE;
    else if(strcmpi(szBuf, "MINIMIZE") == 0)
      iShowFolder = SW_MINIMIZE;
    else if(strcmpi(szBuf, "RESTORE") == 0)
      iShowFolder = SW_RESTORE;
    else if(strcmpi(szBuf, "SHOW") == 0)
      iShowFolder = SW_SHOW;
    else if(strcmpi(szBuf, "SHOWMAXIMIZED") == 0)
      iShowFolder = SW_SHOWMAXIMIZED;
    else if(strcmpi(szBuf, "SHOWMINIMIZED") == 0)
      iShowFolder = SW_SHOWMINIMIZED;
    else if(strcmpi(szBuf, "SHOWMINNOACTIVE") == 0)
      iShowFolder = SW_SHOWMINNOACTIVE;
    else if(strcmpi(szBuf, "SHOWNA") == 0)
      iShowFolder = SW_SHOWNA;
    else if(strcmpi(szBuf, "SHOWNOACTIVATE") == 0)
      iShowFolder = SW_SHOWNOACTIVATE;
    else if(strcmpi(szBuf, "SHOWNORMAL") == 0)
      iShowFolder = SW_SHOWNORMAL;

    if(iShowFolder != SW_HIDE)
      if(sgProduct.mode != SILENT)
        WinSpawn(szProgramFolder, NULL, NULL, iShowFolder, WS_WAIT);

    ++dwIndex0;
    BuildNumberedString(dwIndex0, NULL, "Program Folder", szSection0, sizeof(szSection0));
    GetPrivateProfileString(szSection0, "Program Folder", "", szBuf, sizeof(szBuf), szFileIniConfig);
  }
  return(FO_SUCCESS);
}

HRESULT ProcessCreateCustomFiles(DWORD dwTiming)
{
  DWORD dwCompIndex;
  DWORD dwFileIndex;
  DWORD dwSectIndex;
  DWORD dwKVIndex;
  siC   *siCObject = NULL;
  char  szBufTiny[MAX_BUF_TINY];
  char  szSection[MAX_BUF_TINY];
  char  szBuf[MAX_BUF];
  char  szFileName[MAX_BUF];
  char  szDefinedSection[MAX_BUF]; 
  char  szDefinedKey[MAX_BUF]; 
  char  szDefinedValue[MAX_BUF];

  dwCompIndex   = 0;
  siCObject = SiCNodeGetObject(dwCompIndex, TRUE, AC_ALL);

  while(siCObject)
  {
    dwFileIndex   = 0;
    wsprintf(szSection,"%s-Configuration File%d",siCObject->szReferenceName,dwFileIndex);
    siCObject = SiCNodeGetObject(++dwCompIndex, TRUE, AC_ALL);
    if(TimingCheck(dwTiming, szSection, szFileIniConfig) == FALSE)
    {
      continue;
    }

    GetPrivateProfileString(szSection, "FileName", "", szBuf, sizeof(szBuf), szFileIniConfig);
    while (*szBuf != '\0')
    {
      DecryptString(szFileName, szBuf);
      if(FileExists(szFileName))
      {
        DeleteFile(szFileName);
      }

      
      dwSectIndex = 0;
      wsprintf(szBufTiny, "Section%d",dwSectIndex);
      GetPrivateProfileString(szSection, szBufTiny, "", szDefinedSection, sizeof(szDefinedSection), szFileIniConfig);
      while(*szDefinedSection != '\0')
      {  
        dwKVIndex =0;
        wsprintf(szBufTiny,"Section%d-Key%d",dwSectIndex,dwKVIndex);
        GetPrivateProfileString(szSection, szBufTiny, "", szDefinedKey, sizeof(szDefinedKey), szFileIniConfig);
        while(*szDefinedKey != '\0')
        {
          wsprintf(szBufTiny,"Section%d-Value%d",dwSectIndex,dwKVIndex);
          GetPrivateProfileString(szSection, szBufTiny, "", szBuf, sizeof(szBuf), szFileIniConfig);
          DecryptString(szDefinedValue, szBuf);
          if(WritePrivateProfileString(szDefinedSection, szDefinedKey, szDefinedValue, szFileName) == 0)
          {
            char szEWPPS[MAX_BUF];
            char szBuf[MAX_BUF];
            char szBuf2[MAX_BUF];
            if(GetPrivateProfileString("Messages", "ERROR_WRITEPRIVATEPROFILESTRING", "", szEWPPS, sizeof(szEWPPS), szFileIniInstall))
            {
              wsprintf(szBuf, "%s\n    [%s]\n    %s=%s", szFileName, szDefinedSection, szDefinedKey, szDefinedValue);
              wsprintf(szBuf2, szEWPPS, szBuf);
              PrintError(szBuf2, ERROR_CODE_SHOW);
            }
            return(FO_ERROR_WRITE);
          }
          wsprintf(szBufTiny,"Section%d-Key%d",dwSectIndex,++dwKVIndex);
          GetPrivateProfileString(szSection, szBufTiny, "", szDefinedKey, sizeof(szDefinedKey), szFileIniConfig);
        } 

        wsprintf(szBufTiny, "Section%d",++dwSectIndex);
        GetPrivateProfileString(szSection, szBufTiny, "", szDefinedSection, sizeof(szDefinedSection), szFileIniConfig);
      } 

      wsprintf(szSection,"%s-Configuration File%d",siCObject->szReferenceName,++dwFileIndex);
      GetPrivateProfileString(szSection, "FileName", "", szBuf, sizeof(szBuf), szFileIniConfig);
    } 
  } 
  return (FO_SUCCESS);
}

