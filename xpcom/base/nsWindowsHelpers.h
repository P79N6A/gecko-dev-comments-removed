



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

typedef nsAutoRef<HKEY> nsAutoRegKey;
typedef nsAutoRef<SC_HANDLE> nsAutoServiceHandle;
typedef nsAutoRef<HANDLE> nsAutoHandle;

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
}

#endif
