





#ifndef WINDIRENT_H__
#define WINDIRENT_H__

#ifndef XP_WIN
#error This library should only be used on Windows
#endif

#include <windows.h>

struct DIR {
  explicit DIR(const WCHAR* path);
  ~DIR();
  HANDLE findHandle;
  WCHAR name[MAX_PATH];
};

struct dirent {
  dirent();
  WCHAR d_name[MAX_PATH];
};

DIR* opendir(const WCHAR* path);
int closedir(DIR* dir);
dirent* readdir(DIR* dir);

#endif  
