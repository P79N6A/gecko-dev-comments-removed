




































#include "win_dirent.h"
#include <errno.h>
#include <string.h>





static dirent gDirEnt;

DIR::DIR(const WCHAR* path)
  : findHandle(INVALID_HANDLE_VALUE)
{
  memset(name, 0, sizeof(name));
  wcsncpy(name, path, sizeof(name)/sizeof(name[0]));
  wcsncat(name, L"\\*", sizeof(name)/sizeof(name[0]) - wcslen(name) - 1);
}

DIR::~DIR()
{
  if (findHandle != INVALID_HANDLE_VALUE) {
    FindClose(findHandle);
  }
}

dirent::dirent()
{
  d_name[0] = L'\0';
}

DIR*
opendir(const WCHAR* path)
{
  return new DIR(path);
}

int
closedir(DIR* dir)
{
  delete dir;
  return 0;
}

dirent* readdir(DIR* dir)
{
  WIN32_FIND_DATAW data;
  if (dir->findHandle != INVALID_HANDLE_VALUE) {
    BOOL result = FindNextFileW(dir->findHandle, &data);
    if (!result) {
      if (GetLastError() != ERROR_FILE_NOT_FOUND) {
        errno = ENOENT;
      }
      return 0;
    }
  } else {
    
    dir->findHandle = FindFirstFileW(dir->name, &data);
    if (dir->findHandle == INVALID_HANDLE_VALUE) {
      if (GetLastError() == ERROR_FILE_NOT_FOUND) {
        errno = ENOENT;
      } else {
        errno = EBADF;
      }
      return 0;
    }
  }
  memset(gDirEnt.d_name, 0, sizeof(gDirEnt.d_name));
  wcsncpy(gDirEnt.d_name, data.cFileName,
           sizeof(gDirEnt.d_name)/sizeof(gDirEnt.d_name[0]));
  return &gDirEnt;
}

