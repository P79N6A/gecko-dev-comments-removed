













#ifndef _SQLITE_OS_OS2_H_
#define _SQLITE_OS_OS2_H_




#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>








#if defined(THREADSAFE) && THREADSAFE
# include <sys/builtin.h>
# include <sys/smutex.h>
# define SQLITE_OS2_THREADS 1
#endif










typedef struct OsFile OsFile;
struct OsFile {
     int h;        
     int locked;              
     int delOnClose;          
     char *pathToDel;         
     unsigned char locktype;   
     unsigned char isOpen;   
     unsigned char fullSync;
};




#define SQLITE_TEMPNAME_SIZE 200




#define SQLITE_MIN_SLEEP_MS 1

#ifndef SQLITE_DEFAULT_FILE_PERMISSIONS
# define SQLITE_DEFAULT_FILE_PERMISSIONS 0600
#endif

#endif 
