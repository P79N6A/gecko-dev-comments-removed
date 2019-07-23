













#include "sqliteInt.h"
#include "os.h"
#if OS_UNIX              














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
#include <time.h>
#include <sys/time.h>
#include <errno.h>





#if defined(THREADSAFE) && THREADSAFE
# include <pthread.h>
# define SQLITE_UNIX_THREADS 1
#endif




#ifndef SQLITE_DEFAULT_FILE_PERMISSIONS
# define SQLITE_DEFAULT_FILE_PERMISSIONS 0644
#endif







typedef struct unixFile unixFile;
struct unixFile {
  IoMethod const *pMethod;  
  struct openCnt *pOpen;    
  struct lockInfo *pLock;   
  int h;                    
  unsigned char locktype;   
  unsigned char isOpen;     
  unsigned char fullSync;   
  int dirfd;                
  i64 offset;               
#ifdef SQLITE_UNIX_THREADS
  pthread_t tid;            
#endif
};






#ifdef SQLITE_CRASH_TEST
  extern int sqlite3CrashTestEnable;
  extern int sqlite3CrashOpenReadWrite(const char*, OsFile**, int*);
  extern int sqlite3CrashOpenExclusive(const char*, OsFile**, int);
  extern int sqlite3CrashOpenReadOnly(const char*, OsFile**, int);
# define CRASH_TEST_OVERRIDE(X,A,B,C) \
    if(sqlite3CrashTestEnable){ return X(A,B,C); }
#else
# define CRASH_TEST_OVERRIDE(X,A,B,C)
#endif





#include "os_common.h"






#ifndef SQLITE_OMIT_DISKIO





#ifndef O_LARGEFILE
# define O_LARGEFILE 0
#endif
#ifdef SQLITE_DISABLE_LFS
# undef O_LARGEFILE
# define O_LARGEFILE 0
#endif
#ifndef O_NOFOLLOW
# define O_NOFOLLOW 0
#endif
#ifndef O_BINARY
# define O_BINARY 0
#endif







#ifdef __DJGPP__
# define fcntl(A,B,C) 0
#endif





#ifdef SQLITE_UNIX_THREADS
#define threadid pthread_self()
#else
#define threadid 0
#endif

















#if defined(SQLITE_UNIX_THREADS)
# define SET_THREADID(X)   (X)->tid = pthread_self()
# define CHECK_THREADID(X) (threadsOverrideEachOthersLocks==0 && \
                            !pthread_equal((X)->tid, pthread_self()))
#else
# define SET_THREADID(X)
# define CHECK_THREADID(X) 0
#endif














































































































struct lockKey {
  dev_t dev;       
  ino_t ino;       
#ifdef SQLITE_UNIX_THREADS
  pthread_t tid;   
#endif
};










struct lockInfo {
  struct lockKey key;  
  int cnt;             
  int locktype;        
  int nRef;            
};






struct openKey {
  dev_t dev;   
  ino_t ino;   
};








struct openCnt {
  struct openKey key;   
  int nRef;             
  int nLock;            
  int nPending;         
  int *aPending;        
};






static Hash lockHash = {SQLITE_HASH_BINARY, 0, 0, 0, 
    sqlite3ThreadSafeMalloc, sqlite3ThreadSafeFree, 0, 0};
static Hash openHash = {SQLITE_HASH_BINARY, 0, 0, 0, 
    sqlite3ThreadSafeMalloc, sqlite3ThreadSafeFree, 0, 0};

#ifdef SQLITE_UNIX_THREADS


















#ifndef SQLITE_THREAD_OVERRIDE_LOCK
# define SQLITE_THREAD_OVERRIDE_LOCK -1
#endif
#ifdef SQLITE_TEST
int threadsOverrideEachOthersLocks = SQLITE_THREAD_OVERRIDE_LOCK;
#else
static int threadsOverrideEachOthersLocks = SQLITE_THREAD_OVERRIDE_LOCK;
#endif





struct threadTestData {
  int fd;                
  struct flock lock;     
  int result;            
};

#ifdef SQLITE_LOCK_TRACE








static int lockTrace(int fd, int op, struct flock *p){
  char *zOpName, *zType;
  int s;
  int savedErrno;
  if( op==F_GETLK ){
    zOpName = "GETLK";
  }else if( op==F_SETLK ){
    zOpName = "SETLK";
  }else{
    s = fcntl(fd, op, p);
    sqlite3DebugPrintf("fcntl unknown %d %d %d\n", fd, op, s);
    return s;
  }
  if( p->l_type==F_RDLCK ){
    zType = "RDLCK";
  }else if( p->l_type==F_WRLCK ){
    zType = "WRLCK";
  }else if( p->l_type==F_UNLCK ){
    zType = "UNLCK";
  }else{
    assert( 0 );
  }
  assert( p->l_whence==SEEK_SET );
  s = fcntl(fd, op, p);
  savedErrno = errno;
  sqlite3DebugPrintf("fcntl %d %d %s %s %d %d %d %d\n",
     threadid, fd, zOpName, zType, (int)p->l_start, (int)p->l_len,
     (int)p->l_pid, s);
  if( s && op==F_SETLK && (p->l_type==F_RDLCK || p->l_type==F_WRLCK) ){
    struct flock l2;
    l2 = *p;
    fcntl(fd, F_GETLK, &l2);
    if( l2.l_type==F_RDLCK ){
      zType = "RDLCK";
    }else if( l2.l_type==F_WRLCK ){
      zType = "WRLCK";
    }else if( l2.l_type==F_UNLCK ){
      zType = "UNLCK";
    }else{
      assert( 0 );
    }
    sqlite3DebugPrintf("fcntl-failure-reason: %s %d %d %d\n",
       zType, (int)l2.l_start, (int)l2.l_len, (int)l2.l_pid);
  }
  errno = savedErrno;
  return s;
}
#define fcntl lockTrace
#endif 








