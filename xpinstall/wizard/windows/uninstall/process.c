




































#include "process.h"
#include "extern.h"
#include "extra.h"
#include "dialogs.h"
#include <tlhelp32.h>
#include <winperf.h>

#define INDEX_STR_LEN               10
#define PN_PROCESS                  TEXT("Process")
#define PN_PROCESS_ID               TEXT("ID Process")
#define PN_THREAD                   TEXT("Thread")


#define CW_CLOSE_ALL               0x00000001
#define CW_CLOSE_VISIBLE_ONLY      0x00000002
#define CW_CHECK_VISIBLE_ONLY      0x00000003

typedef PERF_DATA_BLOCK             PERF_DATA,      *PPERF_DATA;
typedef PERF_OBJECT_TYPE            PERF_OBJECT,    *PPERF_OBJECT;
typedef PERF_INSTANCE_DEFINITION    PERF_INSTANCE,  *PPERF_INSTANCE;
typedef BOOL   (WINAPI *NS_ProcessWalk)(HANDLE hSnapshot, LPPROCESSENTRY32 lppe);
typedef HANDLE (WINAPI *NS_CreateSnapshot)(DWORD dwFlags, DWORD th32ProcessID);

TCHAR   INDEX_PROCTHRD_OBJ[2*INDEX_STR_LEN];
DWORD   PX_PROCESS;
DWORD   PX_PROCESS_ID;
DWORD   PX_THREAD;

BOOL _FindAndKillProcessNT4(LPSTR aProcessName,
                         BOOL aKillProcess);
BOOL KillProcess(char *aProcessName, HWND aHwndProcess, DWORD aProcessID);
DWORD GetTitleIdx(HWND hWnd, LPTSTR Title[], DWORD LastIndex, LPTSTR Name);
PPERF_OBJECT FindObject (PPERF_DATA pData, DWORD TitleIndex);
PPERF_OBJECT NextObject (PPERF_OBJECT pObject);
PPERF_OBJECT FirstObject (PPERF_DATA pData);
PPERF_INSTANCE   NextInstance (PPERF_INSTANCE pInst);
PPERF_INSTANCE   FirstInstance (PPERF_OBJECT pObject);
DWORD   GetPerfData    (HKEY        hPerfKey,
                        LPTSTR      szObjectIndex,
                        PPERF_DATA  *ppData,
                        DWORD       *pDataSize);
BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM aParam);

typedef struct _CAWOWH
{
  DWORD cwState;
  DWORD processID;
} CAWOWH;

typedef struct _kpf
{
  NS_CreateSnapshot pCreateToolhelp32Snapshot;
  NS_ProcessWalk    pProcessWalkFirst;
  NS_ProcessWalk    pProcessWalkNext;
} kpf;

BOOL _FindAndKillProcess(kpf *kpfRoutines, LPSTR aProcessName, BOOL aKillProcess);











BOOL FindAndKillProcess(LPSTR aProcessName, BOOL aKillProcess)
{
  HANDLE hKernel       = NULL;
  BOOL   bDoWin95Check = TRUE;
  kpf    kpfRoutines;

  if((hKernel = GetModuleHandle("kernel32.dll")) == NULL)
    return(FALSE);

  kpfRoutines.pCreateToolhelp32Snapshot = (NS_CreateSnapshot)GetProcAddress(hKernel, "CreateToolhelp32Snapshot");
  kpfRoutines.pProcessWalkFirst         = (NS_ProcessWalk)GetProcAddress(hKernel,    "Process32First");
  kpfRoutines.pProcessWalkNext          = (NS_ProcessWalk)GetProcAddress(hKernel,    "Process32Next");

  if(kpfRoutines.pCreateToolhelp32Snapshot && kpfRoutines.pProcessWalkFirst && kpfRoutines.pProcessWalkNext)
    return(_FindAndKillProcess(&kpfRoutines, aProcessName, aKillProcess));
  else
    return(_FindAndKillProcessNT4(aProcessName, aKillProcess));
}








BOOL KillProcess(char *aProcessName, HWND aHwndProcess, DWORD aProcessID)
{
  BOOL rv = FALSE;
  HWND hwndProcess;

  hwndProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, aProcessID);
  if(!hwndProcess)
    return(rv);

  if(!TerminateProcess(hwndProcess, 1))
  {
    char errorMsg[MAX_BUF];

    if(GetPrivateProfileString("Messages", "ERROR_TERMINATING_PROCESS", "", errorMsg, sizeof(errorMsg), szFileIniUninstall))
    {
      char buf[MAX_BUF];

      wsprintf(buf, errorMsg, aProcessName);
      PrintError(buf, ERROR_CODE_SHOW);
    }
  }
  else
    rv = TRUE;

  CloseHandle(hwndProcess);
  return(rv);
}




