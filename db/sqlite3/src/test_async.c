







































































#include "sqliteInt.h"
#include "os.h"
#include <tcl.h>


#ifndef THREADSAFE
# define THREADSAFE 0
#endif








#if OS_UNIX && THREADSAFE && defined(SQLITE_ENABLE_REDEF_IO)






#include <pthread.h>
#include <sched.h>


#define MIN(x,y) ((x)<(y)?(x):(y))
#define MAX(x,y) ((x)>(y)?(x):(y))


typedef struct AsyncWrite AsyncWrite;
typedef struct AsyncFile AsyncFile;


static int sqlite3async_trace = 0;
# define TRACE(X) if( sqlite3async_trace ) asyncTrace X
static void asyncTrace(const char *zFormat, ...){
  char *z;
  va_list ap;
  va_start(ap, zFormat);
  z = sqlite3_vmprintf(zFormat, ap);
  va_end(ap);
  fprintf(stderr, "[%d] %s", (int)pthread_self(), z);
  free(z);
}











































































































#ifndef SQLITE_ASYNC_TWO_FILEHANDLES

#define SQLITE_ASYNC_TWO_FILEHANDLES 1
#endif





static struct TestAsyncStaticData {
  pthread_mutex_t queueMutex;  
  pthread_mutex_t writerMutex; 
  pthread_mutex_t lockMutex;   
  pthread_cond_t queueSignal;  
  pthread_cond_t emptySignal;  
  AsyncWrite *pQueueFirst;     
  AsyncWrite *pQueueLast;      
  Hash aLock;                  
  volatile int ioDelay;             
  volatile int writerHaltWhenIdle;  
  volatile int writerHaltNow;       
  int ioError;                 
  int nFile;                   
} async = {
  PTHREAD_MUTEX_INITIALIZER,
  PTHREAD_MUTEX_INITIALIZER,
  PTHREAD_MUTEX_INITIALIZER,
  PTHREAD_COND_INITIALIZER,
  PTHREAD_COND_INITIALIZER,
};


#define ASYNC_NOOP          0
#define ASYNC_WRITE         1
#define ASYNC_SYNC          2
#define ASYNC_TRUNCATE      3
#define ASYNC_CLOSE         4
#define ASYNC_OPENDIRECTORY 5
#define ASYNC_SETFULLSYNC   6
#define ASYNC_DELETE        7
#define ASYNC_OPENEXCLUSIVE 8
#define ASYNC_SYNCDIRECTORY 9




static const char *azOpcodeName[] = {
  "NOOP", "WRITE", "SYNC", "TRUNCATE", "CLOSE",
  "OPENDIR", "SETFULLSYNC", "DELETE", "OPENEX", "SYNCDIR",
};















































struct AsyncWrite {
  AsyncFile *pFile;   
  int op;             
  i64 iOffset;        
  int nByte;          
  char *zBuf;         
  AsyncWrite *pNext;  
};




struct AsyncFile {
  IoMethod *pMethod;   
  i64 iOffset;         
  char *zName;         
  int nName;           
  OsFile *pBaseRead;   
  OsFile *pBaseWrite;  
};











static void addAsyncWrite(AsyncWrite *pWrite){
  
  pthread_mutex_lock(&async.queueMutex);

  
  assert( !pWrite->pNext );
  if( async.pQueueLast ){
    assert( async.pQueueFirst );
    async.pQueueLast->pNext = pWrite;
  }else{
    async.pQueueFirst = pWrite;
  }
  async.pQueueLast = pWrite;
  TRACE(("PUSH %p (%s %s %d)\n", pWrite, azOpcodeName[pWrite->op],
         pWrite->pFile ? pWrite->pFile->zName : "-", pWrite->iOffset));

  if( pWrite->op==ASYNC_CLOSE ){
    async.nFile--;
    if( async.nFile==0 ){
      async.ioError = SQLITE_OK;
    }
  }

  
  pthread_mutex_unlock(&async.queueMutex);

  

  pthread_cond_signal(&async.queueSignal);
}




