



#include "chrome/common/chrome_paths.h"

#include "base/command_line.h"
#include "base/file_path.h"
#include "base/file_util.h"
#include "base/logging.h"
#include "base/path_service.h"
#include "base/string_util.h"
#include "base/sys_info.h"
#include "chrome/common/chrome_constants.h"
#include "chrome/common/chrome_paths_internal.h"
#include "chrome/common/chrome_switches.h"

namespace chrome {

bool GetGearsPluginPathFromCommandLine(FilePath* path) {
#ifndef NDEBUG
  
  std::wstring plugin_path = CommandLine::ForCurrentProcess()->GetSwitchValue(
      switches::kGearsPluginPathOverride);
  
  
  *path = FilePath::FromWStringHack(plugin_path);
  return !plugin_path.empty();
#else
  return false;
#endif
}

bool PathProvider(int key, FilePath* result) {
  
  switch (key) {
    case chrome::DIR_APP:
      return PathService::Get(base::DIR_MODULE, result);
    case chrome::DIR_LOGS:
#ifndef NDEBUG
      return PathService::Get(chrome::DIR_USER_DATA, result);
#else
      return PathService::Get(base::DIR_EXE, result);
#endif
    case chrome::FILE_RESOURCE_MODULE:
      return PathService::Get(base::FILE_MODULE, result);
  }

  
  
  bool create_dir = false;

  FilePath cur;
  switch (key) {
    case chrome::DIR_USER_DATA:
      if (!GetDefaultUserDataDirectory(&cur))
        return false;
      create_dir = true;
      break;
    case chrome::DIR_USER_DOCUMENTS:
      if (!GetUserDocumentsDirectory(&cur))
        return false;
      create_dir = true;
      break;
    case chrome::DIR_DEFAULT_DOWNLOADS:
      if (!GetUserDownloadsDirectory(&cur))
        return false;
      break;
    case chrome::DIR_CRASH_DUMPS:
      
      
      
      
      if (!GetDefaultUserDataDirectory(&cur))
        return false;
      cur = cur.Append(FILE_PATH_LITERAL("Crash Reports"));
      create_dir = true;
      break;
    case chrome::DIR_USER_DESKTOP:
      if (!GetUserDesktop(&cur))
        return false;
      break;
    case chrome::DIR_RESOURCES:
      if (!PathService::Get(chrome::DIR_APP, &cur))
        return false;
      cur = cur.Append(FILE_PATH_LITERAL("resources"));
      create_dir = true;
      break;
    case chrome::DIR_INSPECTOR:
      if (!PathService::Get(chrome::DIR_APP, &cur))
        return false;
      cur = cur.Append(FILE_PATH_LITERAL("resources"));
      cur = cur.Append(FILE_PATH_LITERAL("inspector"));
      break;
    case chrome::DIR_THEMES:
      if (!PathService::Get(chrome::DIR_APP, &cur))
        return false;
      cur = cur.Append(FILE_PATH_LITERAL("themes"));
      create_dir = true;
      break;
    case chrome::DIR_LOCALES:
      if (!PathService::Get(chrome::DIR_APP, &cur))
        return false;
#if defined(OS_MACOSX)
      
      
      cur = cur.DirName();
      cur = cur.Append(FILE_PATH_LITERAL("Resources"));
#else
      cur = cur.Append(FILE_PATH_LITERAL("locales"));
#endif
      create_dir = true;
      break;
    case chrome::DIR_APP_DICTIONARIES:
#if defined(OS_LINUX)
      
      
      if (!PathService::Get(chrome::DIR_USER_DATA, &cur))
        return false;
#else
      if (!PathService::Get(base::DIR_EXE, &cur))
        return false;
#endif
      cur = cur.Append(FILE_PATH_LITERAL("Dictionaries"));
      create_dir = true;
      break;
    case chrome::FILE_LOCAL_STATE:
      if (!PathService::Get(chrome::DIR_USER_DATA, &cur))
        return false;
      cur = cur.Append(chrome::kLocalStateFilename);
      break;
    case chrome::FILE_RECORDED_SCRIPT:
      if (!PathService::Get(chrome::DIR_USER_DATA, &cur))
        return false;
      cur = cur.Append(FILE_PATH_LITERAL("script.log"));
      break;
    case chrome::FILE_GEARS_PLUGIN:
      if (!GetGearsPluginPathFromCommandLine(&cur)) {
#if defined(OS_WIN)
        
        
        
        if (!PathService::Get(base::DIR_MODULE, &cur))
          return false;
        cur = cur.Append(FILE_PATH_LITERAL("gears.dll"));

        if (!file_util::PathExists(cur)) {
          if (!PathService::Get(base::DIR_EXE, &cur))
            return false;
          cur = cur.Append(FILE_PATH_LITERAL("plugins"));
          cur = cur.Append(FILE_PATH_LITERAL("gears"));
          cur = cur.Append(FILE_PATH_LITERAL("gears.dll"));
        }
#else
        
        return false;
#endif
      }
      break;
    
    
    
    case chrome::DIR_TEST_DATA:
      if (!PathService::Get(base::DIR_SOURCE_ROOT, &cur))
        return false;
      cur = cur.Append(FILE_PATH_LITERAL("chrome"));
      cur = cur.Append(FILE_PATH_LITERAL("test"));
      cur = cur.Append(FILE_PATH_LITERAL("data"));
      if (!file_util::PathExists(cur))  
        return false;
      break;
    case chrome::DIR_TEST_TOOLS:
      if (!PathService::Get(base::DIR_SOURCE_ROOT, &cur))
        return false;
      cur = cur.Append(FILE_PATH_LITERAL("chrome"));
      cur = cur.Append(FILE_PATH_LITERAL("tools"));
      cur = cur.Append(FILE_PATH_LITERAL("test"));
      if (!file_util::PathExists(cur))  
        return false;
      break;
    case chrome::FILE_PYTHON_RUNTIME:
      if (!PathService::Get(base::DIR_SOURCE_ROOT, &cur))
        return false;
      cur = cur.Append(FILE_PATH_LITERAL("third_party"));
      cur = cur.Append(FILE_PATH_LITERAL("python_24"));
      cur = cur.Append(FILE_PATH_LITERAL("python.exe"));
      if (!file_util::PathExists(cur))  
        return false;
      break;
    case chrome::FILE_TEST_SERVER:
      if (!PathService::Get(base::DIR_SOURCE_ROOT, &cur))
        return false;
      cur = cur.Append(FILE_PATH_LITERAL("net"));
      cur = cur.Append(FILE_PATH_LITERAL("tools"));
      cur = cur.Append(FILE_PATH_LITERAL("test"));
      cur = cur.Append(FILE_PATH_LITERAL("testserver"));
      cur = cur.Append(FILE_PATH_LITERAL("testserver.py"));
      if (!file_util::PathExists(cur))  
        return false;
      break;
    default:
      return false;
  }

  if (create_dir && !file_util::PathExists(cur) &&
      !file_util::CreateDirectory(cur))
    return false;

  *result = cur;
  return true;
}



void RegisterPathProvider() {
  PathService::RegisterProvider(PathProvider, PATH_START, PATH_END);
}

}  
