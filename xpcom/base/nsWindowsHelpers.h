



#ifndef nsWindowsHelpers_h
#define nsWindowsHelpers_h

#include <windows.h>
#include "nsAutoRef.h"
#include "nscore.h"

template<>
class nsAutoRefTraits<HKEY>
{
public:
  typedef HKEY RawRef;
  static HKEY Void()
  {
    return NULL;
  }

  static void Release(RawRef aFD)
  {
    if (aFD != Void()) {
      RegCloseKey(aFD);
    }
  }
};

template<>
class nsAutoRefTraits<SC_HANDLE>
{
public:
  typedef SC_HANDLE RawRef;
  static SC_HANDLE Void()
  {
    return NULL;
  }

  static void Release(RawRef aFD)
  {
    if (aFD != Void()) {
      CloseServiceHandle(aFD);
    }
  }
};

template<>
class nsSimpleRef<HANDLE>
{
protected:
  typedef HANDLE RawRef;

  nsSimpleRef() : mRawRef(NULL)
  {
  }

  nsSimpleRef(RawRef aRawRef) : mRawRef(aRawRef)
  {
  }

  bool HaveResource() const
  {
    return mRawRef != NULL && mRawRef != INVALID_HANDLE_VALUE;
  }

public:
  RawRef get() const
  {
    return mRawRef;
  }

  static void Release(RawRef aRawRef)
  {
    if (aRawRef != NULL && aRawRef != INVALID_HANDLE_VALUE) {
      CloseHandle(aRawRef);
    }
  }
  RawRef mRawRef;
};


template<>
class nsAutoRefTraits<HMODULE>
{
public:
  typedef HMODULE RawRef;
  static RawRef Void()
  {
    return NULL;
  }

  static void Release(RawRef aFD)
  {
    if (aFD != Void()) {
      FreeLibrary(aFD);
    }
  }
};

typedef nsAutoRef<HKEY> nsAutoRegKey;
typedef nsAutoRef<SC_HANDLE> nsAutoServiceHandle;
typedef nsAutoRef<HANDLE> nsAutoHandle;
typedef nsAutoRef<HMODULE> nsModuleHandle;

namespace
{
  bool
  IsVistaOrLater()
  {
    OSVERSIONINFO info;
    ZeroMemory(&info, sizeof(OSVERSIONINFO));
    info.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&info);
    return info.dwMajorVersion >= 6;
  }

  bool
  IsRunningInWindowsMetro()
  {
    static bool alreadyChecked = false;
    static bool isMetro = false;
    if (alreadyChecked) {
      return isMetro;
    }

    HMODULE user32DLL = LoadLibraryW(L"user32.dll");
    if (!user32DLL) {
      return false;
    }

    typedef BOOL (WINAPI* IsImmersiveProcessFunc)(HANDLE process);
    IsImmersiveProcessFunc IsImmersiveProcessPtr =
      (IsImmersiveProcessFunc)GetProcAddress(user32DLL,
                                              "IsImmersiveProcess");
    FreeLibrary(user32DLL);
    if (!IsImmersiveProcessPtr) {
      
      alreadyChecked = true;
      return false;
    }

    isMetro = IsImmersiveProcessPtr(GetCurrentProcess());
    alreadyChecked = true;
    return isMetro;
  }

  HMODULE
  LoadLibrarySystem32(LPCWSTR module)
  {
    WCHAR systemPath[MAX_PATH + 1] = { L'\0' };

    
    
    GetSystemDirectoryW(systemPath, MAX_PATH + 1);
    size_t systemDirLen = wcslen(systemPath);

    
    if (systemDirLen && systemPath[systemDirLen - 1] != L'\\') {
      systemPath[systemDirLen] = L'\\';
      ++systemDirLen;
      
    }

    size_t fileLen = wcslen(module);
    wcsncpy(systemPath + systemDirLen, module,
            MAX_PATH - systemDirLen);
    if (systemDirLen + fileLen <= MAX_PATH) {
      systemPath[systemDirLen + fileLen] = L'\0';
    } else {
      systemPath[MAX_PATH] = L'\0';
    }
    return LoadLibraryW(systemPath);
  }
}

#endif