static void incrOpenFileCount(){
  
  pthread_mutex_lock(&async.queueMutex);
  if( async.nFile==0 ){
    async.ioError = SQLITE_OK;
  }
  async.nFile++;
  pthread_mutex_unlock(&async.queueMutex);
}





static int addNewAsyncWrite(
  AsyncFile *pFile, 
  int op, 
  i64 iOffset, 
  int nByte,
  const char *zByte
){
  AsyncWrite *p;
  if( op!=ASYNC_CLOSE && async.ioError ){
    return async.ioError;
  }
  p = sqlite3OsMalloc(sizeof(AsyncWrite) + (zByte?nByte:0));
  if( !p ){
    return SQLITE_NOMEM;
  }
  p->op = op;
  p->iOffset = iOffset;
  p->nByte = nByte;
  p->pFile = pFile;
  p->pNext = 0;
  if( zByte ){
    p->zBuf = (char *)&p[1];
    memcpy(p->zBuf, zByte, nByte);
  }else{
    p->zBuf = 0;
  }
  addAsyncWrite(p);
  return SQLITE_OK;
}





static int asyncClose(OsFile **pId){
  return addNewAsyncWrite((AsyncFile *)*pId, ASYNC_CLOSE, 0, 0, 0);
}







static int asyncWrite(OsFile *id, const void *pBuf, int amt){
  AsyncFile *pFile = (AsyncFile *)id;
  int rc = addNewAsyncWrite(pFile, ASYNC_WRITE, pFile->iOffset, amt, pBuf);
  pFile->iOffset += (i64)amt;
  return rc;
}





static int asyncTruncate(OsFile *id, i64 nByte){
  return addNewAsyncWrite((AsyncFile *)id, ASYNC_TRUNCATE, nByte, 0, 0);
}






static int asyncOpenDirectory(OsFile *id, const char *zName){
  AsyncFile *pFile = (AsyncFile *)id;
  return addNewAsyncWrite(pFile, ASYNC_OPENDIRECTORY, 0, strlen(zName)+1,zName);
}





static int asyncSync(OsFile *id, int fullsync){
  return addNewAsyncWrite((AsyncFile *)id, ASYNC_SYNC, 0, fullsync, 0);
}





static void asyncSetFullSync(OsFile *id, int value){
  addNewAsyncWrite((AsyncFile *)id, ASYNC_SETFULLSYNC, 0, value, 0);
}








static int asyncRead(OsFile *id, void *obuf, int amt){
  int rc = SQLITE_OK;
  i64 filesize;
  int nRead;
  AsyncFile *pFile = (AsyncFile *)id;
  OsFile *pBase = pFile->pBaseRead;

  


  if( async.ioError!=SQLITE_OK ){
    return async.ioError;
  }

  
  pthread_mutex_lock(&async.queueMutex);

  if( pBase ){
    rc = sqlite3OsFileSize(pBase, &filesize);
    if( rc!=SQLITE_OK ){
      goto asyncread_out;
    }
    rc = sqlite3OsSeek(pBase, pFile->iOffset);
    if( rc!=SQLITE_OK ){
      goto asyncread_out;
    }
    nRead = MIN(filesize - pFile->iOffset, amt);
    if( nRead>0 ){
      rc = sqlite3OsRead(pBase, obuf, nRead);
      TRACE(("READ %s %d bytes at %d\n", pFile->zName, nRead, pFile->iOffset));
    }
  }

  if( rc==SQLITE_OK ){
    AsyncWrite *p;
    i64 iOffset = pFile->iOffset;           

    for(p=async.pQueueFirst; p; p = p->pNext){
      if( p->pFile==pFile && p->op==ASYNC_WRITE ){
        int iBeginOut = (p->iOffset - iOffset);
        int iBeginIn = -iBeginOut;
        int nCopy;

        if( iBeginIn<0 ) iBeginIn = 0;
        if( iBeginOut<0 ) iBeginOut = 0;
        nCopy = MIN(p->nByte-iBeginIn, amt-iBeginOut);

        if( nCopy>0 ){
          memcpy(&((char *)obuf)[iBeginOut], &p->zBuf[iBeginIn], nCopy);
          TRACE(("OVERREAD %d bytes at %d\n", nCopy, iBeginOut+iOffset));
        }
      }
    }

    pFile->iOffset += (i64)amt;
  }

asyncread_out:
  pthread_mutex_unlock(&async.queueMutex);
  return rc;
}






