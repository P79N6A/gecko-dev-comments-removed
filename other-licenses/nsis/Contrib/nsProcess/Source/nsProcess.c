










#define UNICODE
#define _UNICODE

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <Tlhelp32.h>
#include "ConvFunc.h"


#define NSIS_MAX_STRLEN 1024

#define SystemProcessInformation     5
#define STATUS_SUCCESS               0x00000000L
#define STATUS_INFO_LENGTH_MISMATCH  0xC0000004L

typedef struct _SYSTEM_THREAD_INFO {
  FILETIME ftCreationTime;
  DWORD dwUnknown1;
  DWORD dwStartAddress;
  DWORD dwOwningPID;
  DWORD dwThreadID;
  DWORD dwCurrentPriority;
  DWORD dwBasePriority;
  DWORD dwContextSwitches;
  DWORD dwThreadState;
  DWORD dwUnknown2;
  DWORD dwUnknown3;
  DWORD dwUnknown4;
  DWORD dwUnknown5;
  DWORD dwUnknown6;
  DWORD dwUnknown7;
} SYSTEM_THREAD_INFO;

typedef struct _SYSTEM_PROCESS_INFO {
  DWORD dwOffset;
  DWORD dwThreadCount;
  DWORD dwUnkown1[6];
  FILETIME ftCreationTime;
  DWORD dwUnkown2;
  DWORD dwUnkown3;
  DWORD dwUnkown4;
  DWORD dwUnkown5;
  DWORD dwUnkown6;
  WCHAR *pszProcessName;
  DWORD dwBasePriority;
  DWORD dwProcessID;
  DWORD dwParentProcessID;
  DWORD dwHandleCount;
  DWORD dwUnkown7;
  DWORD dwUnkown8;
  DWORD dwVirtualBytesPeak;
  DWORD dwVirtualBytes;
  DWORD dwPageFaults;
  DWORD dwWorkingSetPeak;
  DWORD dwWorkingSet;
  DWORD dwUnkown9;
  DWORD dwPagedPool;
  DWORD dwUnkown10;
  DWORD dwNonPagedPool;
  DWORD dwPageFileBytesPeak;
  DWORD dwPageFileBytes;
  DWORD dwPrivateBytes;
  DWORD dwUnkown11;
  DWORD dwUnkown12;
  DWORD dwUnkown13;
  DWORD dwUnkown14;
  SYSTEM_THREAD_INFO ati[ANYSIZE_ARRAY];
} SYSTEM_PROCESS_INFO;



#ifdef UNICODE
#define xatoiW
#define xitoaW
#else
#define xatoi
#define xitoa
#endif
#include "ConvFunc.h"


typedef struct _stack_t {
  struct _stack_t *next;
  TCHAR text[1];
} stack_t;

stack_t **g_stacktop;
TCHAR *g_variables;
unsigned int g_stringsize;

#define EXDLL_INIT()        \
{                           \
  g_stacktop=stacktop;      \
  g_variables=variables;    \
  g_stringsize=string_size; \
}


TCHAR szBuf[NSIS_MAX_STRLEN];


int FIND_PROC_BY_NAME(TCHAR *szProcessName, BOOL bTerminate);
int popinteger();
void pushinteger(int integer);
int popstring(TCHAR *str, int len);
void pushstring(const TCHAR *str, int len);



void __declspec(dllexport) _FindProcess(HWND hwndParent, int string_size,
                                      TCHAR *variables, stack_t **stacktop)
{
  EXDLL_INIT();
  {
    int nError;

    popstring(szBuf, NSIS_MAX_STRLEN);
    nError=FIND_PROC_BY_NAME(szBuf, FALSE);
    pushinteger(nError);
  }
}

void __declspec(dllexport) _KillProcess(HWND hwndParent, int string_size,
                                      TCHAR *variables, stack_t **stacktop)
{
  EXDLL_INIT();
  {
    int nError;

    popstring(szBuf, NSIS_MAX_STRLEN);
    nError=FIND_PROC_BY_NAME(szBuf, TRUE);
    pushinteger(nError);
  }
}

void __declspec(dllexport) _Unload(HWND hwndParent, int string_size,
                                      TCHAR *variables, stack_t **stacktop)
{
}

BOOL WINAPI DllMain(HANDLE hInst, ULONG ul_reason_for_call, LPVOID lpReserved)
{
  return TRUE;
}

int FIND_PROC_BY_NAME(TCHAR *szProcessName, BOOL bTerminate)


















































