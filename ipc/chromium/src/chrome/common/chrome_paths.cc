



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
  return true;
}



void RegisterPathProvider() {
  PathService::RegisterProvider(PathProvider, PATH_START, PATH_END);
}

}  