BOOL CALLBACK EnumWindowsProc(HWND aHwnd, LPARAM aParam)
{
  BOOL   rv = TRUE;
  DWORD  processID;
  CAWOWH *closeWindowInfo = (CAWOWH *)aParam;

  GetWindowThreadProcessId(aHwnd, &processID);
  if(processID == closeWindowInfo->processID)
  {
    switch(closeWindowInfo->cwState)
    {
      case CW_CLOSE_ALL:
        SendMessageTimeout(aHwnd, WM_CLOSE, (WPARAM)1, (LPARAM)0, SMTO_NORMAL, WM_CLOSE_TIMEOUT_VALUE, NULL);
        break;

      case CW_CLOSE_VISIBLE_ONLY:
        
        if(GetWindowLong(aHwnd, GWL_STYLE) & WS_VISIBLE)
          SendMessageTimeout(aHwnd, WM_CLOSE, (WPARAM)1, (LPARAM)0, SMTO_NORMAL, WM_CLOSE_TIMEOUT_VALUE, NULL);
        break;

      case CW_CHECK_VISIBLE_ONLY:
        


        if(GetWindowLong(aHwnd, GWL_STYLE) & WS_VISIBLE)
          rv = FALSE;
        break;
    }
  }

  return(rv); 
              
              
              
}
















BOOL CloseAllWindowsOfWindowHandle(HWND aHwndWindow, char *aMsgWait)
{
  CAWOWH closeWindowInfo;
  BOOL  rv = TRUE;

  assert(aHwndWindow);
  assert(aMsgWait);
  if(*aMsgWait != '\0')
    ShowMessage(aMsgWait, TRUE);

  GetWindowThreadProcessId(aHwndWindow, &closeWindowInfo.processID);

  
  closeWindowInfo.cwState = CW_CLOSE_VISIBLE_ONLY;
  EnumWindows(EnumWindowsProc, (LPARAM)&closeWindowInfo);

  
  closeWindowInfo.cwState = CW_CHECK_VISIBLE_ONLY;
  rv = EnumWindows(EnumWindowsProc, (LPARAM)&closeWindowInfo);
  if(rv)
  {
    
    closeWindowInfo.cwState = CW_CLOSE_ALL;
    EnumWindows(EnumWindowsProc, (LPARAM)&closeWindowInfo);
  }
  if(*aMsgWait != '\0')
    ShowMessage(aMsgWait, FALSE);

  return(rv);
}









BOOL _FindAndKillProcess(kpf *kpfRoutines, LPSTR aProcessName, BOOL aKillProcess)
{
  BOOL            rv              = FALSE;
  HANDLE          hCreateSnapshot = NULL;
  PROCESSENTRY32  peProcessEntry;
  
  hCreateSnapshot = kpfRoutines->pCreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
  if(hCreateSnapshot == (HANDLE)-1)
    return(rv);

  peProcessEntry.dwSize = sizeof(PROCESSENTRY32);
  if(kpfRoutines->pProcessWalkFirst(hCreateSnapshot, &peProcessEntry))
  {
    char  szBuf[MAX_BUF];

    do
    {
      ParsePath(peProcessEntry.szExeFile, szBuf, sizeof(szBuf), PP_FILENAME_ONLY);
      
      
      if(lstrcmpi(szBuf, aProcessName) == 0)
      {
        rv = TRUE;
        if(aKillProcess)
          KillProcess(aProcessName, NULL, peProcessEntry.th32ProcessID);
        else
          break;
      }

    } while(kpfRoutines->pProcessWalkNext(hCreateSnapshot, &peProcessEntry));
  }

  CloseHandle(hCreateSnapshot);
  return(rv);
}














