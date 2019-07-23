





































#include <windows.h>
#include <winternl.h>

#include <stdio.h>

#include "nsAutoPtr.h"

#include "prlog.h"

#include "nsWindowsDllInterceptor.h"

#define IN_WINDOWS_DLL_BLOCKLIST
#include "nsWindowsDllBlocklist.h"

typedef NTSTATUS (NTAPI *LdrLoadDll_func) (PWCHAR filePath, PULONG flags, PUNICODE_STRING moduleFileName, PHANDLE handle);

static LdrLoadDll_func stub_LdrLoadDll = 0;

static NTSTATUS NTAPI
patched_LdrLoadDll (PWCHAR filePath, PULONG flags, PUNICODE_STRING moduleFileName, PHANDLE handle)
{
  
#define DLLNAME_MAX 128
  char dllName[DLLNAME_MAX+1];

  
  
  
  
  
  
  
  
  int len = moduleFileName->Length;
  wchar_t *fn_buf = moduleFileName->Buffer;

  int count = 0;
  while (count < len && fn_buf[count] != 0)
    count++;

  len = count;

  
  nsAutoArrayPtr<wchar_t> fname = new wchar_t[len+1];
  wcsncpy(fname, moduleFileName->Buffer, len);
  fname[len] = 0; 

  wchar_t *dll_part = wcsrchr(fname, L'\\');
  if (dll_part) {
    dll_part = dll_part + 1;
    len = (fname+len) - dll_part;
  } else {
    dll_part = fname;
  }

  
  
  
  if (len > DLLNAME_MAX)
    goto continue_loading;

  
  for (int i = 0; i < len; i++) {
    wchar_t c = dll_part[i];
    if (c >= 'A' && c <= 'Z')
      c += 'a' - 'A';

    if (c > 0x7f) {
      
      
      goto continue_loading;
    }

    dllName[i] = (char) c;
  }

  dllName[len] = 0;

  
  DllBlockInfo *info = &sWindowsDllBlocklist[0];
  while (info->name) {
    if (strcmp(info->name, dllName) == 0)
      break;

    info++;
  }

  if (info->name) {
    BOOL load_ok = FALSE;

    if (info->maxVersion != ALL_VERSIONS) {
      
      DWORD pathlen = SearchPathW(filePath, fname, L".dll", 0, NULL, NULL);
      if (pathlen == 0) {
        
        return STATUS_DLL_NOT_FOUND;
      }

      nsAutoArrayPtr<wchar_t> full_fname = new wchar_t[pathlen+1];

      
      SearchPathW(filePath, fname, L".dll", pathlen+1, full_fname, NULL);

      DWORD zero;
      DWORD infoSize = GetFileVersionInfoSizeW(full_fname, &zero);

      

      if (infoSize != 0) {
        nsAutoArrayPtr<unsigned char> infoData = new unsigned char[infoSize];
        VS_FIXEDFILEINFO *vInfo;
        UINT vInfoLen;

        if (GetFileVersionInfoW(full_fname, 0, infoSize, infoData) &&
            VerQueryValueW(infoData, L"\\", (LPVOID*) &vInfo, &vInfoLen))
        {
          unsigned long long fVersion =
            ((unsigned long long)vInfo->dwFileVersionMS) << 32 |
            ((unsigned long long)vInfo->dwFileVersionLS);

          
          
          if (fVersion > info->maxVersion)
            goto continue_loading;
        }
      }
    }

    PR_LogPrint("LdrLoadDll: Blocking load of '%s'", dllName);
    return STATUS_DLL_NOT_FOUND;
  }

continue_loading:
  return stub_LdrLoadDll(filePath, flags, moduleFileName, handle);
}

WindowsDllInterceptor NtDllIntercept;

void
SetupDllBlocklist()
{
  NtDllIntercept.Init("ntdll.dll");

  bool ok = NtDllIntercept.AddHook("LdrLoadDll", patched_LdrLoadDll, (void**) &stub_LdrLoadDll);

  if (!ok)
    PR_LogPrint ("LdrLoadDll hook failed, no dll blocklisting active");
}




