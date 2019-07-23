













#include "sqliteInt.h"
#include "os.h"
#if OS_WIN               

#include <winbase.h>

#ifdef __CYGWIN__
# include <sys/cygwin.h>
#endif




#if defined(THREADSAFE) && THREADSAFE
# define SQLITE_W32_THREADS 1
#endif




#include "os_common.h"





#if defined(_WIN32_WCE)
# define OS_WINCE 1
#else
# define OS_WINCE 0
#endif





#if OS_WINCE
typedef struct winceLock {
  int nReaders;       
  BOOL bPending;      
  BOOL bReserved;     
  BOOL bExclusive;    
} winceLock;
#endif





typedef struct winFile winFile;
struct winFile {
  IoMethod const *pMethod;
  HANDLE h;               
  unsigned char locktype; 
  short sharedLockByte;   
#if OS_WINCE
  WCHAR *zDeleteOnClose;  
  HANDLE hMutex;            
  HANDLE hShared;         
  winceLock local;        
  winceLock *shared;      
#endif
};







#ifndef SQLITE_OMIT_DISKIO













int sqlite3_os_type = 0;












#if OS_WINCE
# define isNT()  (1)
#else
  static int isNT(void){
    if( sqlite3_os_type==0 ){
      OSVERSIONINFO sInfo;
      sInfo.dwOSVersionInfoSize = sizeof(sInfo);
      GetVersionEx(&sInfo);
      sqlite3_os_type = sInfo.dwPlatformId==VER_PLATFORM_WIN32_NT ? 2 : 1;
    }
    return sqlite3_os_type==2;
  }
#endif 





static WCHAR *utf8ToUnicode(const char *zFilename){
  int nChar;
  WCHAR *zWideFilename;

  nChar = MultiByteToWideChar(CP_UTF8, 0, zFilename, -1, NULL, 0);
  zWideFilename = sqliteMalloc( nChar*sizeof(zWideFilename[0]) );
  if( zWideFilename==0 ){
    return 0;
  }
  nChar = MultiByteToWideChar(CP_UTF8, 0, zFilename, -1, zWideFilename, nChar);
  if( nChar==0 ){
    sqliteFree(zWideFilename);
    zWideFilename = 0;
  }
  return zWideFilename;
}





static char *unicodeToUtf8(const WCHAR *zWideFilename){
  int nByte;
  char *zFilename;

  nByte = WideCharToMultiByte(CP_UTF8, 0, zWideFilename, -1, 0, 0, 0, 0);
  zFilename = sqliteMalloc( nByte );
  if( zFilename==0 ){
    return 0;
  }
  nByte = WideCharToMultiByte(CP_UTF8, 0, zWideFilename, -1, zFilename, nByte,
                              0, 0);
  if( nByte == 0 ){
    sqliteFree(zFilename);
    zFilename = 0;
  }
  return zFilename;
}





static WCHAR *mbcsToUnicode(const char *zFilename){
  int nByte;
  WCHAR *zMbcsFilename;

  nByte = MultiByteToWideChar(CP_ACP, 0, zFilename, -1, NULL, 0)*sizeof(WCHAR);
  zMbcsFilename = sqliteMalloc( nByte*sizeof(zMbcsFilename[0]) );
  if( zMbcsFilename==0 ){
    return 0;
  }
  nByte = MultiByteToWideChar(CP_ACP, 0, zFilename, -1, zMbcsFilename, nByte);
  if( nByte==0 ){
    sqliteFree(zMbcsFilename);
    zMbcsFilename = 0;
  }
  return zMbcsFilename;
}





static char *unicodeToMbcs(const WCHAR *zWideFilename){
  int nByte;
  char *zFilename;

  nByte = WideCharToMultiByte(CP_ACP, 0, zWideFilename, -1, 0, 0, 0, 0);
  zFilename = sqliteMalloc( nByte );
  if( zFilename==0 ){
    return 0;
  }
  nByte = WideCharToMultiByte(CP_ACP, 0, zWideFilename, -1, zFilename, nByte,
                              0, 0);
  if( nByte == 0 ){
    sqliteFree(zFilename);
    zFilename = 0;
  }
  return zFilename;
}





static char *mbcsToUtf8(const char *zFilename){
  char *zFilenameUtf8;
  WCHAR *zTmpWide;

  zTmpWide = mbcsToUnicode(zFilename);
  if( zTmpWide==0 ){
    return 0;
  }
  zFilenameUtf8 = unicodeToUtf8(zTmpWide);
  sqliteFree(zTmpWide);
  return zFilenameUtf8;
}





static char *utf8ToMbcs(const char *zFilename){
  char *zFilenameMbcs;
  WCHAR *zTmpWide;

  zTmpWide = utf8ToUnicode(zFilename);
  if( zTmpWide==0 ){
    return 0;
  }
  zFilenameMbcs = unicodeToMbcs(zTmpWide);
  sqliteFree(zTmpWide);
  return zFilenameMbcs;
}

#if OS_WINCE