BOOL _FindAndKillProcessNT4(LPSTR aProcessName, BOOL aKillProcess)
{
  BOOL          bRv;
  HKEY          hKey;
  DWORD         dwType;
  DWORD         dwSize;
  DWORD         dwTemp;
  DWORD         dwTitleLastIdx;
  DWORD         dwIndex;
  DWORD         dwLen;
  DWORD         pDataSize = 50 * 1024;
  LPSTR         szCounterValueName;
  LPSTR         szTitle;
  LPSTR         *szTitleSz;
  LPSTR         szTitleBuffer;
  PPERF_DATA    pData;
  PPERF_OBJECT  pProcessObject;

  bRv   = FALSE;
  hKey  = NULL;

  if(RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                  TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\perflib"),
                  0,
                  KEY_READ,
                  &hKey) != ERROR_SUCCESS)
    return(bRv);

  dwSize = sizeof(dwTitleLastIdx);
  if(RegQueryValueEx(hKey, TEXT("Last Counter"), 0, &dwType, (LPBYTE)&dwTitleLastIdx, &dwSize) != ERROR_SUCCESS)
  {
    RegCloseKey(hKey);
    return(bRv);
  }
    

  dwSize = sizeof(dwTemp);
  if(RegQueryValueEx(hKey, TEXT("Version"), 0, &dwType, (LPBYTE)&dwTemp, &dwSize) != ERROR_SUCCESS)
  {
    RegCloseKey(hKey);
    szCounterValueName = TEXT("Counters");
    if(RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                    TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Perflib\\009"),
                    0,
                    KEY_READ,
                    &hKey) != ERROR_SUCCESS)
      return(bRv);
  }
  else
  {
    RegCloseKey(hKey);
    szCounterValueName = TEXT("Counter 009");
    hKey = HKEY_PERFORMANCE_DATA;
  }

  
  
  dwSize = 0;
  if(RegQueryValueEx(hKey, szCounterValueName, 0, &dwType, 0, &dwSize) != ERROR_SUCCESS)
    return(bRv);


  
  
  szTitleBuffer = (LPTSTR)LocalAlloc(LMEM_FIXED, dwSize);
  if(!szTitleBuffer)
  {
    RegCloseKey(hKey);
    return(bRv);
  }

  szTitleSz = (LPTSTR *)LocalAlloc(LPTR, (dwTitleLastIdx + 1) * sizeof(LPTSTR));
  if(!szTitleSz)
  {
    if(szTitleBuffer != NULL)
    {
      LocalFree(szTitleBuffer);
      szTitleBuffer = NULL;
    }

    RegCloseKey(hKey);
    return(bRv);
  }

  
  
  if(RegQueryValueEx(hKey, szCounterValueName, 0, &dwType, (BYTE *)szTitleBuffer, &dwSize) != ERROR_SUCCESS)
  {
    RegCloseKey(hKey);
    if(szTitleSz)
      LocalFree(szTitleSz);
    if(szTitleBuffer)
      LocalFree(szTitleBuffer);

    return(bRv);
  }

  
  
  
  szTitle = szTitleBuffer;
  while(dwLen = lstrlen(szTitle))
  {
    dwIndex = atoi(szTitle);
    szTitle = szTitle + dwLen + 1;

    if(dwIndex <= dwTitleLastIdx)
      szTitleSz[dwIndex] = szTitle;

    szTitle = szTitle + lstrlen(szTitle) + 1;
  }

  PX_PROCESS    = GetTitleIdx (NULL, szTitleSz, dwTitleLastIdx, PN_PROCESS);
  PX_PROCESS_ID = GetTitleIdx (NULL, szTitleSz, dwTitleLastIdx, PN_PROCESS_ID);
  PX_THREAD     = GetTitleIdx (NULL, szTitleSz, dwTitleLastIdx, PN_THREAD);
  wsprintf(INDEX_PROCTHRD_OBJ, TEXT("%ld %ld"), PX_PROCESS, PX_THREAD);
  pData = NULL;
  if(GetPerfData(HKEY_PERFORMANCE_DATA, INDEX_PROCTHRD_OBJ, &pData, &pDataSize) == ERROR_SUCCESS)
  {
    PPERF_INSTANCE pInst;
    DWORD          i = 0;

    pProcessObject = FindObject(pData, PX_PROCESS);
    if(pProcessObject)
    {
      LPSTR szPtrStr;
      int   iLen;
      char  szProcessNamePruned[MAX_BUF];
      char  szNewProcessName[MAX_BUF];

      if(sizeof(szProcessNamePruned) < (lstrlen(aProcessName) + 1))
      {
        if(hKey)
          RegCloseKey(hKey);
        if(szTitleSz)
          LocalFree(szTitleSz);
        if(szTitleBuffer)
          LocalFree(szTitleBuffer);

        return(bRv);
      }

      

      lstrcpy(szProcessNamePruned, aProcessName);
      iLen = lstrlen(szProcessNamePruned);
      szPtrStr = &szProcessNamePruned[iLen - 4];
      if((lstrcmpi(szPtrStr, ".exe") == 0) || (lstrcmpi(szPtrStr, ".dll") == 0))
        *szPtrStr = '\0';

      pInst = FirstInstance(pProcessObject);
      for(i = 0; i < (DWORD)(pProcessObject->NumInstances); i++)
      {
        *szNewProcessName = '\0';
        if(WideCharToMultiByte(CP_ACP,
                               0,
                               (LPCWSTR)((PCHAR)pInst + pInst->NameOffset),
                               -1,
                               szNewProcessName,
                               MAX_BUF,
                               NULL,
                               NULL) != 0)
        {
          if(lstrcmpi(szNewProcessName, szProcessNamePruned) == 0)
          {
            if(aKillProcess)
              KillProcess(aProcessName, NULL, PX_PROCESS_ID);
            bRv = TRUE;
            break;
          }
        }

        pInst = NextInstance(pInst);
      }
    }
  }

  if(hKey)
    RegCloseKey(hKey);
  if(szTitleSz)
    LocalFree(szTitleSz);
  if(szTitleBuffer)
    LocalFree(szTitleBuffer);

  return(bRv);
}





























