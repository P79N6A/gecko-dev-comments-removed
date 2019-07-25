



#include <shlwapi.h>
#include "servicebase.h"
#include "updatehelper.h"
#include "nsWindowsHelpers.h"
#define MAX_KEY_LENGTH 255







BOOL
WritePrefetchClearedReg()
{
  HKEY baseKeyRaw;
  LONG retCode = RegOpenKeyExW(HKEY_LOCAL_MACHINE, 
                               BASE_SERVICE_REG_KEY, 0,
                               KEY_WRITE | KEY_WOW64_64KEY, &baseKeyRaw);
  if (retCode != ERROR_SUCCESS) {
    LOG(("Could not open key for prefetch. (%d)\n", retCode));
    return FALSE;
  }
  nsAutoRegKey baseKey(baseKeyRaw);
  DWORD disabledValue = 1;
  if (RegSetValueExW(baseKey, L"FFPrefetchDisabled", 0, REG_DWORD,
                     reinterpret_cast<LPBYTE>(&disabledValue),
                     sizeof(disabledValue)) != ERROR_SUCCESS) {
    LOG(("Could not write prefetch cleared value to registry. (%d)\n",
         GetLastError()));
    return FALSE;
  }
  return TRUE;
}
























BOOL
ClearPrefetch(LPCWSTR prefetchProcessName)
{
  LOG(("Clearing prefetch files...\n"));
  size_t prefetchProcessNameLength = wcslen(prefetchProcessName);

  
  
  
  
  if (wcsstr(prefetchProcessName, L"..") ||
      wcsstr(prefetchProcessName, L"\\") ||
      wcsstr(prefetchProcessName, L"*") ||
      wcsstr(prefetchProcessName, L"/") ||
      prefetchProcessNameLength < 2 ||
      prefetchProcessNameLength >= (MAX_PATH - 10)) {
    LOG(("Prefetch path to clear is not safe\n"));
    return FALSE;
  }

  
  
  static WCHAR lastPrefetchProcessName[MAX_PATH - 10] = { '\0' };
  if (!wcscmp(lastPrefetchProcessName, prefetchProcessName)) {
    LOG(("Already processed process name\n"));
    return FALSE;
  }
  wcscpy(lastPrefetchProcessName, prefetchProcessName);

  
  WCHAR prefetchPath[MAX_PATH + 1];
  if (!GetWindowsDirectoryW(prefetchPath,
                            sizeof(prefetchPath) / 
                            sizeof(prefetchPath[0]))) {
    LOG(("Could not obtain windows directory\n"));
    return FALSE;
  }
  if (!PathAppendSafe(prefetchPath, L"prefetch")) {
    LOG(("Could not obtain prefetch directory\n"));
    return FALSE;
  }

  size_t prefetchDirLen = wcslen(prefetchPath);
  WCHAR prefetchSearchFile[MAX_PATH + 1];
  
  wsprintf(prefetchSearchFile, L"\\%s.EXE-*.pf", prefetchProcessName);
  
  wcscpy(prefetchPath + prefetchDirLen, prefetchSearchFile);

  
  WIN32_FIND_DATAW findFileData;
  HANDLE findHandle = FindFirstFileW(prefetchPath, &findFileData);
  if (INVALID_HANDLE_VALUE == findHandle) {
    if (GetLastError() == ERROR_FILE_NOT_FOUND) {
      LOG(("No files matching firefox.exe prefetch path.\n"));
      return TRUE;
    } else {
      LOG(("Error finding firefox.exe prefetch files. (%d)\n",
           GetLastError()));
      return FALSE;
    }
  }
  
  BOOL deletedAllFFPrefetch = TRUE;
  size_t deletedCount = 0;
  do {
    
    
    
    prefetchPath[prefetchDirLen + 1] = L'\0';

    
    LPWSTR filenameOffsetInBuffer = prefetchPath + prefetchDirLen + 1;
    if (wcslen(findFileData.cFileName) + prefetchDirLen + 1 > MAX_PATH) {
      LOG(("Error appending prefetch path %ls, path is too long. (%d)\n",
           findFileData.cFileName, GetLastError()));
      deletedAllFFPrefetch = FALSE;
      continue;
    }
    if (!PathAppendSafe(filenameOffsetInBuffer, findFileData.cFileName)) {
      LOG(("Error appending prefetch path %ls. (%d)\n", findFileData.cFileName,
           GetLastError()));
      deletedAllFFPrefetch = FALSE;
      continue;
    }

    DWORD attributes = GetFileAttributesW(prefetchPath);
    if (INVALID_FILE_ATTRIBUTES == attributes) {
      LOG(("Could not get/set attributes on prefetch file: %ls. (%d)\n", 
           findFileData.cFileName, GetLastError()));
      continue;
    }
    
    if (!(attributes & FILE_ATTRIBUTE_READONLY)) {
      LOG(("Prefetch file is not read-only, don't clear: %ls.\n", 
           findFileData.cFileName));
      continue;
    }

    
    if (!SetFileAttributesW(prefetchPath,
                            attributes & (~FILE_ATTRIBUTE_READONLY))) {
      LOG(("Could not set read only on prefetch file: %ls. (%d)\n", 
           findFileData.cFileName, GetLastError()));
      continue;
    } 

    if (!DeleteFileW(prefetchPath)) {
      LOG(("Could not delete read only prefetch file: %ls. (%d)\n",
            findFileData.cFileName, GetLastError()));
    }

    ++deletedCount;
    LOG(("Prefetch file cleared successfully: %ls\n",
         prefetchPath));
  } while (FindNextFileW(findHandle, &findFileData));
  LOG(("Done searching prefetch paths. (%d)\n", GetLastError()));

  
  FindClose(findHandle);

  if (deletedAllFFPrefetch && deletedCount > 0) {
    WritePrefetchClearedReg();
  }

  return deletedAllFFPrefetch;
}