static void *threadLockingTest(void *pArg){
  struct threadTestData *pData = (struct threadTestData*)pArg;
  pData->result = fcntl(pData->fd, F_SETLK, &pData->lock);
  return pArg;
}






static void testThreadLockingBehavior(int fd_orig){
  int fd;
  struct threadTestData d[2];
  pthread_t t[2];

  fd = dup(fd_orig);
  if( fd<0 ) return;
  memset(d, 0, sizeof(d));
  d[0].fd = fd;
  d[0].lock.l_type = F_RDLCK;
  d[0].lock.l_len = 1;
  d[0].lock.l_start = 0;
  d[0].lock.l_whence = SEEK_SET;
  d[1] = d[0];
  d[1].lock.l_type = F_WRLCK;
  pthread_create(&t[0], 0, threadLockingTest, &d[0]);
  pthread_create(&t[1], 0, threadLockingTest, &d[1]);
  pthread_join(t[0], 0);
  pthread_join(t[1], 0);
  close(fd);
  threadsOverrideEachOthersLocks =  d[0].result==0 && d[1].result==0;
}
#endif 




static void releaseLockInfo(struct lockInfo *pLock){
  assert( sqlite3OsInMutex(1) );
  pLock->nRef--;
  if( pLock->nRef==0 ){
    sqlite3HashInsert(&lockHash, &pLock->key, sizeof(pLock->key), 0);
    sqlite3ThreadSafeFree(pLock);
  }
}




static void releaseOpenCnt(struct openCnt *pOpen){
  assert( sqlite3OsInMutex(1) );
  pOpen->nRef--;
  if( pOpen->nRef==0 ){
    sqlite3HashInsert(&openHash, &pOpen->key, sizeof(pOpen->key), 0);
    free(pOpen->aPending);
    sqlite3ThreadSafeFree(pOpen);
  }
}








static int findLockInfo(
  int fd,                      
  struct lockInfo **ppLock,    
  struct openCnt **ppOpen      
){
  int rc;
  struct lockKey key1;
  struct openKey key2;
  struct stat statbuf;
  struct lockInfo *pLock;
  struct openCnt *pOpen;
  rc = fstat(fd, &statbuf);
  if( rc!=0 ) return 1;

  assert( sqlite3OsInMutex(1) );
  memset(&key1, 0, sizeof(key1));
  key1.dev = statbuf.st_dev;
  key1.ino = statbuf.st_ino;
#ifdef SQLITE_UNIX_THREADS
  if( threadsOverrideEachOthersLocks<0 ){
    testThreadLockingBehavior(fd);
  }
  key1.tid = threadsOverrideEachOthersLocks ? 0 : pthread_self();
#endif
  memset(&key2, 0, sizeof(key2));
  key2.dev = statbuf.st_dev;
  key2.ino = statbuf.st_ino;
  pLock = (struct lockInfo*)sqlite3HashFind(&lockHash, &key1, sizeof(key1));
  if( pLock==0 ){
    struct lockInfo *pOld;
    pLock = sqlite3ThreadSafeMalloc( sizeof(*pLock) );
    if( pLock==0 ){
      rc = 1;
      goto exit_findlockinfo;
    }
    pLock->key = key1;
    pLock->nRef = 1;
    pLock->cnt = 0;
    pLock->locktype = 0;
    pOld = sqlite3HashInsert(&lockHash, &pLock->key, sizeof(key1), pLock);
    if( pOld!=0 ){
      assert( pOld==pLock );
      sqlite3ThreadSafeFree(pLock);
      rc = 1;
      goto exit_findlockinfo;
    }
  }else{
    pLock->nRef++;
  }
  *ppLock = pLock;
  if( ppOpen!=0 ){
    pOpen = (struct openCnt*)sqlite3HashFind(&openHash, &key2, sizeof(key2));
    if( pOpen==0 ){
      struct openCnt *pOld;
      pOpen = sqlite3ThreadSafeMalloc( sizeof(*pOpen) );
      if( pOpen==0 ){
        releaseLockInfo(pLock);
        rc = 1;
        goto exit_findlockinfo;
      }
      pOpen->key = key2;
      pOpen->nRef = 1;
      pOpen->nLock = 0;
      pOpen->nPending = 0;
      pOpen->aPending = 0;
      pOld = sqlite3HashInsert(&openHash, &pOpen->key, sizeof(key2), pOpen);
      if( pOld!=0 ){
        assert( pOld==pOpen );
        sqlite3ThreadSafeFree(pOpen);
        releaseLockInfo(pLock);
        rc = 1;
        goto exit_findlockinfo;
      }
    }else{
      pOpen->nRef++;
    }
    *ppOpen = pOpen;
  }

exit_findlockinfo:
  return rc;
}

