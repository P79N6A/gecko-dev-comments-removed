




#include <windows.h>




struct AutoLoadSystemDependencies
{
  AutoLoadSystemDependencies()
  {
    
    
    SetDllDirectory(L"");

    static LPCWSTR delayDLLs[] = { L"wsock32.dll", L"crypt32.dll",
                                   L"cryptsp.dll", L"cryptbase.dll",
                                   L"msasn1.dll", L"userenv.dll",
                                   L"secur32.dll" };

    WCHAR systemDirectory[MAX_PATH + 1] = { L'\0' };
    
    
    GetSystemDirectory(systemDirectory, MAX_PATH + 1);
    size_t systemDirLen = wcslen(systemDirectory);

    
    if (systemDirectory[systemDirLen - 1] != L'\\' && systemDirLen) {
      systemDirectory[systemDirLen] = L'\\';
      ++systemDirLen;
      
    }

    
    for (size_t i = 0; i < sizeof(delayDLLs) / sizeof(delayDLLs[0]); ++i) {
      size_t fileLen = wcslen(delayDLLs[i]);
      wcsncpy(systemDirectory + systemDirLen, delayDLLs[i], 
              MAX_PATH - systemDirLen);
      if (systemDirLen + fileLen <= MAX_PATH) {
        systemDirectory[systemDirLen + fileLen] = L'\0';
      } else {
        systemDirectory[MAX_PATH] = L'\0';
      }
      LPCWSTR fullModulePath = systemDirectory; 
      LoadLibraryW(fullModulePath);
    }
  }
} loadDLLs;
