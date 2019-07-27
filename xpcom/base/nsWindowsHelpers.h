





#ifndef nsWindowsHelpers_h
#define nsWindowsHelpers_h

#include <windows.h>
#include "nsAutoRef.h"
#include "nscore.h"





class AutoCriticalSection
{
public:
  AutoCriticalSection(LPCRITICAL_SECTION aSection)
    : mSection(aSection)
  {
    ::EnterCriticalSection(mSection);
  }
  ~AutoCriticalSection()
  {
    ::LeaveCriticalSection(mSection);
  }
private:
  LPCRITICAL_SECTION mSection;
};

template<>
class nsAutoRefTraits<HKEY>
{
public:
  typedef HKEY RawRef;
  static HKEY Void()
  {
    return nullptr;
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
    return nullptr;
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

  nsSimpleRef() : mRawRef(nullptr)
  {
  }

  nsSimpleRef(RawRef aRawRef) : mRawRef(aRawRef)
  {
  }

  bool HaveResource() const
  {
    return mRawRef && mRawRef != INVALID_HANDLE_VALUE;
  }

public:
  RawRef get() const
  {
    return mRawRef;
  }

  static void Release(RawRef aRawRef)
  {
    if (aRawRef && aRawRef != INVALID_HANDLE_VALUE) {
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
    return nullptr;
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

namespace {

HMODULE inline
LoadLibrarySystem32(LPCWSTR aModule)
{
  WCHAR systemPath[MAX_PATH + 1] = { L'\0' };

  
  
  GetSystemDirectoryW(systemPath, MAX_PATH + 1);
  size_t systemDirLen = wcslen(systemPath);

  
  if (systemDirLen && systemPath[systemDirLen - 1] != L'\\') {
    systemPath[systemDirLen] = L'\\';
    ++systemDirLen;
    
  }

  size_t fileLen = wcslen(aModule);
  wcsncpy(systemPath + systemDirLen, aModule,
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