static int asyncSeek(OsFile *id, i64 offset){
  AsyncFile *pFile = (AsyncFile *)id;
  pFile->iOffset = offset;
  return SQLITE_OK;
}








int asyncFileSize(OsFile *id, i64 *pSize){
  int rc = SQLITE_OK;
  i64 s = 0;
  OsFile *pBase;

  pthread_mutex_lock(&async.queueMutex);

  




  pBase = ((AsyncFile *)id)->pBaseRead;
  if( pBase ){
    rc = sqlite3OsFileSize(pBase, &s);
  }

  if( rc==SQLITE_OK ){
    AsyncWrite *p;
    for(p=async.pQueueFirst; p; p = p->pNext){
      if( p->pFile==(AsyncFile *)id ){
        switch( p->op ){
          case ASYNC_WRITE:
            s = MAX(p->iOffset + (i64)(p->nByte), s);
            break;
          case ASYNC_TRUNCATE:
            s = MIN(s, p->iOffset);
            break;
        }
      }
    }
    *pSize = s;
  }
  pthread_mutex_unlock(&async.queueMutex);
  return rc;
}





static int asyncFileHandle(OsFile *id){
  return sqlite3OsFileHandle(((AsyncFile *)id)->pBaseRead);
}









static int asyncLock(OsFile *id, int lockType){
  AsyncFile *pFile = (AsyncFile*)id;
  TRACE(("LOCK %d (%s)\n", lockType, pFile->zName));
  pthread_mutex_lock(&async.lockMutex);
  sqlite3HashInsert(&async.aLock, pFile->zName, pFile->nName, (void*)lockType);
  pthread_mutex_unlock(&async.lockMutex);
  return SQLITE_OK;
}
static int asyncUnlock(OsFile *id, int lockType){
  return asyncLock(id, lockType);
}





static int asyncCheckReservedLock(OsFile *id){
  AsyncFile *pFile = (AsyncFile*)id;
  int rc;
  pthread_mutex_lock(&async.lockMutex);
  rc = (int)sqlite3HashFind(&async.aLock, pFile->zName, pFile->nName);
  pthread_mutex_unlock(&async.lockMutex);
  TRACE(("CHECK-LOCK %d (%s)\n", rc, pFile->zName));
  return rc>SHARED_LOCK;
}




static int asyncLockState(OsFile *id){
  return SQLITE_OK;
}






static int (*xOrigOpenReadWrite)(const char*, OsFile**, int*) = 0;
static int (*xOrigOpenExclusive)(const char*, OsFile**, int) = 0;
static int (*xOrigOpenReadOnly)(const char*, OsFile**) = 0;
static int (*xOrigDelete)(const char*) = 0;
static int (*xOrigFileExists)(const char*) = 0;
static int (*xOrigSyncDirectory)(const char*) = 0;





