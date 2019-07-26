



#include <windows.h>
#include <shlobj.h>

#include "base/base_paths.h"
#include "base/files/file_path.h"
#include "base/path_service.h"
#include "base/win/scoped_co_mem.h"
#include "base/win/windows_version.h"


extern "C" IMAGE_DOS_HEADER __ImageBase;

using base::FilePath;

namespace {

bool GetQuickLaunchPath(bool default_user, FilePath* result) {
  if (default_user) {
    wchar_t system_buffer[MAX_PATH];
    system_buffer[0] = 0;
    
    
    if (FAILED(SHGetFolderPath(NULL, CSIDL_APPDATA,
                               reinterpret_cast<HANDLE>(-1), SHGFP_TYPE_CURRENT,
                               system_buffer))) {
      return false;
    }
    *result = FilePath(system_buffer);
  } else if (!PathService::Get(base::DIR_APP_DATA, result)) {
    
    
    return false;
  }
  
  
  
  
  
  *result = result->AppendASCII("Microsoft");
  *result = result->AppendASCII("Internet Explorer");
  *result = result->AppendASCII("Quick Launch");
  return true;
}

}  

namespace base {

bool PathProviderWin(int key, FilePath* result) {
  
  
  
  
  
  wchar_t system_buffer[MAX_PATH];
  system_buffer[0] = 0;

  FilePath cur;
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
    case base::DIR_PROGRAM_FILESX86:
      if (base::win::OSInfo::GetInstance()->architecture() !=
          base::win::OSInfo::X86_ARCHITECTURE) {
        if (FAILED(SHGetFolderPath(NULL, CSIDL_PROGRAM_FILESX86, NULL,
                                   SHGFP_TYPE_CURRENT, system_buffer)))
          return false;
        cur = FilePath(system_buffer);
        break;
      }
      
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
    case base::DIR_COMMON_APP_DATA:
      if (FAILED(SHGetFolderPath(NULL, CSIDL_COMMON_APPDATA, NULL,
                                 SHGFP_TYPE_CURRENT, system_buffer)))
        return false;
      cur = FilePath(system_buffer);
      break;
    case base::DIR_PROFILE:
      if (FAILED(SHGetFolderPath(NULL, CSIDL_PROFILE, NULL, SHGFP_TYPE_CURRENT,
                                 system_buffer)))
        return false;
      cur = FilePath(system_buffer);
      break;
    case base::DIR_LOCAL_APP_DATA_LOW:
      if (win::GetVersion() < win::VERSION_VISTA)
        return false;

      
      if (FAILED(SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT,
                                 system_buffer)))
        return false;
      cur = FilePath(system_buffer).DirName().AppendASCII("LocalLow");
      break;
    case base::DIR_LOCAL_APP_DATA:
      if (FAILED(SHGetFolderPath(NULL, CSIDL_LOCAL_APPDATA, NULL,
                                 SHGFP_TYPE_CURRENT, system_buffer)))
        return false;
      cur = FilePath(system_buffer);
      break;
    case base::DIR_SOURCE_ROOT: {
      FilePath executableDir;
      
      
      PathService::Get(base::DIR_EXE, &executableDir);
      cur = executableDir.DirName().DirName();
      break;
    }
    case base::DIR_APP_SHORTCUTS: {
      if (win::GetVersion() < win::VERSION_WIN8)
        return false;

      base::win::ScopedCoMem<wchar_t> path_buf;
      if (FAILED(SHGetKnownFolderPath(FOLDERID_ApplicationShortcuts, 0, NULL,
                                      &path_buf)))
        return false;

      cur = FilePath(string16(path_buf));
      break;
    }
    case base::DIR_USER_DESKTOP:
      if (FAILED(SHGetFolderPath(NULL, CSIDL_DESKTOPDIRECTORY, NULL,
                                 SHGFP_TYPE_CURRENT, system_buffer))) {
        return false;
      }
      cur = FilePath(system_buffer);
      break;
    case base::DIR_COMMON_DESKTOP:
      if (FAILED(SHGetFolderPath(NULL, CSIDL_COMMON_DESKTOPDIRECTORY, NULL,
                                 SHGFP_TYPE_CURRENT, system_buffer))) {
        return false;
      }
      cur = FilePath(system_buffer);
      break;
    case base::DIR_USER_QUICK_LAUNCH:
      if (!GetQuickLaunchPath(false, &cur))
        return false;
      break;
    case base::DIR_DEFAULT_USER_QUICK_LAUNCH:
      if (!GetQuickLaunchPath(true, &cur))
        return false;
      break;
    case base::DIR_TASKBAR_PINS:
      if (!PathService::Get(base::DIR_USER_QUICK_LAUNCH, &cur))
        return false;
      cur = cur.AppendASCII("User Pinned");
      cur = cur.AppendASCII("TaskBar");
      break;
    default:
      return false;
  }

  *result = cur;
  return true;
}

}  