#ifdef SQLITE_DEBUG





static const char *locktypeName(int locktype){
  switch( locktype ){
  case NO_LOCK: return "NONE";
  case SHARED_LOCK: return "SHARED";
  case RESERVED_LOCK: return "RESERVED";
  case PENDING_LOCK: return "PENDING";
  case EXCLUSIVE_LOCK: return "EXCLUSIVE";
  }
  return "ERROR";
}
#endif














#ifdef SQLITE_UNIX_THREADS
static int transferOwnership(unixFile *pFile){
  int rc;
  pthread_t hSelf;
  if( threadsOverrideEachOthersLocks ){
    
    return SQLITE_OK;
  }
  hSelf = pthread_self();
  if( pthread_equal(pFile->tid, hSelf) ){
    
    TRACE1("No-transfer, same thread\n");
    return SQLITE_OK;
  }
  if( pFile->locktype!=NO_LOCK ){
    
    return SQLITE_MISUSE;
  }
  TRACE4("Transfer ownership of %d from %d to %d\n", pFile->h,pFile->tid,hSelf);
  pFile->tid = hSelf;
  releaseLockInfo(pFile->pLock);
  rc = findLockInfo(pFile->h, &pFile->pLock, 0);
  TRACE5("LOCK    %d is now %s(%s,%d)\n", pFile->h,
     locktypeName(pFile->locktype),
     locktypeName(pFile->pLock->locktype), pFile->pLock->cnt);
  return rc;
}
#else
  
# define transferOwnership(X) SQLITE_OK
#endif




int sqlite3UnixDelete(const char *zFilename){
  unlink(zFilename);
  return SQLITE_OK;
}




int sqlite3UnixFileExists(const char *zFilename){
  return access(zFilename, 0)==0;
}


static int allocateUnixFile(unixFile *pInit, OsFile **pId);














int sqlite3UnixOpenReadWrite(
  const char *zFilename,
  OsFile **pId,
  int *pReadonly
){
  int rc;
  unixFile f;

  CRASH_TEST_OVERRIDE(sqlite3CrashOpenReadWrite, zFilename, pId, pReadonly);
  assert( 0==*pId );
  f.h = open(zFilename, O_RDWR|O_CREAT|O_LARGEFILE|O_BINARY,
                          SQLITE_DEFAULT_FILE_PERMISSIONS);
  if( f.h<0 ){
#ifdef EISDIR
    if( errno==EISDIR ){
      return SQLITE_CANTOPEN;
    }
#endif
    f.h = open(zFilename, O_RDONLY|O_LARGEFILE|O_BINARY);
    if( f.h<0 ){
      return SQLITE_CANTOPEN; 
    }
    *pReadonly = 1;
  }else{
    *pReadonly = 0;
  }
  sqlite3OsEnterMutex();
  rc = findLockInfo(f.h, &f.pLock, &f.pOpen);
  sqlite3OsLeaveMutex();
  if( rc ){
    close(f.h);
    return SQLITE_NOMEM;
  }
  TRACE3("OPEN    %-3d %s\n", f.h, zFilename);
  return allocateUnixFile(&f, pId);
}
















int sqlite3UnixOpenExclusive(const char *zFilename, OsFile **pId, int delFlag){
  int rc;
  unixFile f;

  CRASH_TEST_OVERRIDE(sqlite3CrashOpenExclusive, zFilename, pId, delFlag);
  assert( 0==*pId );
  f.h = open(zFilename,
                O_RDWR|O_CREAT|O_EXCL|O_NOFOLLOW|O_LARGEFILE|O_BINARY,
                SQLITE_DEFAULT_FILE_PERMISSIONS);
  if( f.h<0 ){
    return SQLITE_CANTOPEN;
  }
  sqlite3OsEnterMutex();
  rc = findLockInfo(f.h, &f.pLock, &f.pOpen);
  sqlite3OsLeaveMutex();
  if( rc ){
    close(f.h);
    unlink(zFilename);
    return SQLITE_NOMEM;
  }
  if( delFlag ){
    unlink(zFilename);
  }
  TRACE3("OPEN-EX %-3d %s\n", f.h, zFilename);
  return allocateUnixFile(&f, pId);
}








int sqlite3UnixOpenReadOnly(const char *zFilename, OsFile **pId){
  int rc;
  unixFile f;

  CRASH_TEST_OVERRIDE(sqlite3CrashOpenReadOnly, zFilename, pId, 0);
  assert( 0==*pId );
  f.h = open(zFilename, O_RDONLY|O_LARGEFILE|O_BINARY);
  if( f.h<0 ){
    return SQLITE_CANTOPEN;
  }
  sqlite3OsEnterMutex();
  rc = findLockInfo(f.h, &f.pLock, &f.pOpen);
  sqlite3OsLeaveMutex();
  if( rc ){
    close(f.h);
    return SQLITE_NOMEM;
  }
  TRACE3("OPEN-RO %-3d %s\n", f.h, zFilename);
  return allocateUnixFile(&f, pId);
}

