#include <time.h>
struct tm *__cdecl localtime(const time_t *t)
{
  static struct tm y;
  FILETIME uTm, lTm;
  SYSTEMTIME pTm;
  i64 t64;
  t64 = *t;
  t64 = (t64 + 11644473600)*10000000;
  uTm.dwLowDateTime = t64 & 0xFFFFFFFF;
  uTm.dwHighDateTime= t64 >> 32;
  FileTimeToLocalFileTime(&uTm,&lTm);
  FileTimeToSystemTime(&lTm,&pTm);
  y.tm_year = pTm.wYear - 1900;
  y.tm_mon = pTm.wMonth - 1;
  y.tm_wday = pTm.wDayOfWeek;
  y.tm_mday = pTm.wDay;
  y.tm_hour = pTm.wHour;
  y.tm_min = pTm.wMinute;
  y.tm_sec = pTm.wSecond;
  return &y;
}


#define GetTempPathA(a,b)

#define LockFile(a,b,c,d,e)       winceLockFile(&a, b, c, d, e)
#define UnlockFile(a,b,c,d,e)     winceUnlockFile(&a, b, c, d, e)
#define LockFileEx(a,b,c,d,e,f)   winceLockFileEx(&a, b, c, d, e, f)

#define HANDLE_TO_WINFILE(a) (winFile*)&((char*)a)[-offsetof(winFile,h)]




static void winceMutexAcquire(HANDLE h){
   DWORD dwErr;
   do {
     dwErr = WaitForSingleObject(h, INFINITE);
   } while (dwErr != WAIT_OBJECT_0 && dwErr != WAIT_ABANDONED);
}



#define winceMutexRelease(h) ReleaseMutex(h)





static BOOL winceCreateLock(const char *zFilename, winFile *pFile){
  WCHAR *zTok;
  WCHAR *zName = utf8ToUnicode(zFilename);
  BOOL bInit = TRUE;

  
  ZeroMemory(&pFile->local, sizeof(pFile->local));

  

  zTok = CharLowerW(zName);
  for (;*zTok;zTok++){
    if (*zTok == '\\') *zTok = '_';
  }

  
  pFile->hMutex = CreateMutexW(NULL, FALSE, zName);
  if (!pFile->hMutex){
    sqliteFree(zName);
    return FALSE;
  }

  
  winceMutexAcquire(pFile->hMutex);
  
  



  CharUpperW(zName);
  pFile->hShared = CreateFileMappingW(INVALID_HANDLE_VALUE, NULL,
                                       PAGE_READWRITE, 0, sizeof(winceLock),
                                       zName);  

  

  if (GetLastError() == ERROR_ALREADY_EXISTS){
    bInit = FALSE;
  }

  sqliteFree(zName);

  
  if (pFile->hShared){
    pFile->shared = (winceLock*)MapViewOfFile(pFile->hShared, 
             FILE_MAP_READ|FILE_MAP_WRITE, 0, 0, sizeof(winceLock));
    
    if (!pFile->shared){
      CloseHandle(pFile->hShared);
      pFile->hShared = NULL;
    }
  }

  
  if (pFile->hShared == NULL){
    winceMutexRelease(pFile->hMutex);
    CloseHandle(pFile->hMutex);
    pFile->hMutex = NULL;
    return FALSE;
  }
  
  
  if (bInit) {
    ZeroMemory(pFile->shared, sizeof(winceLock));
  }

  winceMutexRelease(pFile->hMutex);
  return TRUE;
}




static void winceDestroyLock(winFile *pFile){
  if (pFile->hMutex){
    
    winceMutexAcquire(pFile->hMutex);

    

    if (pFile->local.nReaders){
      pFile->shared->nReaders --;
    }
    if (pFile->local.bReserved){
      pFile->shared->bReserved = FALSE;
    }
    if (pFile->local.bPending){
      pFile->shared->bPending = FALSE;
    }
    if (pFile->local.bExclusive){
      pFile->shared->bExclusive = FALSE;
    }

    
    UnmapViewOfFile(pFile->shared);
    CloseHandle(pFile->hShared);

    
    winceMutexRelease(pFile->hMutex);    
    CloseHandle(pFile->hMutex);
    pFile->hMutex = NULL;
  }
}