BOOL
ClearKnownPrefetch()
{
  
  HKEY baseKeyRaw;
  LONG retCode = RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                               BASE_SERVICE_REG_KEY, 0,
                               KEY_READ | KEY_WOW64_64KEY, &baseKeyRaw);
  if (retCode != ERROR_SUCCESS) {
    LOG(("Could not open maintenance service base key. (%d)\n", retCode));
    return FALSE;
  }
  nsAutoRegKey baseKey(baseKeyRaw);

  
  DWORD subkeyCount = 0;
  retCode = RegQueryInfoKeyW(baseKey, NULL, NULL, NULL, &subkeyCount, NULL,
                             NULL, NULL, NULL, NULL, NULL, NULL);
  if (retCode != ERROR_SUCCESS) {
    LOG(("Could not query info key: %d\n", retCode));
    return FALSE;
  }

  
  for (DWORD i = 0; i < subkeyCount; i++) { 
    WCHAR subkeyBuffer[MAX_KEY_LENGTH];
    DWORD subkeyBufferCount = MAX_KEY_LENGTH;  
    retCode = RegEnumKeyExW(baseKey, i, subkeyBuffer, 
                            &subkeyBufferCount, NULL, 
                            NULL, NULL, NULL); 
    if (retCode != ERROR_SUCCESS) {
      LOG(("Could not enum installations: %d\n", retCode));
      return FALSE;
    }

    
    HKEY subKeyRaw;
    retCode = RegOpenKeyExW(baseKey, 
                            subkeyBuffer, 
                            0, 
                            KEY_READ | KEY_WOW64_64KEY, 
                            &subKeyRaw);
    nsAutoRegKey subKey(subKeyRaw);
    if (retCode != ERROR_SUCCESS) {
      LOG(("Could not open subkey: %d\n", retCode));
      continue; 
    }

    const int MAX_CHAR_COUNT = 256;
    DWORD valueBufSize = MAX_CHAR_COUNT * sizeof(WCHAR);
    WCHAR prefetchProcessName[MAX_CHAR_COUNT] = { L'\0' };

    
    retCode = RegQueryValueExW(subKey, L"prefetchProcessName", 0, NULL, 
                               (LPBYTE)prefetchProcessName, &valueBufSize);
    if (retCode != ERROR_SUCCESS) {
      LOG(("Could not obtain process name from registry: %d\n", retCode));
      continue; 
    }

    
    
    ClearPrefetch(prefetchProcessName);
  }

  return TRUE;
}
