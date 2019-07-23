













#ifndef _SQLITE_OS_WIN_H_
#define _SQLITE_OS_WIN_H_

#include <windows.h>
#include <winbase.h>







typedef struct OsFile OsFile;
struct OsFile {
  HANDLE h;               
  unsigned char locktype; 
  unsigned char isOpen;   
  short sharedLockByte;   
};


#define SQLITE_TEMPNAME_SIZE (MAX_PATH+50)
#define SQLITE_MIN_SLEEP_MS 1


#endif 