static BOOL winceLockFile(
  HANDLE *phFile,
  DWORD dwFileOffsetLow,
  DWORD dwFileOffsetHigh,
  DWORD nNumberOfBytesToLockLow,
  DWORD nNumberOfBytesToLockHigh
){
  winFile *pFile = HANDLE_TO_WINFILE(phFile);
  BOOL bReturn = FALSE;

  if (!pFile->hMutex) return TRUE;
  winceMutexAcquire(pFile->hMutex);

  
  if (dwFileOffsetLow == SHARED_FIRST
       && nNumberOfBytesToLockLow == SHARED_SIZE){
    if (pFile->shared->nReaders == 0 && pFile->shared->bExclusive == 0){
       pFile->shared->bExclusive = TRUE;
       pFile->local.bExclusive = TRUE;
       bReturn = TRUE;
    }
  }

  
  else if ((dwFileOffsetLow >= SHARED_FIRST &&
            dwFileOffsetLow < SHARED_FIRST + SHARED_SIZE) &&
            nNumberOfBytesToLockLow == 1){
    if (pFile->shared->bExclusive == 0){
      pFile->local.nReaders ++;
      if (pFile->local.nReaders == 1){
        pFile->shared->nReaders ++;
      }
      bReturn = TRUE;
    }
  }

  
  else if (dwFileOffsetLow == PENDING_BYTE && nNumberOfBytesToLockLow == 1){
    
    if (pFile->shared->bPending == 0) {
      pFile->shared->bPending = TRUE;
      pFile->local.bPending = TRUE;
      bReturn = TRUE;
    }
  }
  
  else if (dwFileOffsetLow == RESERVED_BYTE && nNumberOfBytesToLockLow == 1){
    if (pFile->shared->bReserved == 0) {
      pFile->shared->bReserved = TRUE;
      pFile->local.bReserved = TRUE;
      bReturn = TRUE;
    }
  }

  winceMutexRelease(pFile->hMutex);
  return bReturn;
}




static BOOL winceUnlockFile(
  HANDLE *phFile,
  DWORD dwFileOffsetLow,
  DWORD dwFileOffsetHigh,
  DWORD nNumberOfBytesToUnlockLow,
  DWORD nNumberOfBytesToUnlockHigh
){
  winFile *pFile = HANDLE_TO_WINFILE(phFile);
  BOOL bReturn = FALSE;

  if (!pFile->hMutex) return TRUE;
  winceMutexAcquire(pFile->hMutex);

  
  if (dwFileOffsetLow >= SHARED_FIRST &&
       dwFileOffsetLow < SHARED_FIRST + SHARED_SIZE){
    
    if (pFile->local.bExclusive){
      pFile->local.bExclusive = FALSE;
      pFile->shared->bExclusive = FALSE;
      bReturn = TRUE;
    }

    
    else if (pFile->local.nReaders){
      pFile->local.nReaders --;
      if (pFile->local.nReaders == 0)
      {
        pFile->shared->nReaders --;
      }
      bReturn = TRUE;
    }
  }

  
  else if (dwFileOffsetLow == PENDING_BYTE && nNumberOfBytesToUnlockLow == 1){
    if (pFile->local.bPending){
      pFile->local.bPending = FALSE;
      pFile->shared->bPending = FALSE;
      bReturn = TRUE;
    }
  }
  
  else if (dwFileOffsetLow == RESERVED_BYTE && nNumberOfBytesToUnlockLow == 1){
    if (pFile->local.bReserved) {
      pFile->local.bReserved = FALSE;
      pFile->shared->bReserved = FALSE;
      bReturn = TRUE;
    }
  }

  winceMutexRelease(pFile->hMutex);
  return bReturn;
}




static BOOL winceLockFileEx(
  HANDLE *phFile,
  DWORD dwFlags,
  DWORD dwReserved,
  DWORD nNumberOfBytesToLockLow,
  DWORD nNumberOfBytesToLockHigh,
  LPOVERLAPPED lpOverlapped
){
  

  if (lpOverlapped->Offset == SHARED_FIRST &&
      dwFlags == 1 &&
      nNumberOfBytesToLockLow == SHARED_SIZE){
    return winceLockFile(phFile, SHARED_FIRST, 0, 1, 0);
  }
  return FALSE;
}



#endif 

static void *convertUtf8Filename(const char *zFilename){
  void *zConverted = 0;
  if( isNT() ){
    zConverted = utf8ToUnicode(zFilename);
  }else{
    zConverted = utf8ToMbcs(zFilename);
  }
  
  return zConverted;
}




int sqlite3WinDelete(const char *zFilename){
  void *zConverted = convertUtf8Filename(zFilename);
  if( zConverted==0 )
    return SQLITE_NOMEM;
  if( isNT() ){
    DeleteFileW((WCHAR*)zConverted);
  }else{
    DeleteFileA((char*)zConverted);
  }
  sqliteFree(zConverted);
  TRACE2("DELETE \"%s\"\n", zFilename);
  return SQLITE_OK;
}




int sqlite3WinFileExists(const char *zFilename){
  int exists = 0;
  void *zConverted = convertUtf8Filename(zFilename);
  if( zConverted==0 )
    return SQLITE_NOMEM;
  if( isNT() ){
    exists = GetFileAttributesW((WCHAR*)zConverted) != 0xffffffff;
  }else{
    exists = GetFileAttributesA((char*)zConverted) != 0xffffffff;
  }
  sqliteFree(zConverted);
  return exists;
}


static int allocateWinFile(winFile *pInit, OsFile **pId);














