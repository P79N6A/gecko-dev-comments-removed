




































#ifndef UPDATER_WINCE_H
#define UPDATER_WINCE_H

#include "environment.h"

#define _S_IFDIR    0040000 /* stat, is a directory */
#define _S_IFREG    0100000 /* stat, is a normal file */
#define _S_IREAD    0000400 /* stat, can read */
#define _S_IWRITE   0000200 /* stat, can write */
#define _S_IEXEC    0000100

#define BUFSIZ 1024
#define _putenv putenv

struct stat {
  unsigned short st_mode;
  size_t st_size;
  time_t st_ctime;
  time_t st_atime;
  time_t st_mtime;
};
extern int errno;
int _wchmod(const WCHAR* path, unsigned int mode);
int fstat(FILE* handle, struct stat* buff);
int stat(const char* path, struct stat* buf);
int _wstat(const WCHAR* path, struct stat* buf);
int _wmkdir(const WCHAR* path);
int access(const char* path, int amode);
int _waccess(const WCHAR* path, int amode);
int _wremove(const WCHAR* wpath);

#endif
