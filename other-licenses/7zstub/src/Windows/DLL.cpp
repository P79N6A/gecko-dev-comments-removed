

#include "StdAfx.h"

#include "DLL.h"
#include "Defs.h"
#ifndef _UNICODE
#include "../Common/StringConvert.h"
#endif

#ifndef _UNICODE
extern bool g_IsNT;
#endif

namespace NWindows {
namespace NDLL {

CLibrary::~CLibrary()
{
  Free();
}

bool CLibrary::Free()
{
  if (_module == 0)
    return true;
  
  
  if (!::FreeLibrary(_module))
    return false;
  _module = 0;
  return true;
}

bool CLibrary::LoadOperations(HMODULE newModule)
{
  if (newModule == NULL)
    return false;
  if(!Free())
    return false;
  _module = newModule;
  return true;
}

bool CLibrary::LoadEx(LPCTSTR fileName, DWORD flags)
{
  
  return LoadOperations(::LoadLibraryEx(fileName, NULL, flags));
}

bool CLibrary::Load(LPCTSTR fileName)
{
  
  
  
  
  return LoadOperations(::LoadLibrary(fileName));
}

#ifndef _UNICODE
static inline UINT GetCurrentCodePage() { return ::AreFileApisANSI() ? CP_ACP : CP_OEMCP; } 
CSysString GetSysPath(LPCWSTR sysPath)
  { return UnicodeStringToMultiByte(sysPath, GetCurrentCodePage()); }

bool CLibrary::LoadEx(LPCWSTR fileName, DWORD flags)
{
  if (g_IsNT)
    return LoadOperations(::LoadLibraryExW(fileName, NULL, flags));
  return LoadEx(GetSysPath(fileName), flags);
}
bool CLibrary::Load(LPCWSTR fileName)
{
  if (g_IsNT)
    return LoadOperations(::LoadLibraryW(fileName));
  return Load(GetSysPath(fileName));
}
#endif

bool MyGetModuleFileName(HMODULE hModule, CSysString &result)
{
  result.Empty();
  TCHAR fullPath[MAX_PATH + 2];
  DWORD size = ::GetModuleFileName(hModule, fullPath, MAX_PATH + 1);
  if (size <= MAX_PATH && size != 0)
  {
    result = fullPath;
    return true;
  }
  return false;
}

#ifndef _UNICODE
bool MyGetModuleFileName(HMODULE hModule, UString &result)
{
  result.Empty();
  if (g_IsNT)
  {
    wchar_t fullPath[MAX_PATH + 2];
    DWORD size = ::GetModuleFileNameW(hModule, fullPath, MAX_PATH + 1);
    if (size <= MAX_PATH && size != 0)
    {
      result = fullPath;
      return true;
    }
    return false;
  }
  CSysString resultSys;
  if (!MyGetModuleFileName(hModule, resultSys))
    return false;
  result = MultiByteToUnicodeString(resultSys, GetCurrentCodePage());
  return true;
}
#endif

}}