int sqlite3WinOpenReadWrite(
  const char *zFilename,
  OsFile **pId,
  int *pReadonly
){
  winFile f;
  HANDLE h;
  void *zConverted = convertUtf8Filename(zFilename);
  if( zConverted==0 )
    return SQLITE_NOMEM;
  assert( *pId==0 );

  if( isNT() ){
    h = CreateFileW((WCHAR*)zConverted,
       GENERIC_READ | GENERIC_WRITE,
       FILE_SHARE_READ | FILE_SHARE_WRITE,
       NULL,
       OPEN_ALWAYS,
       FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS,
       NULL
    );
    if( h==INVALID_HANDLE_VALUE ){
      h = CreateFileW((WCHAR*)zConverted,
         GENERIC_READ,
         FILE_SHARE_READ,
         NULL,
         OPEN_ALWAYS,
         FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS,
         NULL
      );
      if( h==INVALID_HANDLE_VALUE ){
        sqliteFree(zConverted);
        return SQLITE_CANTOPEN;
      }
      *pReadonly = 1;
    }else{
      *pReadonly = 0;
    }
#if OS_WINCE
    if (!winceCreateLock(zFilename, &f)){
      CloseHandle(h);
      sqliteFree(zConverted);
      return SQLITE_CANTOPEN;
    }
#endif
  }else{
    h = CreateFileA((char*)zConverted,
       GENERIC_READ | GENERIC_WRITE,
       FILE_SHARE_READ | FILE_SHARE_WRITE,
       NULL,
       OPEN_ALWAYS,
       FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS,
       NULL
    );
    if( h==INVALID_HANDLE_VALUE ){
      h = CreateFileA((char*)zConverted,
         GENERIC_READ,
         FILE_SHARE_READ,
         NULL,
         OPEN_ALWAYS,
         FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS,
         NULL
      );
      if( h==INVALID_HANDLE_VALUE ){
        sqliteFree(zConverted);
        return SQLITE_CANTOPEN;
      }
      *pReadonly = 1;
    }else{
      *pReadonly = 0;
    }
  }

  sqliteFree(zConverted);

  f.h = h;
#if OS_WINCE
  f.zDeleteOnClose = 0;
#endif
  TRACE3("OPEN R/W %d \"%s\"\n", h, zFilename);
  return allocateWinFile(&f, pId);
}
















int sqlite3WinOpenExclusive(const char *zFilename, OsFile **pId, int delFlag){
  winFile f;
  HANDLE h;
  int fileflags;
  void *zConverted = convertUtf8Filename(zFilename);
  if( zConverted==0 )
    return SQLITE_NOMEM;
  assert( *pId == 0 );
  fileflags = FILE_FLAG_RANDOM_ACCESS;
#if !OS_WINCE
  if( delFlag ){
    fileflags |= FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE;
  }
#endif
  if( isNT() ){
    h = CreateFileW((WCHAR*)zConverted,
       GENERIC_READ | GENERIC_WRITE,
       0,
       NULL,
       CREATE_ALWAYS,
       fileflags,
       NULL
    );
  }else{
    h = CreateFileA((char*)zConverted,
       GENERIC_READ | GENERIC_WRITE,
       0,
       NULL,
       CREATE_ALWAYS,
       fileflags,
       NULL
    );
  }
  sqliteFree(zConverted);
  if( h==INVALID_HANDLE_VALUE ){
    return SQLITE_CANTOPEN;
  }
  f.h = h;
#if OS_WINCE
  f.zDeleteOnClose = delFlag ? utf8ToUnicode(zFilename) : 0;
  f.hMutex = NULL;
#endif
  TRACE3("OPEN EX %d \"%s\"\n", h, zFilename);
  return allocateWinFile(&f, pId);
}








int sqlite3WinOpenReadOnly(const char *zFilename, OsFile **pId){
  winFile f;
  HANDLE h;
  void *zConverted = convertUtf8Filename(zFilename);
  if( zConverted==0 )
    return SQLITE_NOMEM;
  assert( *pId==0 );
  if( isNT() ){
    h = CreateFileW((WCHAR*)zConverted,
       GENERIC_READ,
       0,
       NULL,
       OPEN_EXISTING,
       FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS,
       NULL
    );
  }else{
    h = CreateFileA((char*)zConverted,
       GENERIC_READ,
       0,
       NULL,
       OPEN_EXISTING,
       FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS,
       NULL
    );
  }
  sqliteFree(zConverted);
  if( h==INVALID_HANDLE_VALUE ){
    return SQLITE_CANTOPEN;
  }
  f.h = h;
#if OS_WINCE
  f.zDeleteOnClose = 0;
  f.hMutex = NULL;
#endif
  TRACE3("OPEN RO %d \"%s\"\n", h, zFilename);
  return allocateWinFile(&f, pId);
}

















static int winOpenDirectory(
  OsFile *id,
  const char *zDirname
){
  return SQLITE_OK;
}






char *sqlite3_temp_directory = 0;