{
#ifndef UNICODE
  char szName[MAX_PATH];
#endif
  OSVERSIONINFO osvi;
  HMODULE hLib;
  HANDLE hProc;
  ULONG uError;
  BOOL bFound=FALSE;
  BOOL bSuccess=FALSE;
  BOOL bFailed=FALSE;

  
  osvi.dwOSVersionInfoSize=sizeof(OSVERSIONINFO);
  if (!GetVersionEx(&osvi)) return 604;

  if (osvi.dwPlatformId != VER_PLATFORM_WIN32_NT &&
      osvi.dwPlatformId != VER_PLATFORM_WIN32_WINDOWS)
    return 605;

  if (osvi.dwPlatformId == VER_PLATFORM_WIN32_NT)
  {
    

    SYSTEM_PROCESS_INFO *spi;
    SYSTEM_PROCESS_INFO *spiCount;
    DWORD dwSize=0x4000;
    DWORD dwData;
    ULONG (WINAPI *NtQuerySystemInformationPtr)(ULONG, PVOID, LONG, PULONG);

    if (hLib=LoadLibraryA("NTDLL.DLL"))
    {
      NtQuerySystemInformationPtr=(ULONG(WINAPI *)(ULONG, PVOID, LONG, PULONG))GetProcAddress(hLib, "NtQuerySystemInformation");

      if (NtQuerySystemInformationPtr)
      {
        while (1)
        {
          if (spi=LocalAlloc(LMEM_FIXED, dwSize))
          {
            uError=(*NtQuerySystemInformationPtr)(SystemProcessInformation, spi, dwSize, &dwData);

            if (uError == STATUS_SUCCESS) break;

            LocalFree(spi);

            if (uError != STATUS_INFO_LENGTH_MISMATCH)
            {
              uError=608;
              break;
            }
          }
          else
          {
            uError=608;
            break;
          }
          dwSize*=2;
        }
      }
      else uError=607;

      FreeLibrary(hLib);
    }
    else uError=606;

    if (uError != STATUS_SUCCESS) return uError;

    spiCount=spi;

    while (1)
    {
      if (spiCount->pszProcessName)
      {
#ifdef UNICODE
        if (!lstrcmpi(spiCount->pszProcessName, szProcessName))
#else
        WideCharToMultiByte(CP_ACP, 0, spiCount->pszProcessName, -1, szName, MAX_PATH, NULL, NULL);

        if (!lstrcmpi(szName, szProcessName))
#endif
        {
          
          bFound=TRUE;

          if (bTerminate == TRUE)
          {
            
            if (hProc=OpenProcess(PROCESS_TERMINATE, FALSE, spiCount->dwProcessID))
            {
              if (TerminateProcess(hProc, 0))
                bSuccess=TRUE;
              else
                bFailed=TRUE;
              CloseHandle(hProc);
            }
          }
          else break;
        }
      }
      if (spiCount->dwOffset == 0) break;
      spiCount=(SYSTEM_PROCESS_INFO *)((char *)spiCount + spiCount->dwOffset);
    }
    LocalFree(spi);
  }
  else
  {
    

    PROCESSENTRY32 pe;
    TCHAR *pName;
    HANDLE hSnapShot;
    BOOL bResult;
    HANDLE (WINAPI *CreateToolhelp32SnapshotPtr)(DWORD, DWORD);
    BOOL (WINAPI *Process32FirstPtr)(HANDLE, LPPROCESSENTRY32);
    BOOL (WINAPI *Process32NextPtr)(HANDLE, LPPROCESSENTRY32);

    if (hLib=LoadLibraryA("KERNEL32.DLL"))
    {
      CreateToolhelp32SnapshotPtr=(HANDLE(WINAPI *)(DWORD, DWORD)) GetProcAddress(hLib, "CreateToolhelp32Snapshot");
      Process32FirstPtr=(BOOL(WINAPI *)(HANDLE, LPPROCESSENTRY32)) GetProcAddress(hLib, "Process32First");
      Process32NextPtr=(BOOL(WINAPI *)(HANDLE, LPPROCESSENTRY32)) GetProcAddress(hLib, "Process32Next");

      if (CreateToolhelp32SnapshotPtr && Process32NextPtr && Process32FirstPtr)
      {
        
        if ((hSnapShot=(*CreateToolhelp32SnapshotPtr)(TH32CS_SNAPPROCESS, 0)) != INVALID_HANDLE_VALUE)
        {
          
          pe.dwSize=sizeof(PROCESSENTRY32);
          bResult=(*Process32FirstPtr)(hSnapShot, &pe);

          
          while (bResult)
          {
            
            for (pName=pe.szExeFile + lstrlen(pe.szExeFile) - 1; *pName != '\\' && *pName != '\0'; --pName);

            if (!lstrcmpi(++pName, szProcessName))
            {
              
              bFound=TRUE;

              if (bTerminate == TRUE)
              {
                
                if (hProc=OpenProcess(PROCESS_TERMINATE, FALSE, pe.th32ProcessID))
                {
                  if (TerminateProcess(hProc, 0))
                    bSuccess=TRUE;
                  else
                    bFailed=TRUE;
                  CloseHandle(hProc);
                }
              }
              else break;
            }
            
            bResult=(*Process32NextPtr)(hSnapShot, &pe);
          }
          CloseHandle(hSnapShot);
        }
        else uError=611;
      }
      else uError=610;

      FreeLibrary(hLib);
    }
    else uError=609;
  }

  if (bFound == FALSE) return 603;
  if (bTerminate == TRUE)
  {
    if (bSuccess == FALSE) return 601;
    if (bFailed == TRUE) return 602;
  }
  return 0;
}

int popinteger()
{
  TCHAR szInt[32];

  popstring(szInt, 32);
#ifdef UNICODE
  return xatoiW(szInt);
#else
  return xatoi(szInt);
#endif
}

void pushinteger(int integer)
{
  TCHAR szInt[32];

#ifdef UNICODE
  xitoaW(integer, szInt, 0);
#else
  xitoa(integer, szInt, 0);
#endif
  pushstring(szInt, 32);
}


int popstring(TCHAR *str, int len)
{
  stack_t *th;

  if (!g_stacktop || !*g_stacktop) return 1;
  th=(*g_stacktop);
  lstrcpyn(str, th->text, len);
  *g_stacktop=th->next;
  GlobalFree((HGLOBAL)th);
  return 0;
}


void pushstring(const TCHAR *str, int len)
{
  stack_t *th;

  if (!g_stacktop) return;
  th=(stack_t*)GlobalAlloc(GPTR, sizeof(stack_t) + len);
  lstrcpyn(th->text, str, len);
  th->next=*g_stacktop;
  *g_stacktop=th;
}