static int unixOpenDirectory(
  OsFile *id,
  const char *zDirname
){
  unixFile *pFile = (unixFile*)id;
  if( pFile==0 ){
    

    return SQLITE_CANTOPEN;
  }
  SET_THREADID(pFile);
  assert( pFile->dirfd<0 );
  pFile->dirfd = open(zDirname, O_RDONLY|O_BINARY, 0);
  if( pFile->dirfd<0 ){
    return SQLITE_CANTOPEN; 
  }
  TRACE3("OPENDIR %-3d %s\n", pFile->dirfd, zDirname);
  return SQLITE_OK;
}








char *sqlite3_temp_directory = 0;





int sqlite3UnixTempFileName(char *zBuf){
  static const char *azDirs[] = {
     0,
     "/var/tmp",
     "/usr/tmp",
     "/tmp",
     ".",
  };
  static const unsigned char zChars[] =
    "abcdefghijklmnopqrstuvwxyz"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "0123456789";
  int i, j;
  struct stat buf;
  const char *zDir = ".";
  azDirs[0] = sqlite3_temp_directory;
  for(i=0; i<sizeof(azDirs)/sizeof(azDirs[0]); i++){
    if( azDirs[i]==0 ) continue;
    if( stat(azDirs[i], &buf) ) continue;
    if( !S_ISDIR(buf.st_mode) ) continue;
    if( access(azDirs[i], 07) ) continue;
    zDir = azDirs[i];
    break;
  }
  do{
    sprintf(zBuf, "%s/"TEMP_FILE_PREFIX, zDir);
    j = strlen(zBuf);
    sqlite3Randomness(15, &zBuf[j]);
    for(i=0; i<15; i++, j++){
      zBuf[j] = (char)zChars[ ((unsigned char)zBuf[j])%(sizeof(zChars)-1) ];
    }
    zBuf[j] = 0;
  }while( access(zBuf,0)==0 );
  return SQLITE_OK; 
}





int sqlite3UnixIsDirWritable(char *zBuf){
#ifndef SQLITE_OMIT_PAGER_PRAGMAS
  struct stat buf;
  if( zBuf==0 ) return 0;
  if( zBuf[0]==0 ) return 0;
  if( stat(zBuf, &buf) ) return 0;
  if( !S_ISDIR(buf.st_mode) ) return 0;
  if( access(zBuf, 07) ) return 0;
#endif 
  return 1;
}





static int seekAndRead(unixFile *id, void *pBuf, int cnt){
  int got;
#ifdef USE_PREAD
  got = pread(id->h, pBuf, cnt, id->offset);
#else
  lseek(id->h, id->offset, SEEK_SET);
  got = read(id->h, pBuf, cnt);
#endif
  if( got>0 ){
    id->offset += got;
  }
  return got;
}






static int unixRead(OsFile *id, void *pBuf, int amt){
  int got;
  assert( id );
  SimulateIOError(SQLITE_IOERR);
  TIMER_START;
  got = seekAndRead((unixFile*)id, pBuf, amt);
  TIMER_END;
  TRACE5("READ    %-3d %5d %7d %d\n", ((unixFile*)id)->h, got,
          last_page, TIMER_ELAPSED);
  SEEK(0);
  
  if( got==amt ){
    return SQLITE_OK;
  }else{
    return SQLITE_IOERR;
  }
}





static int seekAndWrite(unixFile *id, const void *pBuf, int cnt){
  int got;
#ifdef USE_PREAD
  got = pwrite(id->h, pBuf, cnt, id->offset);
#else
  lseek(id->h, id->offset, SEEK_SET);
  got = write(id->h, pBuf, cnt);
#endif
  if( got>0 ){
    id->offset += got;
  }
  return got;
}






static int unixWrite(OsFile *id, const void *pBuf, int amt){
  int wrote = 0;
  assert( id );
  assert( amt>0 );
  SimulateIOError(SQLITE_IOERR);
  SimulateDiskfullError;
  TIMER_START;
  while( amt>0 && (wrote = seekAndWrite((unixFile*)id, pBuf, amt))>0 ){
    amt -= wrote;
    pBuf = &((char*)pBuf)[wrote];
  }
  TIMER_END;
  TRACE5("WRITE   %-3d %5d %7d %d\n", ((unixFile*)id)->h, wrote,
          last_page, TIMER_ELAPSED);
  SEEK(0);
  if( amt>0 ){
    return SQLITE_FULL;
  }
  return SQLITE_OK;
}




static int unixSeek(OsFile *id, i64 offset){
  assert( id );
  SEEK(offset/1024 + 1);
#ifdef SQLITE_TEST
  if( offset ) SimulateDiskfullError
#endif
  ((unixFile*)id)->offset = offset;
  return SQLITE_OK;
}

#ifdef SQLITE_TEST




int sqlite3_sync_count = 0;
int sqlite3_fullsync_count = 0;
#endif





#ifndef HAVE_FDATASYNC
# define fdatasync fsync
#endif






#ifdef F_FULLFSYNC
# define HAVE_FULLFSYNC 1
#else
# define HAVE_FULLFSYNC 0
#endif