static int asyncOpenFile(
  const char *zName,     
  OsFile **pFile,        
  OsFile *pBaseRead,     
  int openForWriting     
){
  int rc, i, n;
  AsyncFile *p;
  OsFile *pBaseWrite = 0;

  static IoMethod iomethod = {
    asyncClose,
    asyncOpenDirectory,
    asyncRead,
    asyncWrite,
    asyncSeek,
    asyncTruncate,
    asyncSync,
    asyncSetFullSync,
    asyncFileHandle,
    asyncFileSize,
    asyncLock,
    asyncUnlock,
    asyncLockState,
    asyncCheckReservedLock
  };

  if( openForWriting && SQLITE_ASYNC_TWO_FILEHANDLES ){
    int dummy;
    rc = xOrigOpenReadWrite(zName, &pBaseWrite, &dummy);
    if( rc!=SQLITE_OK ){
      goto error_out;
    }
  }

  n = strlen(zName);
  for(i=n-1; i>=0 && zName[i]!='/'; i--){}
  p = (AsyncFile *)sqlite3OsMalloc(sizeof(AsyncFile) + n - i);
  if( !p ){
    rc = SQLITE_NOMEM;
    goto error_out;
  }
  memset(p, 0, sizeof(AsyncFile));
  p->zName = (char*)&p[1];
  strcpy(p->zName, &zName[i+1]);
  p->nName = n - i;
  p->pMethod = &iomethod;
  p->pBaseRead = pBaseRead;
  p->pBaseWrite = pBaseWrite;
  
  *pFile = (OsFile *)p;
  return SQLITE_OK;

error_out:
  assert(!p);
  sqlite3OsClose(&pBaseRead);
  sqlite3OsClose(&pBaseWrite);
  *pFile = 0;
  return rc;
}






static int asyncOpenExclusive(const char *z, OsFile **ppFile, int delFlag){
  int rc = asyncOpenFile(z, ppFile, 0, 0);
  if( rc==SQLITE_OK ){
    AsyncFile *pFile = (AsyncFile *)(*ppFile);
    int nByte = strlen(z)+1;
    i64 i = (i64)(delFlag);
    rc = addNewAsyncWrite(pFile, ASYNC_OPENEXCLUSIVE, i, nByte, z);
    if( rc!=SQLITE_OK ){
      sqlite3OsFree(pFile);
      *ppFile = 0;
    }
  }
  if( rc==SQLITE_OK ){
    incrOpenFileCount();
  }
  return rc;
}
static int asyncOpenReadOnly(const char *z, OsFile **ppFile){
  OsFile *pBase = 0;
  int rc = xOrigOpenReadOnly(z, &pBase);
  if( rc==SQLITE_OK ){
    rc = asyncOpenFile(z, ppFile, pBase, 0);
  }
  if( rc==SQLITE_OK ){
    incrOpenFileCount();
  }
  return rc;
}
static int asyncOpenReadWrite(const char *z, OsFile **ppFile, int *pReadOnly){
  OsFile *pBase = 0;
  int rc = xOrigOpenReadWrite(z, &pBase, pReadOnly);
  if( rc==SQLITE_OK ){
    rc = asyncOpenFile(z, ppFile, pBase, (*pReadOnly ? 0 : 1));
  }
  if( rc==SQLITE_OK ){
    incrOpenFileCount();
  }
  return rc;
}





static int asyncDelete(const char *z){
  return addNewAsyncWrite(0, ASYNC_DELETE, 0, strlen(z)+1, z);
}





static int asyncSyncDirectory(const char *z){
  return addNewAsyncWrite(0, ASYNC_SYNCDIRECTORY, 0, strlen(z)+1, z);
}







static int asyncFileExists(const char *z){
  int ret;
  AsyncWrite *p;

  pthread_mutex_lock(&async.queueMutex);

  
  ret = xOrigFileExists(z);
  
  for(p=async.pQueueFirst; p; p = p->pNext){
    if( p->op==ASYNC_DELETE && 0==strcmp(p->zBuf, z) ){
      ret = 0;
    }else if( p->op==ASYNC_OPENEXCLUSIVE && 0==strcmp(p->zBuf, z) ){
      ret = 1;
    }
  }

  TRACE(("EXISTS: %s = %d\n", z, ret));
  pthread_mutex_unlock(&async.queueMutex);
  return ret;
}








