




































#include "setup.h"
#include "extern.h"
#include "extra.h"
#include "ifuncns.h"
#include "supersede.h"

void              SaveGreInfo(grever *aGreInstalledListTmp, greInfo* aGre);
void              ResolveSupersedeGre(siC *siCObject, greInfo *aGre);
HRESULT           GetGreSupersedeVersionList(siC *siCObject, grever **aGreSupersedeList);
HRESULT           GetGreInstalledVersionList(siC *siCObject, grever **aGreInstalledList);

grever *CreateGVerNode()
{
  grever *gverNode;

  if((gverNode = NS_GlobalAlloc(sizeof(struct structGreVer))) == NULL)
    return(NULL);

  gverNode->greHomePath[0]         = '\0';
  gverNode->greInstaller[0]        = '\0';
  gverNode->greUserAgent[0]        = '\0';
  gverNode->version.ullMajor       = 0;
  gverNode->version.ullMinor       = 0;
  gverNode->version.ullRelease     = 0;
  gverNode->version.ullBuild       = 0;
  gverNode->next                   = NULL;
  return(gverNode);
}

void DeleteGverList(grever *gverHead)
{
  grever *tmp;

  while(gverHead != NULL)
  {
    tmp = gverHead;
    gverHead = gverHead->next;
    FreeMemory(&tmp);
  }
}


HRESULT GetGreSupersedeVersionList(siC *siCObject, grever **aGreSupersedeList)
{
  grever  *gverTmp = NULL;
  grever  *gverTail = NULL;
  char    key[MAX_BUF_TINY];
  char    versionString[MAX_BUF_TINY];
  DWORD   index;

  if(*aGreSupersedeList)
    
    return(WIZ_OK);

  index = 0;
  wsprintf(key, "SupersedeVersion%d", index);        
  GetPrivateProfileString(siCObject->szReferenceName, key, "", versionString,
                          sizeof(versionString), szFileIniConfig);
  while(*versionString != '\0')
  {
    gverTmp = CreateGVerNode();
    if(!gverTmp)
      
      exit(WIZ_OUT_OF_MEMORY);

    TranslateVersionStr(versionString, &gverTmp->version);
    if(*aGreSupersedeList == NULL)
    {
      
      *aGreSupersedeList = gverTmp;
      gverTail = *aGreSupersedeList;
    }
    else if(gverTail)
    {
      
      gverTail->next = gverTmp;
      gverTail = gverTail->next;
    }
    
    wsprintf(key, "SupersedeVersion%d", ++index);        
    GetPrivateProfileString(siCObject->szReferenceName, key, "", versionString,
                            sizeof(versionString), szFileIniConfig);
  }
  return(WIZ_OK);
}


