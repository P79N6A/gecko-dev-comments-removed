



#include "chrome/common/chrome_paths_internal.h"

#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>

#include "base/file_path.h"
#include "base/path_service.h"
#include "chrome/common/chrome_constants.h"

namespace chrome {

bool GetDefaultUserDataDirectory(FilePath* result) {
  if (!PathService::Get(base::DIR_LOCAL_APP_DATA, result))
    return false;
#if defined(GOOGLE_CHROME_BUILD)
  *result = result->Append(FILE_PATH_LITERAL("Google"));
#endif
  *result = result->Append(chrome::kBrowserAppName);
  *result = result->Append(chrome::kUserDataDirname);
  return true;
}

bool GetUserDocumentsDirectory(FilePath* result) {
  wchar_t path_buf[MAX_PATH];
  if (FAILED(SHGetFolderPath(NULL, CSIDL_MYDOCUMENTS, NULL,
                             SHGFP_TYPE_CURRENT, path_buf)))
    return false;
  *result = FilePath(path_buf);
  return true;
}







bool GetUserDownloadsDirectory(FilePath* result) {
  if (!GetUserDocumentsDirectory(result))
    return false;

  *result = result->Append(L"Downloads");
  return true;
}

bool GetUserDesktop(FilePath* result) {
  
  
  
  
  
  wchar_t system_buffer[MAX_PATH];
  system_buffer[0] = 0;
  if (FAILED(SHGetFolderPath(NULL, CSIDL_DESKTOPDIRECTORY, NULL,
                             SHGFP_TYPE_CURRENT, system_buffer)))
    return false;
  *result = FilePath(system_buffer);
  return true;
}

}  