static void asyncEnable(int enable){
  if( enable && xOrigOpenReadWrite==0 ){
    assert(sqlite3Os.xOpenReadWrite);
    sqlite3HashInit(&async.aLock, SQLITE_HASH_BINARY, 1);
    xOrigOpenReadWrite = sqlite3Os.xOpenReadWrite;
    xOrigOpenReadOnly = sqlite3Os.xOpenReadOnly;
    xOrigOpenExclusive = sqlite3Os.xOpenExclusive;
    xOrigDelete = sqlite3Os.xDelete;
    xOrigFileExists = sqlite3Os.xFileExists;
    xOrigSyncDirectory = sqlite3Os.xSyncDirectory;

    sqlite3Os.xOpenReadWrite = asyncOpenReadWrite;
    sqlite3Os.xOpenReadOnly = asyncOpenReadOnly;
    sqlite3Os.xOpenExclusive = asyncOpenExclusive;
    sqlite3Os.xDelete = asyncDelete;
    sqlite3Os.xFileExists = asyncFileExists;
    sqlite3Os.xSyncDirectory = asyncSyncDirectory;
    assert(sqlite3Os.xOpenReadWrite);
  }
  if( !enable && xOrigOpenReadWrite!=0 ){
    assert(sqlite3Os.xOpenReadWrite);
    sqlite3HashClear(&async.aLock);
    sqlite3Os.xOpenReadWrite = xOrigOpenReadWrite;
    sqlite3Os.xOpenReadOnly = xOrigOpenReadOnly;
    sqlite3Os.xOpenExclusive = xOrigOpenExclusive;
    sqlite3Os.xDelete = xOrigDelete;
    sqlite3Os.xFileExists = xOrigFileExists;
    sqlite3Os.xSyncDirectory = xOrigSyncDirectory;

    xOrigOpenReadWrite = 0;
    xOrigOpenReadOnly = 0;
    xOrigOpenExclusive = 0;
    xOrigDelete = 0;
    xOrigFileExists = 0;
    xOrigSyncDirectory = 0;
    assert(sqlite3Os.xOpenReadWrite);
  }
}




















