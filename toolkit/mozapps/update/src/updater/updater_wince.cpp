




































#include <windows.h>
#include "updater_wince.h"

# define F_OK 00
# define W_OK 02
# define R_OK 04

int remove(const char* path) 
{
  if (!_unlink(path)) {
    return 0;
  } 
  else if (GetLastError() == ERROR_ACCESS_DENIED) {
    WCHAR wpath[MAX_PATH];
  
    MultiByteToWideChar(CP_ACP,
			0,
			path,
			-1,
			wpath,
			MAX_PATH );
    return RemoveDirectoryW(wpath) ? 0:-1;
  } 
  else {
    return -1;
  }
}

int chmod(const char* path, unsigned int mode) 
{
  return 0;
}

int fstat(FILE* handle, struct stat* buff)
{
    int position = ftell(handle);
    if (position < 0)
        return -1;

    if (fseek(handle, 0, SEEK_END) < 0)
        return -1;

    buff->st_size = ftell(handle);

    if (fseek(handle, position, SEEK_SET) < 0)
        return -1;

    if (buff->st_size < 0)
        return -1;

    buff->st_mode = _S_IFREG | _S_IREAD | _S_IWRITE | _S_IEXEC;
    
    buff->st_ctime = 0;
    buff->st_atime = 0;
    buff->st_mtime = 0;
    return 0;
}

int stat(const char* path, struct stat* buf) 
{
  FILE* f = fopen(path, "r");
  int rv = fstat(f, buf);
  fclose(f);
  return rv;
}

int _mkdir(const char* path) 
{
  WCHAR wpath[MAX_PATH];
  MultiByteToWideChar(CP_ACP,
		      0,
		      path,
		      -1,
		      wpath,
		      MAX_PATH );
  return CreateDirectoryW(wpath, NULL) ? 0 : -1;
}

FILE* fileno(FILE* f) 
{
  return f;
}

int _access(const char* path, int amode) 
{
  WCHAR wpath[MAX_PATH];
  MultiByteToWideChar(CP_ACP,
		      0,
		      path,
		      -1,
		      wpath,
		      MAX_PATH );
   switch (amode) {
    case R_OK:
      return (GetFileAttributesW(wpath) != INVALID_FILE_ATTRIBUTES);
    default:
      return 0;
  }
}
int _waccess(const WCHAR* path, int amode)
{
  switch (amode) {
    case R_OK:
      return (GetFileAttributesW(path) != INVALID_FILE_ATTRIBUTES);
    default:
      return 0;
  }

}

int _wremove(const WCHAR* wpath) 
{
  if (DeleteFileW(wpath)) 
  {
    return 0;
  } 
  else if (GetLastError() == ERROR_ACCESS_DENIED) {
    return RemoveDirectoryW(wpath) ? 0:-1;
  } 
  else {
    return -1;
  }
}
