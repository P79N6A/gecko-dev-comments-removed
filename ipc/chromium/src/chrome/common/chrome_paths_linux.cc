



#include "chrome/common/chrome_paths_internal.h"

#include <glib.h>
#include <stdlib.h>

#include "base/file_path.h"
#include "base/path_service.h"
#ifndef CHROMIUM_MOZILLA_BUILD
#include "chrome/third_party/xdg_user_dirs/xdg_user_dir_lookup.h"
#endif 

namespace {

FilePath GetHomeDir() {
  const char *home_dir = getenv("HOME");

  if (home_dir && home_dir[0])
    return FilePath(home_dir);

  home_dir = g_get_home_dir();
  if (home_dir && home_dir[0])
    return FilePath(home_dir);

  FilePath rv;
  if (PathService::Get(base::DIR_TEMP, &rv))
    return rv;

  
  return FilePath("/tmp/");
}



FilePath GetXDGUserDirectory(const char* env_name, const char* fallback_dir) {
#ifndef CHROMIUM_MOZILLA_BUILD
  char* xdg_dir = xdg_user_dir_lookup(env_name);
  if (xdg_dir) {
    FilePath rv(xdg_dir);
    free(xdg_dir);
    return rv;
  }
#endif 
  return GetHomeDir().Append(fallback_dir);
}




FilePath GetXDGDirectory(const char* env_name, const char* fallback_dir) {
  const char* env_value = getenv(env_name);
  if (env_value && env_value[0])
    return FilePath(env_value);
  return GetHomeDir().Append(fallback_dir);
}

}  

namespace chrome {






bool GetDefaultUserDataDirectory(FilePath* result) {
  FilePath config_dir(GetXDGDirectory("XDG_CONFIG_HOME", ".config"));
#if defined(GOOGLE_CHROME_BUILD)
  *result = config_dir.Append("google-chrome");
#else
  *result = config_dir.Append("chromium");
#endif
  return true;
}

bool GetUserDocumentsDirectory(FilePath* result) {
  *result = GetXDGUserDirectory("DOCUMENTS", "Documents");
  return true;
}



bool GetUserDownloadsDirectory(FilePath* result) {
  *result = GetXDGUserDirectory("DOWNLOAD", "Downloads");

  FilePath home = GetHomeDir();
  if (*result == home) {
    *result = home.Append("Downloads");
    return true;
  }

  FilePath desktop;
  GetUserDesktop(&desktop);
  if (*result == desktop) {
    *result = home.Append("Downloads");
  }

  return true;
}

bool GetUserDesktop(FilePath* result) {
  *result = GetXDGUserDirectory("DESKTOP", "Desktop");
  return true;
}

}  