static void *asyncWriterThread(void *NotUsed){
  AsyncWrite *p = 0;
  int rc = SQLITE_OK;
  int holdingMutex = 0;

  if( pthread_mutex_trylock(&async.writerMutex) ){
    return 0;
  }
  while( async.writerHaltNow==0 ){
    OsFile *pBase = 0;

    if( !holdingMutex ){
      pthread_mutex_lock(&async.queueMutex);
    }
    while( (p = async.pQueueFirst)==0 ){
      pthread_cond_broadcast(&async.emptySignal);
      if( async.writerHaltWhenIdle ){
        pthread_mutex_unlock(&async.queueMutex);
        break;
      }else{
        TRACE(("IDLE\n"));
        pthread_cond_wait(&async.queueSignal, &async.queueMutex);
        TRACE(("WAKEUP\n"));
      }
    }
    if( p==0 ) break;
    holdingMutex = 1;

    


















    if( async.ioError!=SQLITE_OK && p->op!=ASYNC_CLOSE ){
      p->op = ASYNC_NOOP;
    }
    if( p->pFile ){
      pBase = p->pFile->pBaseWrite;
      if( 
        p->op==ASYNC_CLOSE || 
        p->op==ASYNC_OPENEXCLUSIVE ||
        (pBase && (p->op==ASYNC_SYNC || p->op==ASYNC_WRITE) ) 
      ){
        pthread_mutex_unlock(&async.queueMutex);
        holdingMutex = 0;
      }
      if( !pBase ){
        pBase = p->pFile->pBaseRead;
      }
    }

    switch( p->op ){
      case ASYNC_NOOP:
        break;

      case ASYNC_WRITE:
        assert( pBase );
        TRACE(("WRITE %s %d bytes at %d\n",
                p->pFile->zName, p->nByte, p->iOffset));
        rc = sqlite3OsSeek(pBase, p->iOffset);
        if( rc==SQLITE_OK ){
          rc = sqlite3OsWrite(pBase, (const void *)(p->zBuf), p->nByte);
        }
        break;

      case ASYNC_SYNC:
        assert( pBase );
        TRACE(("SYNC %s\n", p->pFile->zName));
        rc = sqlite3OsSync(pBase, p->nByte);
        break;

      case ASYNC_TRUNCATE:
        assert( pBase );
        TRACE(("TRUNCATE %s to %d bytes\n", p->pFile->zName, p->iOffset));
        rc = sqlite3OsTruncate(pBase, p->iOffset);
        break;

      case ASYNC_CLOSE:
        TRACE(("CLOSE %s\n", p->pFile->zName));
        sqlite3OsClose(&p->pFile->pBaseWrite);
        sqlite3OsClose(&p->pFile->pBaseRead);
        sqlite3OsFree(p->pFile);
        break;

      case ASYNC_OPENDIRECTORY:
        assert( pBase );
        TRACE(("OPENDIR %s\n", p->zBuf));
        sqlite3OsOpenDirectory(pBase, p->zBuf);
        break;

      case ASYNC_SETFULLSYNC:
        assert( pBase );
        TRACE(("SETFULLSYNC %s %d\n", p->pFile->zName, p->nByte));
        sqlite3OsSetFullSync(pBase, p->nByte);
        break;

      case ASYNC_DELETE:
        TRACE(("DELETE %s\n", p->zBuf));
        rc = xOrigDelete(p->zBuf);
        break;

      case ASYNC_SYNCDIRECTORY:
        TRACE(("SYNCDIR %s\n", p->zBuf));
        rc = xOrigSyncDirectory(p->zBuf);
        break;

      case ASYNC_OPENEXCLUSIVE: {
        AsyncFile *pFile = p->pFile;
        int delFlag = ((p->iOffset)?1:0);
        OsFile *pBase = 0;
        TRACE(("OPEN %s delFlag=%d\n", p->zBuf, delFlag));
        assert(pFile->pBaseRead==0 && pFile->pBaseWrite==0);
        rc = xOrigOpenExclusive(p->zBuf, &pBase, delFlag);
        assert( holdingMutex==0 );
        pthread_mutex_lock(&async.queueMutex);
        holdingMutex = 1;
        if( rc==SQLITE_OK ){
          pFile->pBaseRead = pBase;
        }
        break;
      }

      default: assert(!"Illegal value for AsyncWrite.op");
    }

    



    if( !holdingMutex ){
      pthread_mutex_lock(&async.queueMutex);
      holdingMutex = 1;
    }
    
    if( p==async.pQueueLast ){
      async.pQueueLast = 0;
    }
    async.pQueueFirst = p->pNext;
    sqlite3OsFree(p);
    assert( holdingMutex );

    

















    if( rc!=SQLITE_OK ){
      async.ioError = rc;
    }

    


    if( !async.pQueueFirst || !async.ioError ){
      sqlite3ApiExit(0, 0);
      pthread_mutex_unlock(&async.queueMutex);
      holdingMutex = 0;
      if( async.ioDelay>0 ){
        sqlite3OsSleep(async.ioDelay);
      }else{
        sched_yield();
      }
    }
  }
  
  pthread_mutex_unlock(&async.writerMutex);
  return 0;
}
















static int testAsyncEnable(
  void * clientData,
  Tcl_Interp *interp,
  int objc,
  Tcl_Obj *CONST objv[]
){
  if( objc!=1 && objc!=2 ){
    Tcl_WrongNumArgs(interp, 1, objv, "?YES/NO?");
    return TCL_ERROR;
  }
  if( objc==1 ){
    Tcl_SetObjResult(interp, Tcl_NewBooleanObj(xOrigOpenReadWrite!=0));
  }else{
    int en;
    if( Tcl_GetBooleanFromObj(interp, objv[1], &en) ) return TCL_ERROR;
    asyncEnable(en);
  }
  return TCL_OK;
}






