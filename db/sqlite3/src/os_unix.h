













#ifndef _SQLITE_OS_UNIX_H_
#define _SQLITE_OS_UNIX_H_





















#ifndef SQLITE_DISABLE_LFS
# define _LARGE_FILE       1
# ifndef _FILE_OFFSET_BITS
#   define _FILE_OFFSET_BITS 64
# endif
# define _LARGEFILE_SOURCE 1
#endif




#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>







#if defined(THREADSAFE) && THREADSAFE
# include <pthread.h>
# define SQLITE_UNIX_THREADS 1
#endif










typedef struct OsFile OsFile;
struct OsFile {
  struct Pager *pPager;     
  struct openCnt *pOpen;    
  struct lockInfo *pLock;   
  int h;                    
  unsigned char locktype;   
  unsigned char isOpen;     
  unsigned char fullSync;   
  int dirfd;                
#ifdef SQLITE_UNIX_THREADS
  pthread_t tid;            
#endif
};




#define SET_FULLSYNC(x,y)  ((x).fullSync = (y))




#define SQLITE_TEMPNAME_SIZE 200




#if defined(HAVE_USLEEP) && HAVE_USLEEP
# define SQLITE_MIN_SLEEP_MS 1
#else
# define SQLITE_MIN_SLEEP_MS 1000
#endif




#ifndef SQLITE_DEFAULT_FILE_PERMISSIONS
# define SQLITE_DEFAULT_FILE_PERMISSIONS 0644
#endif


#endif 