int sqlite3WinTempFileName(char *zBuf){
  static char zChars[] =
    "abcdefghijklmnopqrstuvwxyz"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "0123456789";
  int i, j;
  char zTempPath[SQLITE_TEMPNAME_SIZE];
  if( sqlite3_temp_directory ){
    strncpy(zTempPath, sqlite3_temp_directory, SQLITE_TEMPNAME_SIZE-30);
    zTempPath[SQLITE_TEMPNAME_SIZE-30] = 0;
  }else if( isNT() ){
    char *zMulti;
    WCHAR zWidePath[SQLITE_TEMPNAME_SIZE];
    GetTempPathW(SQLITE_TEMPNAME_SIZE-30, zWidePath);
    zMulti = unicodeToUtf8(zWidePath);
    if( zMulti ){
      strncpy(zTempPath, zMulti, SQLITE_TEMPNAME_SIZE-30);
      zTempPath[SQLITE_TEMPNAME_SIZE-30] = 0;
      sqliteFree(zMulti);
    }else{
      return SQLITE_NOMEM;
    }
  }else{
    char *zUtf8;
    char zMbcsPath[SQLITE_TEMPNAME_SIZE];
    GetTempPathA(SQLITE_TEMPNAME_SIZE-30, zMbcsPath);
    zUtf8 = mbcsToUtf8(zMbcsPath);
    if( zUtf8 ){
      strncpy(zTempPath, zUtf8, SQLITE_TEMPNAME_SIZE-30);
      zTempPath[SQLITE_TEMPNAME_SIZE-30] = 0;
      sqliteFree(zUtf8);
    }else{
      return SQLITE_NOMEM;
    }
  }
  for(i=strlen(zTempPath); i>0 && zTempPath[i-1]=='\\'; i--){}
  zTempPath[i] = 0;
  for(;;){
    sprintf(zBuf, "%s\\"TEMP_FILE_PREFIX, zTempPath);
    j = strlen(zBuf);
    sqlite3Randomness(15, &zBuf[j]);
    for(i=0; i<15; i++, j++){
      zBuf[j] = (char)zChars[ ((unsigned char)zBuf[j])%(sizeof(zChars)-1) ];
    }
    zBuf[j] = 0;
    if( !sqlite3OsFileExists(zBuf) ) break;
  }
  TRACE2("TEMP FILENAME: %s\n", zBuf);
  return SQLITE_OK; 
}




static int winClose(OsFile **pId){
  winFile *pFile;
  if( pId && (pFile = (winFile*)*pId)!=0 ){
    TRACE2("CLOSE %d\n", pFile->h);
    CloseHandle(pFile->h);
#if OS_WINCE
    winceDestroyLock(pFile);
    if( pFile->zDeleteOnClose ){
      DeleteFileW(pFile->zDeleteOnClose);
      sqliteFree(pFile->zDeleteOnClose);
    }
#endif
    OpenCounter(-1);
    sqliteFree(pFile);
    *pId = 0;
  }
  return SQLITE_OK;
}






static int winRead(OsFile *id, void *pBuf, int amt){
  DWORD got;
  assert( id!=0 );
  SimulateIOError(SQLITE_IOERR);
  TRACE3("READ %d lock=%d\n", ((winFile*)id)->h, ((winFile*)id)->locktype);
  if( !ReadFile(((winFile*)id)->h, pBuf, amt, &got, 0) ){
    got = 0;
  }
  if( got==(DWORD)amt ){
    return SQLITE_OK;
  }else{
    return SQLITE_IOERR;
  }
}





static int winWrite(OsFile *id, const void *pBuf, int amt){
  int rc = 0;
  DWORD wrote;
  assert( id!=0 );
  SimulateIOError(SQLITE_IOERR);
  SimulateDiskfullError;
  TRACE3("WRITE %d lock=%d\n", ((winFile*)id)->h, ((winFile*)id)->locktype);
  assert( amt>0 );
  while( amt>0 && (rc = WriteFile(((winFile*)id)->h, pBuf, amt, &wrote, 0))!=0
         && wrote>0 ){
    amt -= wrote;
    pBuf = &((char*)pBuf)[wrote];
  }
  if( !rc || amt>(int)wrote ){
    return SQLITE_FULL;
  }
  return SQLITE_OK;
}




#ifndef INVALID_SET_FILE_POINTER
# define INVALID_SET_FILE_POINTER ((DWORD)-1)
#endif




static int winSeek(OsFile *id, i64 offset){
  LONG upperBits = offset>>32;
  LONG lowerBits = offset & 0xffffffff;
  DWORD rc;
  assert( id!=0 );
#ifdef SQLITE_TEST
  if( offset ) SimulateDiskfullError
#endif
  SEEK(offset/1024 + 1);
  rc = SetFilePointer(((winFile*)id)->h, lowerBits, &upperBits, FILE_BEGIN);
  TRACE3("SEEK %d %lld\n", ((winFile*)id)->h, offset);
  if( rc==INVALID_SET_FILE_POINTER && GetLastError()!=NO_ERROR ){
    return SQLITE_FULL;
  }
  return SQLITE_OK;
}




