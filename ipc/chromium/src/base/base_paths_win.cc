



#include "base/base_paths_win.h"

#include <windows.h>
#include <shlobj.h>

#include "base/file_path.h"
#include "base/file_util.h"
#include "base/path_service.h"
#include "base/win_util.h"


extern "C" IMAGE_DOS_HEADER __ImageBase;

namespace base {

bool PathProviderWin(int key, FilePath* result) {

  
  
  
  
  
  wchar_t system_buffer[MAX_PATH];
  system_buffer[0] = 0;

  FilePath cur;
  std::wstring wstring_path;
  switch (key) {
    case base::FILE_EXE:
      GetModuleFileName(NULL, system_buffer, MAX_PATH);
      cur = FilePath(system_buffer);
      break;
    case base::FILE_MODULE: {
      
      
      HMODULE this_module = reinterpret_cast<HMODULE>(&__ImageBase);
      GetModuleFileName(this_module, system_buffer, MAX_PATH);
      cur = FilePath(system_buffer);
      break;
    }
    case base::DIR_WINDOWS:
      GetWindowsDirectory(system_buffer, MAX_PATH);
      cur = FilePath(system_buffer);
      break;
    case base::DIR_SYSTEM:
      GetSystemDirectory(system_buffer, MAX_PATH);
      cur = FilePath(system_buffer);
      break;
    case base::DIR_PROGRAM_FILES:
      if (FAILED(SHGetFolderPath(NULL, CSIDL_PROGRAM_FILES, NULL,
                                 SHGFP_TYPE_CURRENT, system_buffer)))
        return false;
      cur = FilePath(system_buffer);
      break;
    case base::DIR_IE_INTERNET_CACHE:
      if (FAILED(SHGetFolderPath(NULL, CSIDL_INTERNET_CACHE, NULL,
                                 SHGFP_TYPE_CURRENT, system_buffer)))
        return false;
      cur = FilePath(system_buffer);
      break;
    case base::DIR_COMMON_START_MENU:
      if (FAILED(SHGetFolderPath(NULL, CSIDL_COMMON_PROGRAMS, NULL,
                                 SHGFP_TYPE_CURRENT, system_buffer)))
        return false;
      cur = FilePath(system_buffer);
      break;
    case base::DIR_START_MENU:
      if (FAILED(SHGetFolderPath(NULL, CSIDL_PROGRAMS, NULL,
                                 SHGFP_TYPE_CURRENT, system_buffer)))
        return false;
      cur = FilePath(system_buffer);
      break;
    case base::DIR_APP_DATA:
      if (FAILED(SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT,
                                 system_buffer)))
        return false;
      cur = FilePath(system_buffer);
      break;
    case base::DIR_LOCAL_APP_DATA_LOW:
      if (win_util::GetWinVersion() < win_util::WINVERSION_VISTA) {
        return false;
      }
      
      if (FAILED(SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT,
                                 system_buffer)))
        return false;
      wstring_path = system_buffer;
      file_util::UpOneDirectory(&wstring_path);
      file_util::AppendToPath(&wstring_path, L"LocalLow");
      cur = FilePath(wstring_path);
      break;
    case base::DIR_LOCAL_APP_DATA:
      if (FAILED(SHGetFolderPath(NULL, CSIDL_LOCAL_APPDATA, NULL,
                                 SHGFP_TYPE_CURRENT, system_buffer)))
        return false;
      cur = FilePath(system_buffer);
      break;
    case base::DIR_SOURCE_ROOT:
      
      
      PathService::Get(base::DIR_EXE, &wstring_path);
      file_util::UpOneDirectory(&wstring_path);
      file_util::UpOneDirectory(&wstring_path);
      cur = FilePath(wstring_path);
      break;
    default:
      return false;
  }

  *result = cur;
  return true;
}

}  