HRESULT GetGreInstalledVersionList(siC *siCObject, grever **aGreInstalledList)
{
  DWORD     index;
  DWORD     enumIndex;
  DWORD     greVersionKeyLen;
  grever    *gverTmp = NULL;
  grever    *gverTail = NULL;
  char      szSupersedeWinRegPath[MAX_BUF];
  char      key[MAX_BUF_TINY];
  char      greVersionKey[MAX_BUF_TINY];
  char      subKey[MAX_BUF];
  char      subKeyInstaller[MAX_BUF];
  char      szBuf[MAX_BUF];
  char      greXpcomPath[MAX_BUF];
  char      greXpcomFile[MAX_BUF];
  int       greXpcomPathLen;
  char      xpcomFilename[] = "xpcom.dll";
  char      greKeyPath[MAX_BUF];
  verBlock  vbInstalledVersion;
  HKEY      hkeyRoot = NULL;
  HKEY      hkGreKeyParentPath = NULL;
  HKEY      hkGreKeyPath = NULL;
  LONG      rv;

  if(*aGreInstalledList)
    
    return(WIZ_OK);

  index = 0;
  wsprintf(key, "SupersedeWinReg%d", index);        
  GetPrivateProfileString(siCObject->szReferenceName, key, "", szSupersedeWinRegPath,
                          sizeof(szSupersedeWinRegPath), szFileIniConfig);
  while(*szSupersedeWinRegPath != '\0')
  {
    BOOL skip = FALSE;
    if(!GetKeyInfo(szSupersedeWinRegPath, szBuf, sizeof(szBuf), KEY_INFO_ROOT))
      
      skip = TRUE;

    hkeyRoot = ParseRootKey(szBuf);
    if(!GetKeyInfo(szSupersedeWinRegPath, subKey, sizeof(subKey), KEY_INFO_SUBKEY))
      
      skip = TRUE;

    if(RegOpenKeyEx(hkeyRoot, subKey, 0, KEY_READ, &hkGreKeyParentPath) != ERROR_SUCCESS)
      
      skip = TRUE;

    greVersionKeyLen = sizeof(greVersionKey);
    enumIndex = 0;
    while(!skip &&
         (RegEnumKeyEx(hkGreKeyParentPath, enumIndex++, greVersionKey, &greVersionKeyLen,
                       NULL, NULL, NULL, NULL) != ERROR_NO_MORE_ITEMS))
    {
      sprintf(greKeyPath, "%s\\%s", subKey, greVersionKey);
      if(RegOpenKeyEx(hkeyRoot, greKeyPath, 0, KEY_READ, &hkGreKeyPath) != ERROR_SUCCESS)
        
        
        
        continue;

      greXpcomPathLen = sizeof(greXpcomPath);
      rv = RegQueryValueEx(hkGreKeyPath, "GreHome", 0, NULL, (BYTE *)greXpcomPath, &greXpcomPathLen);
      RegCloseKey(hkGreKeyPath);

      if(rv != ERROR_SUCCESS)
        
        
        
        continue;
 
      if(MozCopyStr(greXpcomPath, greXpcomFile, sizeof(greXpcomFile)))
      {
        RegCloseKey(hkGreKeyParentPath);
        PrintError(szEOutOfMemory, ERROR_CODE_HIDE);
        exit(WIZ_OUT_OF_MEMORY);
      }

      if(sizeof(greXpcomFile) <= lstrlen(greXpcomFile) + sizeof(xpcomFilename) + 1)
      {
        RegCloseKey(hkGreKeyParentPath);
        PrintError(szEOutOfMemory, ERROR_CODE_HIDE);
        exit(WIZ_OUT_OF_MEMORY);
      }

      AppendBackSlash(greXpcomFile, sizeof(greXpcomFile));
      lstrcat(greXpcomFile, xpcomFilename);

      if(GetFileVersion(greXpcomFile, &vbInstalledVersion))
      {
        
        gverTmp = CreateGVerNode();
        if(!gverTmp)
        {
          RegCloseKey(hkGreKeyParentPath);
          
          exit(WIZ_OUT_OF_MEMORY);
        }

        if(MozCopyStr(greXpcomPath, gverTmp->greHomePath, sizeof(gverTmp->greHomePath)))
        {
          RegCloseKey(hkGreKeyParentPath);
          PrintError(szEOutOfMemory, ERROR_CODE_HIDE);
          exit(WIZ_OUT_OF_MEMORY);
        }

        gverTmp->version.ullMajor   = vbInstalledVersion.ullMajor;
        gverTmp->version.ullMinor   = vbInstalledVersion.ullMinor;
        gverTmp->version.ullRelease = vbInstalledVersion.ullRelease;
        gverTmp->version.ullBuild   = vbInstalledVersion.ullBuild;
        if(*aGreInstalledList == NULL)
        {
          
          *aGreInstalledList = gverTmp;
          gverTail = *aGreInstalledList;
        }
        else if(gverTail)
        {
          
          gverTail->next = gverTmp;
          gverTail = gverTail->next;
        }

        
        sprintf(subKeyInstaller, "%s\\%s\\Installer", subKey, greVersionKey);
        GetWinReg(hkeyRoot, subKeyInstaller, "PathToExe", gverTmp->greInstaller, sizeof(gverTmp->greInstaller));
        MozCopyStr(greVersionKey, gverTmp->greUserAgent, sizeof(gverTmp->greUserAgent));
      }

      greVersionKeyLen = sizeof(greVersionKey);
    }

    if(hkGreKeyParentPath)
    {
      RegCloseKey(hkGreKeyParentPath);
      hkGreKeyParentPath = NULL;
    }

    ++index;
    wsprintf(key, "SupersedeWinReg%d", index);        
    GetPrivateProfileString(siCObject->szReferenceName, key, "", szSupersedeWinRegPath, sizeof(szSupersedeWinRegPath), szFileIniConfig);
  }
  return(WIZ_OK);
}

void SaveGreInfo(grever *aGreInstalledListTmp, greInfo* aGre)
{
  MozCopyStr(aGreInstalledListTmp->greInstaller,     aGre->installerAppPath, sizeof(aGre->installerAppPath));
  MozCopyStr(aGreInstalledListTmp->greUserAgent,     aGre->userAgent,        sizeof(aGre->userAgent));
  MozCopyStr(aGreInstalledListTmp->greHomePath,      aGre->homePath,         sizeof(aGre->homePath));
}

