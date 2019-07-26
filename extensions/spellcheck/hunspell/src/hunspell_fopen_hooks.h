



#ifndef fopen_hooks_h__
#define fopen_hooks_h__










#include "mozilla/FileUtils.h"
#include <stdio.h>
#include <string.h>

#if defined(XP_WIN)
#include "nsString.h"

#include <fcntl.h>
#include <windows.h>

#undef near

#undef RemoveDirectory
#endif 

inline FILE*
hunspell_fopen_readahead(const char* filename, const char* mode)
{
  if (!filename || !mode) {
    return nullptr;
  }
  
  if (!strchr(mode, 'r') || strchr(mode, '+')) {
    return fopen(filename, mode);
  }
  int fd = -1;
#if defined(XP_WIN)
  HANDLE handle = INVALID_HANDLE_VALUE;
  mozilla::ReadAheadFile(NS_ConvertUTF8toUTF16(filename).get(), 0, SIZE_MAX,
                         &handle);
  if (handle == INVALID_HANDLE_VALUE) {
    return nullptr;
  }
  int flags = _O_RDONLY;
  
  if (strchr(mode, 't')) {
    
    flags |= _O_TEXT;
  }
  
  fd = _open_osfhandle((intptr_t)handle, flags);
  if (fd < 0) {
    CloseHandle(handle);
    return nullptr;
  }
#else
  mozilla::ReadAheadFile(filename, 0, SIZE_MAX, &fd);
  if (fd < 0) {
    return nullptr;
  }
#endif 

  FILE* file = fdopen(fd, mode);
  if (!file) {
    close(fd);
  }
  return file;
}

#define fopen(filename, mode) hunspell_fopen_readahead(filename, mode)

#endif 