static int full_fsync(int fd, int fullSync, int dataOnly){
  int rc;

  



#ifdef SQLITE_TEST
  if( fullSync ) sqlite3_fullsync_count++;
  sqlite3_sync_count++;
#endif

  


#ifdef SQLITE_NO_SYNC
  rc = SQLITE_OK;
#else

#if HAVE_FULLFSYNC
  if( fullSync ){
    rc = fcntl(fd, F_FULLFSYNC, 0);
  }else{
    rc = 1;
  }
  
  if( rc ) rc = fsync(fd);

#else 
  if( dataOnly ){
    rc = fdatasync(fd);
  }else{
    rc = fsync(fd);
  }
#endif 
#endif 

  return rc;
}
















static int unixSync(OsFile *id, int dataOnly){
  unixFile *pFile = (unixFile*)id;
  assert( pFile );
  SimulateIOError(SQLITE_IOERR);
  TRACE2("SYNC    %-3d\n", pFile->h);
  if( full_fsync(pFile->h, pFile->fullSync, dataOnly) ){
    return SQLITE_IOERR;
  }
  if( pFile->dirfd>=0 ){
    TRACE4("DIRSYNC %-3d (have_fullfsync=%d fullsync=%d)\n", pFile->dirfd,
            HAVE_FULLFSYNC, pFile->fullSync);
#ifndef SQLITE_DISABLE_DIRSYNC
    



    if( (!HAVE_FULLFSYNC || !pFile->fullSync) && full_fsync(pFile->dirfd,0,0) ){
       





       
    }
#endif
    close(pFile->dirfd);  
    pFile->dirfd = -1;    
  }
  return SQLITE_OK;
}









int sqlite3UnixSyncDirectory(const char *zDirname){
#ifdef SQLITE_DISABLE_DIRSYNC
  return SQLITE_OK;
#else
  int fd;
  int r;
  SimulateIOError(SQLITE_IOERR);
  fd = open(zDirname, O_RDONLY|O_BINARY, 0);
  TRACE3("DIRSYNC %-3d (%s)\n", fd, zDirname);
  if( fd<0 ){
    return SQLITE_CANTOPEN; 
  }
  r = fsync(fd);
  close(fd);
  return ((r==0)?SQLITE_OK:SQLITE_IOERR);
#endif
}




static int unixTruncate(OsFile *id, i64 nByte){
  assert( id );
  SimulateIOError(SQLITE_IOERR);
  return ftruncate(((unixFile*)id)->h, nByte)==0 ? SQLITE_OK : SQLITE_IOERR;
}




static int unixFileSize(OsFile *id, i64 *pSize){
  struct stat buf;
  assert( id );
  SimulateIOError(SQLITE_IOERR);
  if( fstat(((unixFile*)id)->h, &buf)!=0 ){
    return SQLITE_IOERR;
  }
  *pSize = buf.st_size;
  return SQLITE_OK;
}







static int unixCheckReservedLock(OsFile *id){
  int r = 0;
  unixFile *pFile = (unixFile*)id;

  assert( pFile );
  sqlite3OsEnterMutex(); 

  
  if( pFile->pLock->locktype>SHARED_LOCK ){
    r = 1;
  }

  

  if( !r ){
    struct flock lock;
    lock.l_whence = SEEK_SET;
    lock.l_start = RESERVED_BYTE;
    lock.l_len = 1;
    lock.l_type = F_WRLCK;
    fcntl(pFile->h, F_GETLK, &lock);
    if( lock.l_type!=F_UNLCK ){
      r = 1;
    }
  }
  
  sqlite3OsLeaveMutex();
  TRACE3("TEST WR-LOCK %d %d\n", pFile->h, r);

  return r;
}

























