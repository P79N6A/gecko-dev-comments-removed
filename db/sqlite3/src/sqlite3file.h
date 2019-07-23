
















































extern "C" {

struct ThreadData;







#if defined(_MSC_VER) || defined(__BORLANDC__)
  typedef __int64 sqlite_int64;
  typedef unsigned __int64 sqlite_uint64;
#else
  typedef long long int sqlite_int64;
  typedef unsigned long long int sqlite_uint64;
#endif






struct sqlite3;
int sqlite3ApiExit(sqlite3 *db, int);





typedef struct OsFile OsFile;
typedef struct IoMethod IoMethod;






struct IoMethod {
  int (*xClose)(OsFile**);
  int (*xOpenDirectory)(OsFile*, const char*);
  int (*xRead)(OsFile*, void*, int amt);
  int (*xWrite)(OsFile*, const void*, int amt);
  int (*xSeek)(OsFile*, sqlite_int64 offset);
  int (*xTruncate)(OsFile*, sqlite_int64 size);
  int (*xSync)(OsFile*, int);
  void (*xSetFullSync)(OsFile *id, int setting);
  int (*xFileHandle)(OsFile *id);
  int (*xFileSize)(OsFile*, sqlite_int64 *pSize);
  int (*xLock)(OsFile*, int);
  int (*xUnlock)(OsFile*, int);
  int (*xLockState)(OsFile *id);
  int (*xCheckReservedLock)(OsFile *id);
};










struct OsFile {
  IoMethod const *pMethod;
};









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

} 
