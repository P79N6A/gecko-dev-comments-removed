





#ifndef mozilla_FileUtilsWin_h
#define mozilla_FileUtilsWin_h

#include <windows.h>

#include "mozilla/Scoped.h"
#include "nsStringGlue.h"

namespace mozilla {

inline bool
NtPathToDosPath(const nsAString& aNtPath, nsAString& aDosPath)
{
  aDosPath.Truncate();
  if (aNtPath.IsEmpty()) {
    return true;
  }
  NS_NAMED_LITERAL_STRING(symLinkPrefix, "\\??\\");
  uint32_t ntPathLen = aNtPath.Length();
  uint32_t symLinkPrefixLen = symLinkPrefix.Length();
  if (ntPathLen >= 6 && aNtPath.CharAt(5) == L':' &&
      ntPathLen >= symLinkPrefixLen &&
      Substring(aNtPath, 0, symLinkPrefixLen).Equals(symLinkPrefix)) {
    
    aDosPath = aNtPath;
    aDosPath.Cut(0, 4);
    return true;
  }
  nsAutoString logicalDrives;
  DWORD len = 0;
  while (true) {
    len = GetLogicalDriveStringsW(
      len, reinterpret_cast<wchar_t*>(logicalDrives.BeginWriting()));
    if (!len) {
      return false;
    } else if (len > logicalDrives.Length()) {
      logicalDrives.SetLength(len);
    } else {
      break;
    }
  }
  const char16_t* cur = logicalDrives.BeginReading();
  const char16_t* end = logicalDrives.EndReading();
  nsString targetPath;
  targetPath.SetLength(MAX_PATH);
  wchar_t driveTemplate[] = L" :";
  do {
    
    
    driveTemplate[0] = *cur;
    DWORD targetPathLen = 0;
    SetLastError(ERROR_SUCCESS);
    while (true) {
      targetPathLen = QueryDosDeviceW(driveTemplate,
                                      reinterpret_cast<wchar_t*>(targetPath.BeginWriting()),
                                      targetPath.Length());
      if (targetPathLen || GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
        break;
      }
      targetPath.SetLength(targetPath.Length() * 2);
    }
    if (targetPathLen) {
      
      size_t firstTargetPathLen = wcslen(targetPath.get());
      const char16_t* pathComponent = aNtPath.BeginReading() +
                                      firstTargetPathLen;
      bool found = _wcsnicmp(char16ptr_t(aNtPath.BeginReading()), targetPath.get(),
                             firstTargetPathLen) == 0 &&
                   *pathComponent == L'\\';
      if (found) {
        aDosPath = driveTemplate;
        aDosPath += pathComponent;
        return true;
      }
    }
    
    while (*cur++);
  } while (cur != end);
  
  
  NS_NAMED_LITERAL_STRING(uncPrefix, "\\\\");
  NS_NAMED_LITERAL_STRING(deviceMupPrefix, "\\Device\\Mup\\");
  if (StringBeginsWith(aNtPath, deviceMupPrefix)) {
    aDosPath = uncPrefix;
    aDosPath += Substring(aNtPath, deviceMupPrefix.Length());
    return true;
  }
  NS_NAMED_LITERAL_STRING(deviceLanmanRedirectorPrefix,
                          "\\Device\\LanmanRedirector\\");
  if (StringBeginsWith(aNtPath, deviceLanmanRedirectorPrefix)) {
    aDosPath = uncPrefix;
    aDosPath += Substring(aNtPath, deviceLanmanRedirectorPrefix.Length());
    return true;
  }
  return false;
}

bool
HandleToFilename(HANDLE aHandle, const LARGE_INTEGER& aOffset,
                 nsAString& aFilename);

} 

#endif 