static int unixLock(OsFile *id, int locktype){
  





































  int rc = SQLITE_OK;
  unixFile *pFile = (unixFile*)id;
  struct lockInfo *pLock = pFile->pLock;
  struct flock lock;
  int s;

  assert( pFile );
  TRACE7("LOCK    %d %s was %s(%s,%d) pid=%d\n", pFile->h,
      locktypeName(locktype), locktypeName(pFile->locktype),
      locktypeName(pLock->locktype), pLock->cnt , getpid());

  



  if( pFile->locktype>=locktype ){
    TRACE3("LOCK    %d %s ok (already held)\n", pFile->h,
            locktypeName(locktype));
    return SQLITE_OK;
  }

  

  assert( pFile->locktype!=NO_LOCK || locktype==SHARED_LOCK );
  assert( locktype!=PENDING_LOCK );
  assert( locktype!=RESERVED_LOCK || pFile->locktype==SHARED_LOCK );

  

  sqlite3OsEnterMutex();

  

  rc = transferOwnership(pFile);
  if( rc!=SQLITE_OK ){
    sqlite3OsLeaveMutex();
    return rc;
  }
  pLock = pFile->pLock;

  


  if( (pFile->locktype!=pLock->locktype && 
          (pLock->locktype>=PENDING_LOCK || locktype>SHARED_LOCK))
  ){
    rc = SQLITE_BUSY;
    goto end_lock;
  }

  



  if( locktype==SHARED_LOCK && 
      (pLock->locktype==SHARED_LOCK || pLock->locktype==RESERVED_LOCK) ){
    assert( locktype==SHARED_LOCK );
    assert( pFile->locktype==0 );
    assert( pLock->cnt>0 );
    pFile->locktype = SHARED_LOCK;
    pLock->cnt++;
    pFile->pOpen->nLock++;
    goto end_lock;
  }

  lock.l_len = 1L;

  lock.l_whence = SEEK_SET;

  



  if( locktype==SHARED_LOCK 
      || (locktype==EXCLUSIVE_LOCK && pFile->locktype<PENDING_LOCK)
  ){
    lock.l_type = (locktype==SHARED_LOCK?F_RDLCK:F_WRLCK);
    lock.l_start = PENDING_BYTE;
    s = fcntl(pFile->h, F_SETLK, &lock);
    if( s ){
      rc = (errno==EINVAL) ? SQLITE_NOLFS : SQLITE_BUSY;
      goto end_lock;
    }
  }


  


  if( locktype==SHARED_LOCK ){
    assert( pLock->cnt==0 );
    assert( pLock->locktype==0 );

    
    lock.l_start = SHARED_FIRST;
    lock.l_len = SHARED_SIZE;
    s = fcntl(pFile->h, F_SETLK, &lock);

    
    lock.l_start = PENDING_BYTE;
    lock.l_len = 1L;
    lock.l_type = F_UNLCK;
    if( fcntl(pFile->h, F_SETLK, &lock)!=0 ){
      rc = SQLITE_IOERR;  
      goto end_lock;
    }
    if( s ){
      rc = (errno==EINVAL) ? SQLITE_NOLFS : SQLITE_BUSY;
    }else{
      pFile->locktype = SHARED_LOCK;
      pFile->pOpen->nLock++;
      pLock->cnt = 1;
    }
  }else if( locktype==EXCLUSIVE_LOCK && pLock->cnt>1 ){
    

    rc = SQLITE_BUSY;
  }else{
    



    assert( 0!=pFile->locktype );
    lock.l_type = F_WRLCK;
    switch( locktype ){
      case RESERVED_LOCK:
        lock.l_start = RESERVED_BYTE;
        break;
      case EXCLUSIVE_LOCK:
        lock.l_start = SHARED_FIRST;
        lock.l_len = SHARED_SIZE;
        break;
      default:
        assert(0);
    }
    s = fcntl(pFile->h, F_SETLK, &lock);
    if( s ){
      rc = (errno==EINVAL) ? SQLITE_NOLFS : SQLITE_BUSY;
    }
  }
  
  if( rc==SQLITE_OK ){
    pFile->locktype = locktype;
    pLock->locktype = locktype;
  }else if( locktype==EXCLUSIVE_LOCK ){
    pFile->locktype = PENDING_LOCK;
    pLock->locktype = PENDING_LOCK;
  }

end_lock:
  sqlite3OsLeaveMutex();
  TRACE4("LOCK    %d %s %s\n", pFile->h, locktypeName(locktype), 
      rc==SQLITE_OK ? "ok" : "failed");
  return rc;
}








static int unixUnlock(OsFile *id, int locktype){
  struct lockInfo *pLock;
  struct flock lock;
  int rc = SQLITE_OK;
  unixFile *pFile = (unixFile*)id;

  assert( pFile );
  TRACE7("UNLOCK  %d %d was %d(%d,%d) pid=%d\n", pFile->h, locktype,
      pFile->locktype, pFile->pLock->locktype, pFile->pLock->cnt, getpid());

  assert( locktype<=SHARED_LOCK );
  if( pFile->locktype<=locktype ){
    return SQLITE_OK;
  }
  if( CHECK_THREADID(pFile) ){
    return SQLITE_MISUSE;
  }
  sqlite3OsEnterMutex();
  pLock = pFile->pLock;
  assert( pLock->cnt!=0 );
  if( pFile->locktype>SHARED_LOCK ){
    assert( pLock->locktype==pFile->locktype );
    if( locktype==SHARED_LOCK ){
      lock.l_type = F_RDLCK;
      lock.l_whence = SEEK_SET;
      lock.l_start = SHARED_FIRST;
      lock.l_len = SHARED_SIZE;
      if( fcntl(pFile->h, F_SETLK, &lock)!=0 ){
        
        rc = SQLITE_IOERR;
      }
    }
    lock.l_type = F_UNLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = PENDING_BYTE;
    lock.l_len = 2L;  assert( PENDING_BYTE+1==RESERVED_BYTE );
    if( fcntl(pFile->h, F_SETLK, &lock)==0 ){
      pLock->locktype = SHARED_LOCK;
    }else{
      rc = SQLITE_IOERR;  
    }
  }
  if( locktype==NO_LOCK ){
    struct openCnt *pOpen;

    



    pLock->cnt--;
    if( pLock->cnt==0 ){
      lock.l_type = F_UNLCK;
      lock.l_whence = SEEK_SET;
      lock.l_start = lock.l_len = 0L;
      if( fcntl(pFile->h, F_SETLK, &lock)==0 ){
        pLock->locktype = NO_LOCK;
      }else{
        rc = SQLITE_IOERR;  
      }
    }

    



    pOpen = pFile->pOpen;
    pOpen->nLock--;
    assert( pOpen->nLock>=0 );
    if( pOpen->nLock==0 && pOpen->nPending>0 ){
      int i;
      for(i=0; i<pOpen->nPending; i++){
        close(pOpen->aPending[i]);
      }
      free(pOpen->aPending);
      pOpen->nPending = 0;
      pOpen->aPending = 0;
    }
  }
  sqlite3OsLeaveMutex();
  pFile->locktype = locktype;
  return rc;
}