static int winSync(OsFile *id, int dataOnly){
  assert( id!=0 );
  TRACE3("SYNC %d lock=%d\n", ((winFile*)id)->h, ((winFile*)id)->locktype);
  if( FlushFileBuffers(((winFile*)id)->h) ){
    return SQLITE_OK;
  }else{
    return SQLITE_IOERR;
  }
}





int sqlite3WinSyncDirectory(const char *zDirname){
  SimulateIOError(SQLITE_IOERR);
  return SQLITE_OK;
}




static int winTruncate(OsFile *id, i64 nByte){
  LONG upperBits = nByte>>32;
  assert( id!=0 );
  TRACE3("TRUNCATE %d %lld\n", ((winFile*)id)->h, nByte);
  SimulateIOError(SQLITE_IOERR);
  SetFilePointer(((winFile*)id)->h, nByte, &upperBits, FILE_BEGIN);
  SetEndOfFile(((winFile*)id)->h);
  return SQLITE_OK;
}




static int winFileSize(OsFile *id, i64 *pSize){
  DWORD upperBits, lowerBits;
  assert( id!=0 );
  SimulateIOError(SQLITE_IOERR);
  lowerBits = GetFileSize(((winFile*)id)->h, &upperBits);
  *pSize = (((i64)upperBits)<<32) + lowerBits;
  return SQLITE_OK;
}




#ifndef LOCKFILE_FAIL_IMMEDIATELY
# define LOCKFILE_FAIL_IMMEDIATELY 1
#endif






static int getReadLock(winFile *id){
  int res;
  if( isNT() ){
    OVERLAPPED ovlp;
    ovlp.Offset = SHARED_FIRST;
    ovlp.OffsetHigh = 0;
    ovlp.hEvent = 0;
    res = LockFileEx(id->h, LOCKFILE_FAIL_IMMEDIATELY, 0, SHARED_SIZE,0,&ovlp);
  }else{
    int lk;
    sqlite3Randomness(sizeof(lk), &lk);
    id->sharedLockByte = (lk & 0x7fffffff)%(SHARED_SIZE - 1);
    res = LockFile(id->h, SHARED_FIRST+id->sharedLockByte, 0, 1, 0);
  }
  return res;
}




static int unlockReadLock(winFile *pFile){
  int res;
  if( isNT() ){
    res = UnlockFile(pFile->h, SHARED_FIRST, 0, SHARED_SIZE, 0);
  }else{
    res = UnlockFile(pFile->h, SHARED_FIRST + pFile->sharedLockByte, 0, 1, 0);
  }
  return res;
}

#ifndef SQLITE_OMIT_PAGER_PRAGMAS




int sqlite3WinIsDirWritable(char *zDirname){
  int fileAttr;
  void *zConverted;
  if( zDirname==0 ) return 0;
  if( !isNT() && strlen(zDirname)>MAX_PATH ) return 0;

  zConverted = convertUtf8Filename(zDirname);
  if( zConverted==0 )
    return SQLITE_NOMEM;
  if( isNT() ){
    fileAttr = GetFileAttributesW((WCHAR*)zConverted);
  }else{
    fileAttr = GetFileAttributesA((char*)zConverted);
  }
  sqliteFree(zConverted);
  if( fileAttr == 0xffffffff ) return 0;
  if( (fileAttr & FILE_ATTRIBUTE_DIRECTORY) != FILE_ATTRIBUTE_DIRECTORY ){
    return 0;
  }
  return 1;
}
#endif 



























static int winLock(OsFile *id, int locktype){
  int rc = SQLITE_OK;    
  int res = 1;           
  int newLocktype;       
  int gotPendingLock = 0;
  winFile *pFile = (winFile*)id;

  assert( pFile!=0 );
  TRACE5("LOCK %d %d was %d(%d)\n",
          pFile->h, locktype, pFile->locktype, pFile->sharedLockByte);

  



  if( pFile->locktype>=locktype ){
    return SQLITE_OK;
  }

  

  assert( pFile->locktype!=NO_LOCK || locktype==SHARED_LOCK );
  assert( locktype!=PENDING_LOCK );
  assert( locktype!=RESERVED_LOCK || pFile->locktype==SHARED_LOCK );

  



  newLocktype = pFile->locktype;
  if( pFile->locktype==NO_LOCK
   || (locktype==EXCLUSIVE_LOCK && pFile->locktype==RESERVED_LOCK)
  ){
    int cnt = 3;
    while( cnt-->0 && (res = LockFile(pFile->h, PENDING_BYTE, 0, 1, 0))==0 ){
      


      TRACE2("could not get a PENDING lock. cnt=%d\n", cnt);
      Sleep(1);
    }
    gotPendingLock = res;
  }

  

  if( locktype==SHARED_LOCK && res ){
    assert( pFile->locktype==NO_LOCK );
    res = getReadLock(pFile);
    if( res ){
      newLocktype = SHARED_LOCK;
    }
  }

  

  if( locktype==RESERVED_LOCK && res ){
    assert( pFile->locktype==SHARED_LOCK );
    res = LockFile(pFile->h, RESERVED_BYTE, 0, 1, 0);
    if( res ){
      newLocktype = RESERVED_LOCK;
    }
  }

  

  if( locktype==EXCLUSIVE_LOCK && res ){
    newLocktype = PENDING_LOCK;
    gotPendingLock = 0;
  }

  

  if( locktype==EXCLUSIVE_LOCK && res ){
    assert( pFile->locktype>=SHARED_LOCK );
    res = unlockReadLock(pFile);
    TRACE2("unreadlock = %d\n", res);
    res = LockFile(pFile->h, SHARED_FIRST, 0, SHARED_SIZE, 0);
    if( res ){
      newLocktype = EXCLUSIVE_LOCK;
    }else{
      TRACE2("error-code = %d\n", GetLastError());
    }
  }

  


  if( gotPendingLock && locktype==SHARED_LOCK ){
    UnlockFile(pFile->h, PENDING_BYTE, 0, 1, 0);
  }

  


  if( res ){
    rc = SQLITE_OK;
  }else{
    TRACE4("LOCK FAILED %d trying for %d but got %d\n", pFile->h,
           locktype, newLocktype);
    rc = SQLITE_BUSY;
  }
  pFile->locktype = newLocktype;
  return rc;
}






