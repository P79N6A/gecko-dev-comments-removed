















#ifndef _SQLITE_OS_H_
#define _SQLITE_OS_H_





#if !defined(OS_UNIX) && !defined(OS_BEOS) && !defined(OS_OTHER)
# define OS_OTHER 0
# ifndef OS_WIN
#   if defined(_WIN32) || defined(WIN32) || defined(__CYGWIN__) || defined(__MINGW32__) || defined(__BORLANDC__)
#     define OS_WIN 1
#     define OS_UNIX 0
#     define OS_OS2 0
#     define OS_BEOS 0
#   elif defined(__BEOS__)
#     define OS_BEOS 1
#     define OS_WIN 0
#     define OS_OS2 0
#     define OS_UNIX 0
#   elif defined(_EMX_) || defined(_OS2) || defined(OS2) || defined(OS_OS2)
#     define OS_WIN 0
#     define OS_UNIX 0
#     define OS_OS2 1
#     define OS_BEOS 0
#   else
#     define OS_WIN 0
#     define OS_UNIX 1
#     define OS_OS2 0
#     define OS_BEOS 0
#  endif
# else
#  define OS_UNIX 0
#  define OS_OS2 0
# endif
#else
# ifndef OS_WIN
#  define OS_WIN 0
# endif
#endif





#if OS_WIN
# include <windows.h>
# define SQLITE_TEMPNAME_SIZE (MAX_PATH+50)
#elif OS_OS2
# define INCL_DOSDATETIME
# define INCL_DOSFILEMGR
# define INCL_DOSERRORS
# define INCL_DOSMISC
# define INCL_DOSPROCESS
# include <os2.h>
# define SQLITE_TEMPNAME_SIZE (CCHMAXPATHCOMP)
#else
# define SQLITE_TEMPNAME_SIZE 200
#endif




#ifndef SET_FULLSYNC
# define SET_FULLSYNC(x,y)
#endif










#ifndef TEMP_FILE_PREFIX
# define TEMP_FILE_PREFIX "sqlite_"
#endif