static int unixClose(OsFile **pId){
  unixFile *id = (unixFile*)*pId;

  if( !id ) return SQLITE_OK;
  unixUnlock(*pId, NO_LOCK);
  if( id->dirfd>=0 ) close(id->dirfd);
  id->dirfd = -1;
  sqlite3OsEnterMutex();

  if( id->pOpen->nLock ){
    




    int *aNew;
    struct openCnt *pOpen = id->pOpen;
    aNew = realloc( pOpen->aPending, (pOpen->nPending+1)*sizeof(int) );
    if( aNew==0 ){
      
    }else{
      pOpen->aPending = aNew;
      pOpen->aPending[pOpen->nPending] = id->h;
      pOpen->nPending++;
    }
  }else{
    
    close(id->h);
  }
  releaseLockInfo(id->pLock);
  releaseOpenCnt(id->pOpen);

  sqlite3OsLeaveMutex();
  id->isOpen = 0;
  TRACE2("CLOSE   %-3d\n", id->h);
  OpenCounter(-1);
  sqlite3ThreadSafeFree(id);
  *pId = 0;
  return SQLITE_OK;
}







char *sqlite3UnixFullPathname(const char *zRelative){
  char *zFull = 0;
  if( zRelative[0]=='/' ){
    sqlite3SetString(&zFull, zRelative, (char*)0);
  }else{
    char *zBuf = sqliteMalloc(5000);
    if( zBuf==0 ){
      return 0;
    }
    zBuf[0] = 0;
    sqlite3SetString(&zFull, getcwd(zBuf, 5000), "/", zRelative,
                    (char*)0);
    sqliteFree(zBuf);
  }

#if 0
  



  if( zFull ){
    int i, j;
    for(i=j=0; zFull[i]; i++){
      if( zFull[i]=='/' ){
        if( zFull[i+1]=='/' ) continue;
        if( zFull[i+1]=='.' && zFull[i+2]=='/' ){
          i += 1;
          continue;
        }
        if( zFull[i+1]=='.' && zFull[i+2]=='.' && zFull[i+3]=='/' ){
          while( j>0 && zFull[j-1]!='/' ){ j--; }
          i += 3;
          continue;
        }
      }
      zFull[j++] = zFull[i];
    }
    zFull[j] = 0;
  }
#endif

  return zFull;
}




static void unixSetFullSync(OsFile *id, int v){
  ((unixFile*)id)->fullSync = v;
}




static int unixFileHandle(OsFile *id){
  return ((unixFile*)id)->h;
}





static int unixLockState(OsFile *id){
  return ((unixFile*)id)->locktype;
}





static const IoMethod sqlite3UnixIoMethod = {
  unixClose,
  unixOpenDirectory,
  unixRead,
  unixWrite,
  unixSeek,
  unixTruncate,
  unixSync,
  unixSetFullSync,
  unixFileHandle,
  unixFileSize,
  unixLock,
  unixUnlock,
  unixLockState,
  unixCheckReservedLock,
};






static int allocateUnixFile(unixFile *pInit, OsFile **pId){
  unixFile *pNew;
  pInit->dirfd = -1;
  pInit->fullSync = 0;
  pInit->locktype = 0;
  pInit->offset = 0;
  SET_THREADID(pInit);
  pNew = sqlite3ThreadSafeMalloc( sizeof(unixFile) );
  if( pNew==0 ){
    close(pInit->h);
    sqlite3OsEnterMutex();
    releaseLockInfo(pInit->pLock);
    releaseOpenCnt(pInit->pOpen);
    sqlite3OsLeaveMutex();
    *pId = 0;
    return SQLITE_NOMEM;
  }else{
    *pNew = *pInit;
    pNew->pMethod = &sqlite3UnixIoMethod;
    *pId = (OsFile*)pNew;
    OpenCounter(+1);
    return SQLITE_OK;
  }
}


#endif 











int sqlite3UnixRandomSeed(char *zBuf){
  











  memset(zBuf, 0, 256);
#if !defined(SQLITE_TEST)
  {
    int pid, fd;
    fd = open("/dev/urandom", O_RDONLY);
    if( fd<0 ){
      time_t t;
      time(&t);
      memcpy(zBuf, &t, sizeof(t));
      pid = getpid();
      memcpy(&zBuf[sizeof(time_t)], &pid, sizeof(pid));
    }else{
      read(fd, zBuf, 256);
      close(fd);
    }
  }
#endif
  return SQLITE_OK;
}





int sqlite3UnixSleep(int ms){
#if defined(HAVE_USLEEP) && HAVE_USLEEP
  usleep(ms*1000);
  return ms;
#else
  sleep((ms+999)/1000);
  return 1000*((ms+999)/1000);
#endif
}
































static int inMutex = 0;
#ifdef SQLITE_UNIX_THREADS
static pthread_t mutexOwner;          
static int mutexOwnerValid = 0;       
static pthread_mutex_t mutexMain = PTHREAD_MUTEX_INITIALIZER; 
static pthread_mutex_t mutexAux = PTHREAD_MUTEX_INITIALIZER;  
#endif