static int winCheckReservedLock(OsFile *id){
  int rc;
  winFile *pFile = (winFile*)id;
  assert( pFile!=0 );
  if( pFile->locktype>=RESERVED_LOCK ){
    rc = 1;
    TRACE3("TEST WR-LOCK %d %d (local)\n", pFile->h, rc);
  }else{
    rc = LockFile(pFile->h, RESERVED_BYTE, 0, 1, 0);
    if( rc ){
      UnlockFile(pFile->h, RESERVED_BYTE, 0, 1, 0);
    }
    rc = !rc;
    TRACE3("TEST WR-LOCK %d %d (remote)\n", pFile->h, rc);
  }
  return rc;
}












static int winUnlock(OsFile *id, int locktype){
  int type;
  int rc = SQLITE_OK;
  winFile *pFile = (winFile*)id;
  assert( pFile!=0 );
  assert( locktype<=SHARED_LOCK );
  TRACE5("UNLOCK %d to %d was %d(%d)\n", pFile->h, locktype,
          pFile->locktype, pFile->sharedLockByte);
  type = pFile->locktype;
  if( type>=EXCLUSIVE_LOCK ){
    UnlockFile(pFile->h, SHARED_FIRST, 0, SHARED_SIZE, 0);
    if( locktype==SHARED_LOCK && !getReadLock(pFile) ){
      

      rc = SQLITE_IOERR;
    }
  }
  if( type>=RESERVED_LOCK ){
    UnlockFile(pFile->h, RESERVED_BYTE, 0, 1, 0);
  }
  if( locktype==NO_LOCK && type>=SHARED_LOCK ){
    unlockReadLock(pFile);
  }
  if( type>=PENDING_LOCK ){
    UnlockFile(pFile->h, PENDING_BYTE, 0, 1, 0);
  }
  pFile->locktype = locktype;
  return rc;
}







char *sqlite3WinFullPathname(const char *zRelative){
  char *zFull;
#if defined(__CYGWIN__)
  int nByte;
  nByte = strlen(zRelative) + MAX_PATH + 1001;
  zFull = sqliteMalloc( nByte );
  if( zFull==0 ) return 0;
  if( cygwin_conv_to_full_win32_path(zRelative, zFull) ) return 0;
#elif OS_WINCE
  
  zFull = sqliteStrDup(zRelative);
#else
  int nByte;
  void *zConverted;
  zConverted = convertUtf8Filename(zRelative);
  if( isNT() ){
    WCHAR *zTemp, *zNotUsedW;
    nByte = GetFullPathNameW((WCHAR*)zConverted, 0, 0, &zNotUsedW) + 1;
    zTemp = sqliteMalloc( nByte*sizeof(zTemp[0]) );
    if( zTemp==0 ){
      sqliteFree(zConverted);
      return 0;
    }
    GetFullPathNameW((WCHAR*)zConverted, nByte, zTemp, &zNotUsedW);
    sqliteFree(zConverted);
    zFull = unicodeToUtf8(zTemp);
    sqliteFree(zTemp);
  }else{
    char *zTemp, *zNotUsed;
    nByte = GetFullPathNameA((char*)zConverted, 0, 0, &zNotUsed) + 1;
    zTemp = sqliteMalloc( nByte*sizeof(zTemp[0]) );
    if( zTemp==0 ){
      sqliteFree(zConverted);
      return 0;
    }
    GetFullPathNameA((char*)zConverted, nByte, zTemp, &zNotUsed);
    sqliteFree(zConverted);
    zFull = mbcsToUtf8(zTemp);
    sqliteFree(zTemp);
  }
#endif
  return zFull;
}




static void winSetFullSync(OsFile *id, int v){
  return;
}