static int testAsyncHalt(
  void * clientData,
  Tcl_Interp *interp,
  int objc,
  Tcl_Obj *CONST objv[]
){
  const char *zCond;
  if( objc!=2 ){
    Tcl_WrongNumArgs(interp, 1, objv, "\"now\"|\"idle\"|\"never\"");
    return TCL_ERROR;
  }
  zCond = Tcl_GetString(objv[1]);
  if( strcmp(zCond, "now")==0 ){
    async.writerHaltNow = 1;
    pthread_cond_broadcast(&async.queueSignal);
  }else if( strcmp(zCond, "idle")==0 ){
    async.writerHaltWhenIdle = 1;
    async.writerHaltNow = 0;
    pthread_cond_broadcast(&async.queueSignal);
  }else if( strcmp(zCond, "never")==0 ){
    async.writerHaltWhenIdle = 0;
    async.writerHaltNow = 0;
  }else{
    Tcl_AppendResult(interp, 
      "should be one of: \"now\", \"idle\", or \"never\"", (char*)0);
    return TCL_ERROR;
  }
  return TCL_OK;
}








static int testAsyncDelay(
  void * clientData,
  Tcl_Interp *interp,
  int objc,
  Tcl_Obj *CONST objv[]
){
  if( objc!=1 && objc!=2 ){
    Tcl_WrongNumArgs(interp, 1, objv, "?MS?");
    return TCL_ERROR;
  }
  if( objc==1 ){
    Tcl_SetObjResult(interp, Tcl_NewIntObj(async.ioDelay));
  }else{
    int ioDelay;
    if( Tcl_GetIntFromObj(interp, objv[1], &ioDelay) ) return TCL_ERROR;
    async.ioDelay = ioDelay;
  }
  return TCL_OK;
}






static int testAsyncStart(
  void * clientData,
  Tcl_Interp *interp,
  int objc,
  Tcl_Obj *CONST objv[]
){
  pthread_t x;
  int rc;
  rc = pthread_create(&x, 0, asyncWriterThread, 0);
  if( rc ){
    Tcl_AppendResult(interp, "failed to create the thread", 0);
    return TCL_ERROR;
  }
  pthread_detach(x);
  return TCL_OK;
}









static int testAsyncWait(
  void * clientData,
  Tcl_Interp *interp,
  int objc,
  Tcl_Obj *CONST objv[]
){
  int cnt = 10;
  if( async.writerHaltNow==0 && async.writerHaltWhenIdle==0 ){
    Tcl_AppendResult(interp, "would block forever", (char*)0);
    return TCL_ERROR;
  }

  while( cnt-- && !pthread_mutex_trylock(&async.writerMutex) ){
    pthread_mutex_unlock(&async.writerMutex);
    sched_yield();
  }
  if( cnt>=0 ){
    TRACE(("WAIT\n"));
    pthread_mutex_lock(&async.queueMutex);
    pthread_cond_broadcast(&async.queueSignal);
    pthread_mutex_unlock(&async.queueMutex);
    pthread_mutex_lock(&async.writerMutex);
    pthread_mutex_unlock(&async.writerMutex);
  }else{
    TRACE(("NO-WAIT\n"));
  }
  return TCL_OK;
}


#endif  






int Sqlitetestasync_Init(Tcl_Interp *interp){
#if OS_UNIX && THREADSAFE && defined(SQLITE_ENABLE_REDEF_IO)
  Tcl_CreateObjCommand(interp,"sqlite3async_enable",testAsyncEnable,0,0);
  Tcl_CreateObjCommand(interp,"sqlite3async_halt",testAsyncHalt,0,0);
  Tcl_CreateObjCommand(interp,"sqlite3async_delay",testAsyncDelay,0,0);
  Tcl_CreateObjCommand(interp,"sqlite3async_start",testAsyncStart,0,0);
  Tcl_CreateObjCommand(interp,"sqlite3async_wait",testAsyncWait,0,0);
  Tcl_LinkVar(interp, "sqlite3async_trace",
      (char*)&sqlite3async_trace, TCL_LINK_INT);
#endif  
  return TCL_OK;
}