void sqlite3UnixEnterMutex(){
#ifdef SQLITE_UNIX_THREADS
  pthread_mutex_lock(&mutexAux);
  if( !mutexOwnerValid || !pthread_equal(mutexOwner, pthread_self()) ){
    pthread_mutex_unlock(&mutexAux);
    pthread_mutex_lock(&mutexMain);
    assert( inMutex==0 );
    assert( !mutexOwnerValid );
    pthread_mutex_lock(&mutexAux);
    mutexOwner = pthread_self();
    mutexOwnerValid = 1;
  }
  inMutex++;
  pthread_mutex_unlock(&mutexAux);
#else
  inMutex++;
#endif
}
void sqlite3UnixLeaveMutex(){
  assert( inMutex>0 );
#ifdef SQLITE_UNIX_THREADS
  pthread_mutex_lock(&mutexAux);
  inMutex--;
  assert( pthread_equal(mutexOwner, pthread_self()) );
  if( inMutex==0 ){
    assert( mutexOwnerValid );
    mutexOwnerValid = 0;
    pthread_mutex_unlock(&mutexMain);
  }
  pthread_mutex_unlock(&mutexAux);
#else
  inMutex--;
#endif
}








int sqlite3UnixInMutex(int thisThrd){
#ifdef SQLITE_UNIX_THREADS
  int rc;
  pthread_mutex_lock(&mutexAux);
  rc = inMutex>0 && (thisThrd==0 || pthread_equal(mutexOwner,pthread_self()));
  pthread_mutex_unlock(&mutexAux);
  return rc;
#else
  return inMutex>0;
#endif
}






#ifdef SQLITE_TEST
int sqlite3_tsd_count = 0;
# ifdef SQLITE_UNIX_THREADS
    static pthread_mutex_t tsd_counter_mutex = PTHREAD_MUTEX_INITIALIZER;
#   define TSD_COUNTER(N) \
             pthread_mutex_lock(&tsd_counter_mutex); \
             sqlite3_tsd_count += N; \
             pthread_mutex_unlock(&tsd_counter_mutex);
# else
#   define TSD_COUNTER(N)  sqlite3_tsd_count += N
# endif
#else
# define TSD_COUNTER(N)
#endif















ThreadData *sqlite3UnixThreadSpecificData(int allocateFlag){
  static const ThreadData zeroData = {0};  

#ifdef SQLITE_UNIX_THREADS
  static pthread_key_t key;
  static int keyInit = 0;
  ThreadData *pTsd;

  if( !keyInit ){
    sqlite3OsEnterMutex();
    if( !keyInit ){
      int rc;
      rc = pthread_key_create(&key, 0);
      if( rc ){
        sqlite3OsLeaveMutex();
        return 0;
      }
      keyInit = 1;
    }
    sqlite3OsLeaveMutex();
  }

  pTsd = pthread_getspecific(key);
  if( allocateFlag>0 ){
    if( pTsd==0 ){
      if( !sqlite3TestMallocFail() ){
        pTsd = sqlite3OsMalloc(sizeof(zeroData));
      }
#ifdef SQLITE_MEMDEBUG
      sqlite3_isFail = 0;
#endif
      if( pTsd ){
        *pTsd = zeroData;
        pthread_setspecific(key, pTsd);
        TSD_COUNTER(+1);
      }
    }
  }else if( pTsd!=0 && allocateFlag<0 
            && memcmp(pTsd, &zeroData, sizeof(ThreadData))==0 ){
    sqlite3OsFree(pTsd);
    pthread_setspecific(key, 0);
    TSD_COUNTER(-1);
    pTsd = 0;
  }
  return pTsd;
#else
  static ThreadData *pTsd = 0;
  if( allocateFlag>0 ){
    if( pTsd==0 ){
      if( !sqlite3TestMallocFail() ){
        pTsd = sqlite3OsMalloc( sizeof(zeroData) );
      }
#ifdef SQLITE_MEMDEBUG
      sqlite3_isFail = 0;
#endif
      if( pTsd ){
        *pTsd = zeroData;
        TSD_COUNTER(+1);
      }
    }
  }else if( pTsd!=0 && allocateFlag<0
            && memcmp(pTsd, &zeroData, sizeof(ThreadData))==0 ){
    sqlite3OsFree(pTsd);
    TSD_COUNTER(-1);
    pTsd = 0;
  }
  return pTsd;
#endif
}





#ifdef SQLITE_TEST
int sqlite3_current_time = 0;
#endif






int sqlite3UnixCurrentTime(double *prNow){
#ifdef NO_GETTOD
  time_t t;
  time(&t);
  *prNow = t/86400.0 + 2440587.5;
#else
  struct timeval sNow;
  struct timezone sTz;  
  gettimeofday(&sNow, &sTz);
  *prNow = 2440587.5 + sNow.tv_sec/86400.0 + sNow.tv_usec/86400000000.0;
#endif
#ifdef SQLITE_TEST
  if( sqlite3_current_time ){
    *prNow = sqlite3_current_time/86400.0 + 2440587.5;
  }
#endif
  return 0;
}

#endif 