DWORD   GetPerfData    (HKEY        hPerfKey,
                        LPTSTR      szObjectIndex,
                        PPERF_DATA  *ppData,
                        DWORD       *pDataSize)
{
DWORD   DataSize;
DWORD   dwR;
DWORD   Type;


    if (!*ppData)
        *ppData = (PPERF_DATA) LocalAlloc (LMEM_FIXED, *pDataSize);


    do  {
        DataSize = *pDataSize;
        dwR = RegQueryValueEx (hPerfKey,
                               szObjectIndex,
                               NULL,
                               &Type,
                               (BYTE *)*ppData,
                               &DataSize);

        if (dwR == ERROR_MORE_DATA)
            {
            LocalFree (*ppData);
            *pDataSize += 1024;
            *ppData = (PPERF_DATA) LocalAlloc (LMEM_FIXED, *pDataSize);
            }

        if (!*ppData)
            {
            LocalFree (*ppData);
            return ERROR_NOT_ENOUGH_MEMORY;
            }

        } while (dwR == ERROR_MORE_DATA);

    return dwR;
}











PPERF_INSTANCE   FirstInstance (PPERF_OBJECT pObject)
{
    if (pObject)
        return (PPERF_INSTANCE)((PCHAR) pObject + pObject->DefinitionLength);
    else
        return NULL;
}












PPERF_INSTANCE   NextInstance (PPERF_INSTANCE pInst)
{
PERF_COUNTER_BLOCK *pCounterBlock;

    if (pInst)
        {
        pCounterBlock = (PERF_COUNTER_BLOCK *)((PCHAR) pInst + pInst->ByteLength);
        return (PPERF_INSTANCE)((PCHAR) pCounterBlock + pCounterBlock->ByteLength);
        }
    else
        return NULL;
}








PPERF_OBJECT FirstObject (PPERF_DATA pData)
{
    if (pData)
        return ((PPERF_OBJECT) ((PBYTE) pData + pData->HeaderLength));
    else
        return NULL;
}















PPERF_OBJECT NextObject (PPERF_OBJECT pObject)
{
    if (pObject)
        return ((PPERF_OBJECT) ((PBYTE) pObject + pObject->TotalByteLength));
    else
        return NULL;
}








PPERF_OBJECT FindObject (PPERF_DATA pData, DWORD TitleIndex)
{
PPERF_OBJECT pObject;
DWORD        i = 0;

    if (pObject = FirstObject (pData))
        while (i < pData->NumObjectTypes)
            {
            if (pObject->ObjectNameTitleIndex == TitleIndex)
                return pObject;

            pObject = NextObject (pObject);
            i++;
            }

    return NULL;
}







DWORD GetTitleIdx(HWND hWnd, LPTSTR Title[], DWORD LastIndex, LPTSTR Name)
{
  DWORD Index;

  for(Index = 0; Index <= LastIndex; Index++)
    if(Title[Index])
      if(!lstrcmpi (Title[Index], Name))
        return(Index);

  MessageBox(hWnd, Name, TEXT("Setup cannot find process title index"), MB_OK);
  return 0;
}

