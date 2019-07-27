




#include <windows.h>




struct AutoLoadSystemDependencies
{
  AutoLoadSystemDependencies()
  {
    
    
    SetDllDirectory(L"");

    HMODULE module = ::GetModuleHandleW(L"kernel32.dll");
    if (module) {
      
      
      
      decltype(SetDefaultDllDirectories)* setDefaultDllDirectories =
        (decltype(SetDefaultDllDirectories)*) GetProcAddress(module, "SetDefaultDllDirectories");
      if (setDefaultDllDirectories) {
        setDefaultDllDirectories(LOAD_LIBRARY_SEARCH_SYSTEM32);
        return;
      }
    }

    
    
    
#ifdef HAVE_64BIT_BUILD
    
    
    
    static LPCWSTR delayDLLs[] = { L"apphelp.dll",
                                   L"cryptbase.dll",
                                   L"cryptsp.dll",
                                   L"dwmapi.dll",
                                   L"mpr.dll",
                                   L"ntmarta.dll",
                                   L"profapi.dll",
                                   L"propsys.dll",
                                   L"sspicli.dll",
                                   L"wsock32.dll" };

#else
    
    
    
    static LPCWSTR delayDLLs[] = { L"apphelp.dll",
                                   L"crypt32.dll",
                                   L"cryptbase.dll",
                                   L"cryptsp.dll",
                                   L"dwmapi.dll",
                                   L"mpr.dll",
                                   L"msasn1.dll",
                                   L"ntmarta.dll",
                                   L"profapi.dll",
                                   L"propsys.dll",
                                   L"psapi.dll",
                                   L"secur32.dll",
                                   L"sspicli.dll",
                                   L"userenv.dll",
                                   L"uxtheme.dll",
                                   L"ws2_32.dll",
                                   L"ws2help.dll",
                                   L"wsock32.dll" };
#endif

    WCHAR systemDirectory[MAX_PATH + 1] = { L'\0' };
    
    
    GetSystemDirectoryW(systemDirectory, MAX_PATH + 1);
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
      
      
      LoadLibraryExW(fullModulePath, nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
    }
  }
} loadDLLs;
