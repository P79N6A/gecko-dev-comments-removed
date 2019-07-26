







































#ifndef CTEMPLATE_WINDOWS_PORT_H_
#define CTEMPLATE_WINDOWS_PORT_H_

#include "config.h"

#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock.h>         
#include <io.h>              
#include <direct.h>          
#include <process.h>         
#include <stdio.h>           
#include <stdarg.h>          
#include <string.h>          
#include <time.h>            











#pragma warning(disable:4244 4251 4355 4715 4800 4996)


#define PATH_MAX 1024
#define access  _access
#define getcwd  _getcwd
#define open    _open
#define read    _read
#define write   _write
#define lseek   _lseek
#define close   _close
#define popen   _popen
#define pclose  _pclose
#define R_OK    04           /* read-only (for access()) */
#define S_ISDIR(m)  (((m) & _S_IFMT) == _S_IFDIR)
#ifndef __MINGW32__
enum { STDIN_FILENO = 0, STDOUT_FILENO = 1, STDERR_FILENO = 2 };
#endif
#define S_IRUSR S_IREAD
#define S_IWUSR S_IWRITE


#define link(oldpath, newpath)  CopyFileA(oldpath, newpath, false)

#define strcasecmp   _stricmp
#define strncasecmp  _strnicmp


#define hash  hash_compare


#define sleep(secs)  Sleep((secs) * 1000)





extern int snprintf(char *str, size_t size,
                                       const char *format, ...);
extern int safe_vsnprintf(char *str, size_t size,
                          const char *format, va_list ap);
#define vsnprintf(str, size, format, ap)  safe_vsnprintf(str, size, format, ap)
#define va_copy(dst, src)  (dst) = (src)




#define CTEMPLATE_SMALL_HASHTABLE

#define DEFAULT_TEMPLATE_ROOTDIR  ".."


typedef int pid_t;
#define getpid  _getpid


typedef DWORD pthread_t;
typedef DWORD pthread_key_t;
typedef LONG pthread_once_t;
enum { PTHREAD_ONCE_INIT = 0 };   
#define pthread_self  GetCurrentThreadId
#define pthread_equal(pthread_t_1, pthread_t_2)  ((pthread_t_1)==(pthread_t_2))

inline struct tm* localtime_r(const time_t* timep, struct tm* result) {
  localtime_s(result, timep);
  return result;
}

inline char* strerror_r(int errnum, char* buf, size_t buflen) {
  strerror_s(buf, buflen, errnum);
  return buf;
}

#ifndef __cplusplus

#define inline
#endif

#endif  

#endif  