void ResolveSupersedeGre(siC *siCObject, greInfo *aGre)
{
  grever  *greSupersedeListTmp = NULL;
  grever  *greInstalledListTmp = NULL;
  char    versionStr[MAX_BUF_TINY];
  siC     *siCTmp = NULL;
  BOOL    foundVersionWithinRange = FALSE;
  BOOL    minVerRead = FALSE;
  BOOL    maxVerRead = FALSE;
  BOOL    stillInRange = FALSE;

  if(GetGreSupersedeVersionList(siCObject, &aGre->greSupersedeList))
    return;
  if(GetGreInstalledVersionList(siCObject, &aGre->greInstalledList))
    return;
  if(!aGre->greSupersedeList || !aGre->greInstalledList)
    
    return;

  GetPrivateProfileString(siCObject->szReferenceName, "SupersedeMinVersion", "",
                          versionStr, sizeof(versionStr), szFileIniConfig);
  if(*versionStr != '\0')
  {
    TranslateVersionStr(versionStr, &aGre->minVersion);
    minVerRead = TRUE;
  }

  GetPrivateProfileString(siCObject->szReferenceName, "SupersedeMaxVersion", "",
                          versionStr, sizeof(versionStr), szFileIniConfig);
  if(*versionStr != '\0')
  {
    TranslateVersionStr(versionStr, &aGre->maxVersion);
    maxVerRead = TRUE;
  }


  
  greInstalledListTmp = aGre->greInstalledList;
  while(greInstalledListTmp)
  {
    greSupersedeListTmp = aGre->greSupersedeList;
    while(greSupersedeListTmp)
    {
      if(CompareVersion(greInstalledListTmp->version, greSupersedeListTmp->version) == 0)
      {
        SaveGreInfo(greInstalledListTmp, aGre);
        siCObject->bSupersede = TRUE;
        aGre->siCGreComponent = siCObject;
        break;
      }
      else if(!foundVersionWithinRange && (minVerRead || maxVerRead))
      {
        stillInRange = TRUE;

        if(minVerRead)
          stillInRange = CompareVersion(greInstalledListTmp->version, aGre->minVersion) >= 0;

        if(stillInRange && maxVerRead)
          stillInRange = CompareVersion(greInstalledListTmp->version, aGre->maxVersion) <= 0;

        if(stillInRange)
        {
          
          SaveGreInfo(greInstalledListTmp, aGre);
          foundVersionWithinRange = TRUE;
        }
      }
      greSupersedeListTmp = greSupersedeListTmp->next;
    }

    if(siCObject->bSupersede)
      break;

    greInstalledListTmp = greInstalledListTmp->next;
  }

  if(!siCObject->bSupersede && foundVersionWithinRange)
    siCObject->bSupersede = TRUE;

  if(siCObject->bSupersede)
  {
    siCObject->dwAttributes &= ~SIC_SELECTED;
    siCObject->dwAttributes |= SIC_DISABLED;
    siCObject->dwAttributes |= SIC_INVISIBLE;
  }
  else
    






    siCObject->dwAttributes &= ~SIC_DISABLED;
}

BOOL ResolveSupersede(siC *siCObject, greInfo *aGre)
{
  DWORD dwIndex;
  char  szFilePath[MAX_BUF];
  char  szSupersedeFile[MAX_BUF];
  char  szSupersedeVersion[MAX_BUF];
  char  szType[MAX_BUF_TINY];
  char  szKey[MAX_BUF_TINY];
  verBlock  vbVersionNew;
  verBlock  vbFileVersion;

  siCObject->bSupersede = FALSE;
  if(siCObject->dwAttributes & SIC_SUPERSEDE)
  {
    dwIndex = 0;
    GetPrivateProfileString(siCObject->szReferenceName, "SupersedeType", "", szType, sizeof(szType), szFileIniConfig);
    if(*szType !='\0')
    {
      if(lstrcmpi(szType, "File Exists") == 0)
      {
        wsprintf(szKey, "SupersedeFile%d", dwIndex);        
        GetPrivateProfileString(siCObject->szReferenceName, szKey, "", szSupersedeFile, sizeof(szSupersedeFile), szFileIniConfig);
        while(*szSupersedeFile != '\0')
        {
          DecryptString(szFilePath, szSupersedeFile);
          if(FileExists(szFilePath))
          {
            wsprintf(szKey, "SupersedeMinVersion%d",dwIndex);
            GetPrivateProfileString(siCObject->szReferenceName, szKey, "", szSupersedeVersion, sizeof(szSupersedeVersion), szFileIniConfig);
            if(*szSupersedeVersion != '\0')
            {
              if(GetFileVersion(szFilePath,&vbFileVersion))
              {
                

                TranslateVersionStr(szSupersedeVersion, &vbVersionNew);
                if(CompareVersion(vbFileVersion,vbVersionNew) >= 0)
                {  
                  siCObject->bSupersede = TRUE;
                  break;  
                }
              }
            }
            else
            { 
              siCObject->bSupersede = TRUE;
              break;  
            }
          }
          wsprintf(szKey, "SupersedeFile%d", ++dwIndex);        
          GetPrivateProfileString(siCObject->szReferenceName, szKey, "", szSupersedeFile, sizeof(szSupersedeFile), szFileIniConfig);
        }
      }
      else if(lstrcmpi(szType, "GRE") == 0)
      {
        
        aGre->siCGreComponent = siCObject;

        







        if((gbForceInstallGre) && (lstrcmpi(sgProduct.szProductNameInternal, "GRE") != 0))
        {
          siCObject->dwAttributes |= SIC_SELECTED;
          siCObject->dwAttributes |= SIC_DISABLED;
        }
        else
          ResolveSupersedeGre(siCObject, aGre);
      }
    }

    if(siCObject->bSupersede)
    {
      siCObject->dwAttributes &= ~SIC_SELECTED;
      siCObject->dwAttributes |= SIC_DISABLED;
      siCObject->dwAttributes |= SIC_INVISIBLE;
    }
    else
      






      siCObject->dwAttributes &= ~SIC_DISABLED;
  }
  return(siCObject->bSupersede);
}