static int winFileHandle(OsFile *id){
  return (int)((winFile*)id)->h;
}





static int winLockState(OsFile *id){
  return ((winFile*)id)->locktype;
}





static const IoMethod sqlite3WinIoMethod = {
  winClose,
  winOpenDirectory,
  winRead,
  winWrite,
  winSeek,
  winTruncate,
  winSync,
  winSetFullSync,
  winFileHandle,
  winFileSize,
  winLock,
  winUnlock,
  winLockState,
  winCheckReservedLock,
};






static int allocateWinFile(winFile *pInit, OsFile **pId){
  winFile *pNew;
  pNew = sqliteMalloc( sizeof(*pNew) );
  if( pNew==0 ){
    CloseHandle(pInit->h);
#if OS_WINCE
    sqliteFree(pInit->zDeleteOnClose);
#endif
    *pId = 0;
    return SQLITE_NOMEM;
  }else{
    *pNew = *pInit;
    pNew->pMethod = &sqlite3WinIoMethod;
    pNew->locktype = NO_LOCK;
    pNew->sharedLockByte = 0;
    *pId = (OsFile*)pNew;
    OpenCounter(+1);
    return SQLITE_OK;
  }
}


#endif 










int sqlite3WinRandomSeed(char *zBuf){
  











  memset(zBuf, 0, 256);
  GetSystemTime((LPSYSTEMTIME)zBuf);
  return SQLITE_OK;
}




int sqlite3WinSleep(int ms){
  Sleep(ms);
  return ms;
}




static int inMutex = 0;
#ifdef SQLITE_W32_THREADS
  static DWORD mutexOwner;
  static CRITICAL_SECTION cs;
#endif












void sqlite3WinEnterMutex(){
#ifdef SQLITE_W32_THREADS
  static int isInit = 0;
  while( !isInit ){
    static long lock = 0;
    if( InterlockedIncrement(&lock)==1 ){
      InitializeCriticalSection(&cs);
      isInit = 1;
    }else{
      Sleep(1);
    }
  }
  EnterCriticalSection(&cs);
  mutexOwner = GetCurrentThreadId();
#endif
  inMutex++;
}
void sqlite3WinLeaveMutex(){
  assert( inMutex );
  inMutex--;
#ifdef SQLITE_W32_THREADS
  assert( mutexOwner==GetCurrentThreadId() );
  LeaveCriticalSection(&cs);
#endif
}








int sqlite3WinInMutex(int thisThreadOnly){
#ifdef SQLITE_W32_THREADS
  return inMutex>0 && (thisThreadOnly==0 || mutexOwner==GetCurrentThreadId());
#else
  return inMutex>0;
#endif
}






#ifdef SQLITE_TEST
int sqlite3_current_time = 0;
#endif






int sqlite3WinCurrentTime(double *prNow){
  FILETIME ft;
  


  double now;
#if OS_WINCE
  SYSTEMTIME time;
  GetSystemTime(&time);
  SystemTimeToFileTime(&time,&ft);
#else
  GetSystemTimeAsFileTime( &ft );
#endif
  now = ((double)ft.dwHighDateTime) * 4294967296.0; 
  *prNow = (now + ft.dwLowDateTime)/864000000000.0 + 2305813.5;
#ifdef SQLITE_TEST
  if( sqlite3_current_time ){
    *prNow = sqlite3_current_time/86400.0 + 2440587.5;
  }
#endif
  return 0;
}






#ifdef SQLITE_TEST
int sqlite3_tsd_count = 0;
# define TSD_COUNTER_INCR InterlockedIncrement(&sqlite3_tsd_count)
# define TSD_COUNTER_DECR InterlockedDecrement(&sqlite3_tsd_count)
#else
# define TSD_COUNTER_INCR
# define TSD_COUNTER_DECR
#endif

















ThreadData *sqlite3WinThreadSpecificData(int allocateFlag){
  static int key;
  static int keyInit = 0;
  static const ThreadData zeroData = {0};
  ThreadData *pTsd;

  if( !keyInit ){
    sqlite3OsEnterMutex();
    if( !keyInit ){
      key = TlsAlloc();
      if( key==0xffffffff ){
        sqlite3OsLeaveMutex();
        return 0;
      }
      keyInit = 1;
    }
    sqlite3OsLeaveMutex();
  }
  pTsd = TlsGetValue(key);
  if( allocateFlag>0 ){
    if( !pTsd ){
      pTsd = sqlite3OsMalloc( sizeof(zeroData) );
      if( pTsd ){
        *pTsd = zeroData;
        TlsSetValue(key, pTsd);
        TSD_COUNTER_INCR;
      }
    }
  }else if( pTsd!=0 && allocateFlag<0 
              && memcmp(pTsd, &zeroData, sizeof(ThreadData))==0 ){
    sqlite3OsFree(pTsd);
    TlsSetValue(key, 0);
    TSD_COUNTER_DECR;
    pTsd = 0;
  }
  return pTsd;
}
#endif 