#if OS_UNIX
#define sqlite3OsOpenReadWrite      sqlite3UnixOpenReadWrite
#define sqlite3OsOpenExclusive      sqlite3UnixOpenExclusive
#define sqlite3OsOpenReadOnly       sqlite3UnixOpenReadOnly
#define sqlite3OsDelete             sqlite3UnixDelete
#define sqlite3OsFileExists         sqlite3UnixFileExists
#define sqlite3OsFullPathname       sqlite3UnixFullPathname
#define sqlite3OsIsDirWritable      sqlite3UnixIsDirWritable
#define sqlite3OsSyncDirectory      sqlite3UnixSyncDirectory
#define sqlite3OsTempFileName       sqlite3UnixTempFileName
#define sqlite3OsRandomSeed         sqlite3UnixRandomSeed
#define sqlite3OsSleep              sqlite3UnixSleep
#define sqlite3OsCurrentTime        sqlite3UnixCurrentTime
#define sqlite3OsEnterMutex         sqlite3UnixEnterMutex
#define sqlite3OsLeaveMutex         sqlite3UnixLeaveMutex
#define sqlite3OsInMutex            sqlite3UnixInMutex
#define sqlite3OsThreadSpecificData sqlite3UnixThreadSpecificData
#define sqlite3OsMalloc             sqlite3GenericMalloc
#define sqlite3OsRealloc            sqlite3GenericRealloc
#define sqlite3OsFree               sqlite3GenericFree
#define sqlite3OsAllocationSize     sqlite3GenericAllocationSize
#endif
#if OS_WIN
#define sqlite3OsOpenReadWrite      sqlite3WinOpenReadWrite
#define sqlite3OsOpenExclusive      sqlite3WinOpenExclusive
#define sqlite3OsOpenReadOnly       sqlite3WinOpenReadOnly
#define sqlite3OsDelete             sqlite3WinDelete
#define sqlite3OsFileExists         sqlite3WinFileExists
#define sqlite3OsFullPathname       sqlite3WinFullPathname
#define sqlite3OsIsDirWritable      sqlite3WinIsDirWritable
#define sqlite3OsSyncDirectory      sqlite3WinSyncDirectory
#define sqlite3OsTempFileName       sqlite3WinTempFileName
#define sqlite3OsRandomSeed         sqlite3WinRandomSeed
#define sqlite3OsSleep              sqlite3WinSleep
#define sqlite3OsCurrentTime        sqlite3WinCurrentTime
#define sqlite3OsEnterMutex         sqlite3WinEnterMutex
#define sqlite3OsLeaveMutex         sqlite3WinLeaveMutex
#define sqlite3OsInMutex            sqlite3WinInMutex
#define sqlite3OsThreadSpecificData sqlite3WinThreadSpecificData
#define sqlite3OsMalloc             sqlite3GenericMalloc
#define sqlite3OsRealloc            sqlite3GenericRealloc
#define sqlite3OsFree               sqlite3GenericFree
#define sqlite3OsAllocationSize     sqlite3GenericAllocationSize
#endif
#if OS_OS2
#define sqlite3OsOpenReadWrite      sqlite3Os2OpenReadWrite
#define sqlite3OsOpenExclusive      sqlite3Os2OpenExclusive
#define sqlite3OsOpenReadOnly       sqlite3Os2OpenReadOnly
#define sqlite3OsDelete             sqlite3Os2Delete
#define sqlite3OsFileExists         sqlite3Os2FileExists
#define sqlite3OsFullPathname       sqlite3Os2FullPathname
#define sqlite3OsIsDirWritable      sqlite3Os2IsDirWritable
#define sqlite3OsSyncDirectory      sqlite3Os2SyncDirectory
#define sqlite3OsTempFileName       sqlite3Os2TempFileName
#define sqlite3OsRandomSeed         sqlite3Os2RandomSeed
#define sqlite3OsSleep              sqlite3Os2Sleep
#define sqlite3OsCurrentTime        sqlite3Os2CurrentTime
#define sqlite3OsEnterMutex         sqlite3Os2EnterMutex
#define sqlite3OsLeaveMutex         sqlite3Os2LeaveMutex
#define sqlite3OsInMutex            sqlite3Os2InMutex
#define sqlite3OsThreadSpecificData sqlite3Os2ThreadSpecificData
#define sqlite3OsMalloc             sqlite3GenericMalloc
#define sqlite3OsRealloc            sqlite3GenericRealloc
#define sqlite3OsFree               sqlite3GenericFree
#define sqlite3OsAllocationSize     sqlite3GenericAllocationSize
#endif
#if OS_BEOS
#define sqlite3OsOpenReadWrite      sqlite3BeOpenReadWrite
#define sqlite3OsOpenExclusive      sqlite3BeOpenExclusive
#define sqlite3OsOpenReadOnly       sqlite3BeOpenReadOnly
#define sqlite3OsDelete             sqlite3BeDelete
#define sqlite3OsFileExists         sqlite3BeFileExists
#define sqlite3OsFullPathname       sqlite3BeFullPathname
#define sqlite3OsIsDirWritable      sqlite3BeIsDirWritable
#define sqlite3OsSyncDirectory      sqlite3BeSyncDirectory
#define sqlite3OsTempFileName       sqlite3BeTempFileName
#define sqlite3OsRandomSeed         sqlite3BeRandomSeed
#define sqlite3OsSleep              sqlite3BeSleep
#define sqlite3OsCurrentTime        sqlite3BeCurrentTime
#define sqlite3OsEnterMutex         sqlite3BeEnterMutex
#define sqlite3OsLeaveMutex         sqlite3BeLeaveMutex
#define sqlite3OsInMutex            sqlite3BeInMutex
#define sqlite3OsThreadSpecificData sqlite3BeThreadSpecificData
#define sqlite3OsMalloc             sqlite3GenericMalloc
#define sqlite3OsRealloc            sqlite3GenericRealloc
#define sqlite3OsFree               sqlite3GenericFree
#define sqlite3OsAllocationSize     sqlite3GenericAllocationSize
#endif






#if OS_OTHER
# include "os_other.h"
#endif






typedef struct OsFile OsFile;
typedef struct IoMethod IoMethod;





struct IoMethod {
  int (*xClose)(OsFile**);
  int (*xOpenDirectory)(OsFile*, const char*);
  int (*xRead)(OsFile*, void*, int amt);
  int (*xWrite)(OsFile*, const void*, int amt);
  int (*xSeek)(OsFile*, i64 offset);
  int (*xTruncate)(OsFile*, i64 size);
  int (*xSync)(OsFile*, int);
  void (*xSetFullSync)(OsFile *id, int setting);
  int (*xFileHandle)(OsFile *id);
  int (*xFileSize)(OsFile*, i64 *pSize);
  int (*xLock)(OsFile*, int);
  int (*xUnlock)(OsFile*, int);
  int (*xLockState)(OsFile *id);
  int (*xCheckReservedLock)(OsFile *id);
};









struct OsFile {
  IoMethod const *pMethod;
};


















#define NO_LOCK         0
#define SHARED_LOCK     1
#define RESERVED_LOCK   2
#define PENDING_LOCK    3
#define EXCLUSIVE_LOCK  4


























































#ifndef SQLITE_TEST
#define PENDING_BYTE      0x40000000  /* First byte past the 1GB boundary */
#else
extern unsigned int sqlite3_pending_byte;
#define PENDING_BYTE sqlite3_pending_byte
#endif

#define RESERVED_BYTE     (PENDING_BYTE+1)
#define SHARED_FIRST      (PENDING_BYTE+2)
#define SHARED_SIZE       510




int sqlite3OsClose(OsFile**);
int sqlite3OsOpenDirectory(OsFile*, const char*);
int sqlite3OsRead(OsFile*, void*, int amt);
int sqlite3OsWrite(OsFile*, const void*, int amt);
int sqlite3OsSeek(OsFile*, i64 offset);
int sqlite3OsTruncate(OsFile*, i64 size);
int sqlite3OsSync(OsFile*, int);
void sqlite3OsSetFullSync(OsFile *id, int setting);
int sqlite3OsFileHandle(OsFile *id);
int sqlite3OsFileSize(OsFile*, i64 *pSize);
int sqlite3OsLock(OsFile*, int);
int sqlite3OsUnlock(OsFile*, int);
int sqlite3OsLockState(OsFile *id);
int sqlite3OsCheckReservedLock(OsFile *id);
int sqlite3OsOpenReadWrite(const char*, OsFile**, int*);
int sqlite3OsOpenExclusive(const char*, OsFile**, int);
int sqlite3OsOpenReadOnly(const char*, OsFile**);
int sqlite3OsDelete(const char*);
int sqlite3OsFileExists(const char*);
char *sqlite3OsFullPathname(const char*);
int sqlite3OsIsDirWritable(char*);
int sqlite3OsSyncDirectory(const char*);
int sqlite3OsTempFileName(char*);
int sqlite3OsRandomSeed(char*);
int sqlite3OsSleep(int ms);
int sqlite3OsCurrentTime(double*);
void sqlite3OsEnterMutex(void);
void sqlite3OsLeaveMutex(void);
int sqlite3OsInMutex(int);
ThreadData *sqlite3OsThreadSpecificData(int);
void *sqlite3OsMalloc(int);
void *sqlite3OsRealloc(void *, int);
void sqlite3OsFree(void *);
int sqlite3OsAllocationSize(void *);










#ifdef SQLITE_ENABLE_REDEF_IO








struct sqlite3OsVtbl {
  int (*xOpenReadWrite)(const char*, OsFile**, int*);
  int (*xOpenExclusive)(const char*, OsFile**, int);
  int (*xOpenReadOnly)(const char*, OsFile**);

  int (*xDelete)(const char*);
  int (*xFileExists)(const char*);
  char *(*xFullPathname)(const char*);
  int (*xIsDirWritable)(char*);
  int (*xSyncDirectory)(const char*);
  int (*xTempFileName)(char*);

  int (*xRandomSeed)(char*);
  int (*xSleep)(int ms);
  int (*xCurrentTime)(double*);

  void (*xEnterMutex)(void);
  void (*xLeaveMutex)(void);
  int (*xInMutex)(int);
  ThreadData *(*xThreadSpecificData)(int);

  void *(*xMalloc)(int);
  void *(*xRealloc)(void *, int);
  void (*xFree)(void *);
  int (*xAllocationSize)(void *);
};




#ifdef SQLITE_OMIT_DISKIO
# define IF_DISKIO(X)  0
#else
# define IF_DISKIO(X)  X
#endif

#ifdef _SQLITE_OS_C_
  


  struct sqlite3OsVtbl sqlite3Os = {
    IF_DISKIO( sqlite3OsOpenReadWrite ),
    IF_DISKIO( sqlite3OsOpenExclusive ),
    IF_DISKIO( sqlite3OsOpenReadOnly ),
    IF_DISKIO( sqlite3OsDelete ),
    IF_DISKIO( sqlite3OsFileExists ),
    IF_DISKIO( sqlite3OsFullPathname ),
    IF_DISKIO( sqlite3OsIsDirWritable ),
    IF_DISKIO( sqlite3OsSyncDirectory ),
    IF_DISKIO( sqlite3OsTempFileName ),
    sqlite3OsRandomSeed,
    sqlite3OsSleep,
    sqlite3OsCurrentTime,
    sqlite3OsEnterMutex,
    sqlite3OsLeaveMutex,
    sqlite3OsInMutex,
    sqlite3OsThreadSpecificData,
    sqlite3OsMalloc,
    sqlite3OsRealloc,
    sqlite3OsFree,
    sqlite3OsAllocationSize
  };
#else
  


  extern struct sqlite3OsVtbl sqlite3Os;
#endif 



struct sqlite3OsVtbl *sqlite3_os_switch(void);






#undef sqlite3OsOpenReadWrite
#undef sqlite3OsOpenExclusive
#undef sqlite3OsOpenReadOnly
#undef sqlite3OsDelete
#undef sqlite3OsFileExists
#undef sqlite3OsFullPathname
#undef sqlite3OsIsDirWritable
#undef sqlite3OsSyncDirectory
#undef sqlite3OsTempFileName
#undef sqlite3OsRandomSeed
#undef sqlite3OsSleep
#undef sqlite3OsCurrentTime
#undef sqlite3OsEnterMutex
#undef sqlite3OsLeaveMutex
#undef sqlite3OsInMutex
#undef sqlite3OsThreadSpecificData
#undef sqlite3OsMalloc
#undef sqlite3OsRealloc
#undef sqlite3OsFree
#undef sqlite3OsAllocationSize
#define sqlite3OsOpenReadWrite      sqlite3Os.xOpenReadWrite
#define sqlite3OsOpenExclusive      sqlite3Os.xOpenExclusive
#define sqlite3OsOpenReadOnly       sqlite3Os.xOpenReadOnly
#define sqlite3OsDelete             sqlite3Os.xDelete
#define sqlite3OsFileExists         sqlite3Os.xFileExists
#define sqlite3OsFullPathname       sqlite3Os.xFullPathname
#define sqlite3OsIsDirWritable      sqlite3Os.xIsDirWritable
#define sqlite3OsSyncDirectory      sqlite3Os.xSyncDirectory
#define sqlite3OsTempFileName       sqlite3Os.xTempFileName
#define sqlite3OsRandomSeed         sqlite3Os.xRandomSeed
#define sqlite3OsSleep              sqlite3Os.xSleep
#define sqlite3OsCurrentTime        sqlite3Os.xCurrentTime
#define sqlite3OsEnterMutex         sqlite3Os.xEnterMutex
#define sqlite3OsLeaveMutex         sqlite3Os.xLeaveMutex
#define sqlite3OsInMutex            sqlite3Os.xInMutex
#define sqlite3OsThreadSpecificData sqlite3Os.xThreadSpecificData
#define sqlite3OsMalloc             sqlite3Os.xMalloc
#define sqlite3OsRealloc            sqlite3Os.xRealloc
#define sqlite3OsFree               sqlite3Os.xFree
#define sqlite3OsAllocationSize     sqlite3Os.xAllocationSize

#endif 

#endif 
