







































































































































































































































#include "mozStorageService.h"
#include "nsAutoLock.h"
#include "nsIConsoleService.h"
#include "nsIPrompt.h"
#include "nsIRunnable.h"
#include "nsIStringBundle.h"
#include "nsIThread.h"
#include "nsMemory.h"
#include "nsNetCID.h"
#include "nsProxyRelease.h"
#include "nsThreadUtils.h"
#include "nsXPCOMCIDInternal.h"
#include "plstr.h"
#include "prlock.h"
#include "prcvar.h"
#include "prtypes.h"

#include "sqlite3.h"
#include "sqlite3file.h"




#define SQLITE_ASYNC_TWO_FILEHANDLES 1























struct AsyncOsFile : public OsFile
{
  
  nsCString* mFilename;

  
  
  
  
  
  sqlite_int64 mOffset;

  
  
  
  PRBool mOpen;

  OsFile* mBaseRead;
  OsFile* mBaseWrite;
};














































struct AsyncMessage
{
  
  AsyncOsFile* mFile;

  
  PRUint32 mOp;            
  sqlite_int64 mOffset;    
  PRInt32 mBytes;          

  
  
  
  
  char *mBuf;

  
  AsyncMessage* mNext;
};

struct AsyncMessageBarrierData
{
  PRLock *mLock;
  PRCondVar *mCondVar;
};


#define ASYNC_WRITE         1
#define ASYNC_SYNC          2
#define ASYNC_TRUNCATE      3
#define ASYNC_CLOSE         4
#define ASYNC_OPENDIRECTORY 5
#define ASYNC_SETFULLSYNC   6
#define ASYNC_DELETE        7
#define ASYNC_OPENEXCLUSIVE 8
#define ASYNC_SYNCDIRECTORY 9
#define ASYNC_BARRIER       10


static int AsyncOpenReadWrite(const char *aName, OsFile **aFile, int *aReadOnly);
static int AsyncOpenExclusive(const char *aName, OsFile **aFile, int aDelFlag);
static int AsyncOpenReadOnly(const char *aName, OsFile **aFile);
static int AsyncDelete(const char* aName);
static int AsyncSyncDirectory(const char* aName);
static int AsyncFileExists(const char *aName);
static int AsyncClose(OsFile** aFile);
static int AsyncWrite(OsFile* aFile, const void* aBuf, int aCount);
static int AsyncTruncate(OsFile* aFile, sqlite_int64 aNumBytes);
static int AsyncOpenDirectory(OsFile* aFile, const char* aName);
static int AsyncSync(OsFile* aFile, int aFullsync);
static void AsyncSetFullSync(OsFile* aFile, int aValue);
static int AsyncRead(OsFile* aFile, void *aBuffer, int aCount);
static int AsyncSeek(OsFile* aFile, sqlite_int64 aOffset);
static int AsyncFileSize(OsFile* aFile, sqlite_int64* aSize);
static int AsyncFileHandle(OsFile* aFile);
static int AsyncLock(OsFile* aFile, int aLockType);
static int AsyncUnlock(OsFile* aFile, int aLockType);
static int AsyncCheckReservedLock(OsFile* aFile);
static int AsyncLockState(OsFile* aFile);

static int AsyncBarrier(PRLock* aLock, PRCondVar* aCondVar);


static int AsyncOpenFile(const char *aName, AsyncOsFile **aFile,
                     OsFile *aBaseRead, PRBool aOpenForWriting);


static AsyncMessage* AsyncQueueFirst = nsnull;
static AsyncMessage* AsyncQueueLast = nsnull;
#ifdef SINGLE_THREADED
  
  
  static PRBool AsyncWriterHaltWhenIdle = PR_TRUE;
#else
  static PRBool AsyncWriterHaltWhenIdle = PR_FALSE;
#endif
static void ProcessAsyncMessages();
static int ProcessOneMessage(AsyncMessage* aMessage);
static void AppendAsyncMessage(AsyncMessage* aMessage);
static int AppendNewAsyncMessage(AsyncOsFile* aFile, PRUint32 aOp,
                                 sqlite_int64 aOffset, PRInt32 aDataSize,
                                 const char *aData);
static int AsyncWriteError = SQLITE_OK; 
static void DisplayAsyncWriteError();




static nsIThread* AsyncWriteThreadInstance = nsnull;
static PRLock* AsyncQueueLock = nsnull;
static PRCondVar* AsyncQueueCondition = nsnull; 


static int (*sqliteOrigOpenReadWrite)(const char*, OsFile**, int*) = nsnull;
static int (*sqliteOrigOpenExclusive)(const char*, OsFile**, int) = nsnull;
static int (*sqliteOrigOpenReadOnly)(const char*, OsFile**) = nsnull;
static int (*sqliteOrigDelete)(const char*) = nsnull;
static int (*sqliteOrigFileExists)(const char*) = nsnull;
static int (*sqliteOrigSyncDirectory)(const char*) = nsnull;



static int (*sqliteOrigClose)(OsFile**) = nsnull;
static int (*sqliteOrigRead)(OsFile*, void*, int amt) = nsnull;
static int (*sqliteOrigWrite)(OsFile*, const void*, int amt) = nsnull;
static int (*sqliteOrigFileSize)(OsFile*, sqlite_int64 *pSize) = nsnull;
static int (*sqliteOrigSeek)(OsFile*, sqlite_int64 offset) = nsnull;
static int (*sqliteOrigSync)(OsFile*, int) = nsnull;
static int (*sqliteOrigTruncate)(OsFile*, sqlite_int64 size) = nsnull;
static int (*sqliteOrigOpenDirectory)(OsFile*, const char*);
static void (*sqliteOrigSetFullSync)(OsFile*, int setting);


#ifndef SINGLE_THREADED
class AsyncWriteThread : public nsIRunnable
{
public:
  AsyncWriteThread(mozIStorageService* aStorageService) :
    mStorageService(aStorageService) {}

  NS_DECL_ISUPPORTS

  NS_IMETHOD Run()
  {
    NS_ASSERTION(! AsyncWriterHaltWhenIdle, "You don't want halt on idle when starting up!");
    ProcessAsyncMessages();

    
    
    nsCOMPtr<nsIThread> mainThread;
    nsresult rv = NS_GetMainThread(getter_AddRefs(mainThread));
    if (NS_SUCCEEDED(rv)) {
      mozIStorageService* service = nsnull;
      mStorageService.swap(service);
      NS_ProxyRelease(mainThread, service);
    } else {
      NS_NOTREACHED("No event queue");
    }
    return NS_OK;
  }

protected:
  
  
  
  
  nsCOMPtr<mozIStorageService> mStorageService;
};
NS_IMPL_THREADSAFE_ISUPPORTS1(AsyncWriteThread, nsIRunnable)
#endif 







nsresult
mozStorageService::InitStorageAsyncIO()
{
  sqlite3OsVtbl* vtable = &sqlite3Os;

  sqliteOrigOpenReadWrite = vtable->xOpenReadWrite;
  sqliteOrigOpenReadOnly = vtable->xOpenReadOnly;
  sqliteOrigOpenExclusive = vtable->xOpenExclusive;
  sqliteOrigDelete = vtable->xDelete;
  sqliteOrigFileExists = vtable->xFileExists;
  sqliteOrigSyncDirectory = vtable->xSyncDirectory;

  vtable->xOpenReadWrite = AsyncOpenReadWrite;
  vtable->xOpenReadOnly = AsyncOpenReadOnly;
  vtable->xOpenExclusive = AsyncOpenExclusive;
  vtable->xDelete = AsyncDelete;
  vtable->xFileExists = AsyncFileExists;
  vtable->xSyncDirectory = AsyncSyncDirectory;

  
  AsyncQueueLock = PR_NewLock();
  if (! AsyncQueueLock) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  
  AsyncQueueCondition = PR_NewCondVar(AsyncQueueLock);
  if (! AsyncQueueCondition)
    return NS_ERROR_OUT_OF_MEMORY;

#ifndef SINGLE_THREADED
  
  nsCOMPtr<nsIRunnable> thread = new AsyncWriteThread(this);
  if (! thread)
    return NS_ERROR_OUT_OF_MEMORY;

  nsresult rv = NS_NewThread(&AsyncWriteThreadInstance, thread);
  if (NS_FAILED(rv)) {
    AsyncWriteThreadInstance = nsnull;
    return rv;
  }
#endif

  return NS_OK;
}








nsresult
mozStorageService::FlushAsyncIO()
{
  
  if (!AsyncWriteThreadInstance)
    return NS_OK;

  PRLock *flushLock = PR_NewLock();
  if (!flushLock)
    return NS_ERROR_OUT_OF_MEMORY;

  PRCondVar *flushCond = PR_NewCondVar(flushLock);
  if (!flushCond) {
    PR_DestroyLock(flushLock);
    return NS_ERROR_OUT_OF_MEMORY;
  }

  PR_Lock(flushLock);

  int rc = AsyncBarrier(flushLock, flushCond);
  if (rc == SQLITE_OK) {
    
    
    
    
    PR_WaitCondVar(flushCond, PR_INTERVAL_NO_TIMEOUT);
  }

  PR_Unlock(flushLock);

  PR_DestroyCondVar(flushCond);
  PR_DestroyLock(flushLock);

  if (rc == SQLITE_NOMEM)
    return NS_ERROR_OUT_OF_MEMORY;
  else if (rc != SQLITE_OK)
    return NS_ERROR_FAILURE;
  return NS_OK;
}












nsresult
mozStorageService::FinishAsyncIO()
{
  {
    nsAutoLock lock(AsyncQueueLock);

    if (!AsyncWriteThreadInstance)
      return NS_OK; 

    
    AsyncWriterHaltWhenIdle = PR_TRUE;

    
    PR_NotifyAllCondVar(AsyncQueueCondition);
  }

  
  AsyncWriteThreadInstance->Shutdown();

  
  NS_RELEASE(AsyncWriteThreadInstance);
  AsyncWriteThreadInstance = nsnull;

  return NS_OK;
}








void
mozStorageService::FreeLocks()
{
  
  if (AsyncQueueCondition) {
    PR_DestroyCondVar(AsyncQueueCondition);
    AsyncQueueCondition = nsnull;
  }

  if (AsyncQueueLock) {
    PR_DestroyLock(AsyncQueueLock);
    AsyncQueueLock = nsnull;
  }
}












int
AsyncOpenFile(const char* aName, AsyncOsFile** aFile,
                                 OsFile* aBaseRead, PRBool aOpenForWriting)
{
  int rc;
  OsFile *baseWrite = nsnull;

  if (! sqliteOrigClose) {
    sqliteOrigClose = aBaseRead->pMethod->xClose;
    sqliteOrigRead = aBaseRead->pMethod->xRead;
    sqliteOrigWrite = aBaseRead->pMethod->xWrite;
    sqliteOrigFileSize = aBaseRead->pMethod->xFileSize;
    sqliteOrigSeek = aBaseRead->pMethod->xSeek;
    sqliteOrigSync = aBaseRead->pMethod->xSync;
    sqliteOrigTruncate = aBaseRead->pMethod->xTruncate;
    sqliteOrigOpenDirectory = aBaseRead->pMethod->xOpenDirectory;
    sqliteOrigSetFullSync = aBaseRead->pMethod->xSetFullSync;
  }

  static IoMethod iomethod = {
    AsyncClose,
    AsyncOpenDirectory,
    AsyncRead,
    AsyncWrite,
    AsyncSeek,
    AsyncTruncate,
    AsyncSync,
    AsyncSetFullSync,
    AsyncFileHandle,
    AsyncFileSize,
    AsyncLock,
    AsyncUnlock,
    AsyncLockState,
    AsyncCheckReservedLock
  };

  if (aOpenForWriting && SQLITE_ASYNC_TWO_FILEHANDLES) {
    int dummy;
    rc = sqliteOrigOpenReadWrite(aName, &baseWrite, &dummy);
    if (rc != SQLITE_OK)
      goto error_out;
  }

  *aFile = static_cast<AsyncOsFile*>(nsMemory::Alloc(sizeof(AsyncOsFile)));
  if (! *aFile) {
    rc = SQLITE_NOMEM;
    goto error_out;
  }
  memset(*aFile, 0, sizeof(AsyncOsFile));

  (*aFile)->mFilename = new nsCString(aName);
  (*aFile)->pMethod = &iomethod;
  (*aFile)->mOpen = PR_TRUE;
  (*aFile)->mBaseRead = aBaseRead;
  (*aFile)->mBaseWrite = baseWrite;

  return SQLITE_OK;

error_out:
  NS_ASSERTION(!*aFile, "File not cleared on error");
  sqliteOrigClose(&aBaseRead);
  sqliteOrigClose(&baseWrite);
  return rc;
}













void
AppendAsyncMessage(AsyncMessage* aMessage)
{
  
  PR_Lock(AsyncQueueLock);

  
  NS_ASSERTION(! aMessage->mNext, "New messages should not have next pointers");
  if (AsyncQueueLast) {
    NS_ASSERTION(AsyncQueueFirst, "If we have a last item, we need to have a first one");
    AsyncQueueLast->mNext = aMessage;
  } else {
    AsyncQueueFirst = aMessage;
  }
  AsyncQueueLast = aMessage;

  
  
  if (AsyncWriteThreadInstance) {
    PR_NotifyCondVar(AsyncQueueCondition);
    PR_Unlock(AsyncQueueLock);
  } else {
    
    NS_ASSERTION(AsyncWriterHaltWhenIdle, "In single-threaded mode, the writer thread should always halt when idle");
    PR_Unlock(AsyncQueueLock);
    ProcessAsyncMessages();
  }
}










int 
AppendNewAsyncMessage(AsyncOsFile* aFile, PRUint32 aOp,
                                         sqlite_int64 aOffset, PRInt32 aDataSize,
                                         const char *aData)
{
  
  AsyncMessage* p = static_cast<AsyncMessage*>
                               (nsMemory::Alloc(sizeof(AsyncMessage) + (aData ? aDataSize : 0)));
  if (! p)
    return SQLITE_NOMEM;

  p->mOp = aOp;
  p->mOffset = aOffset;
  p->mBytes = aDataSize;
  p->mFile = aFile;
  p->mNext = nsnull;
  if (aData) {
    
    p->mBuf = (char*)&p[1];
    memcpy(p->mBuf, aData, aDataSize);
  } else {
    p->mBuf = nsnull;
  }
  AppendAsyncMessage(p);
  return SQLITE_OK;
}























int 
AsyncOpenExclusive(const char* aName, OsFile** aFile,
                                      int aDelFlag)
{
  if (AsyncWriteError != SQLITE_OK)
    return AsyncWriteError;
  
  
  AsyncOsFile* osfile;
  int rc = AsyncOpenFile(aName, &osfile, nsnull, PR_FALSE);
  if (rc != SQLITE_OK)
    return rc;

  rc = AppendNewAsyncMessage(osfile, ASYNC_OPENEXCLUSIVE, aDelFlag,
                             PL_strlen(aName) + 1, aName);
  if (rc != SQLITE_OK) {
    nsMemory::Free(osfile);
    osfile = nsnull;
  }
  *aFile = osfile;
  return rc;
}




int 
AsyncOpenReadOnly(const char* aName, OsFile** aFile)
{
  if (AsyncWriteError != SQLITE_OK)
    return AsyncWriteError;
  OsFile* base = nsnull;
  int rc = sqliteOrigOpenReadOnly(aName, &base);
  if (rc == SQLITE_OK) {
    AsyncOsFile* asyncfile;
    rc = AsyncOpenFile(aName, &asyncfile, base, PR_FALSE);
    if (rc == SQLITE_OK)
      *aFile = asyncfile;
    else
      *aFile = nsnull;
  }
  return rc;
}




int 
AsyncOpenReadWrite(const char *aName, OsFile** aFile,
                                      int* aReadOnly)
{
  if (AsyncWriteError != SQLITE_OK)
    return AsyncWriteError;
  OsFile* base = nsnull;
  int rc = sqliteOrigOpenReadWrite(aName, &base, aReadOnly);
  if (rc == SQLITE_OK) {
    AsyncOsFile* asyncfile;
    rc = AsyncOpenFile(aName, &asyncfile, base, !(*aReadOnly));
    if (rc == SQLITE_OK)
      *aFile = asyncfile;
    else
      *aFile = nsnull;
  }
  return rc;
}







int 
AsyncDelete(const char* aName)
{
  if (AsyncWriteError != SQLITE_OK)
    return AsyncWriteError;
  return AppendNewAsyncMessage(0, ASYNC_DELETE, 0, PL_strlen(aName) + 1, aName);
}







int 
AsyncSyncDirectory(const char* aName)
{
  if (AsyncWriteError != SQLITE_OK)
    return AsyncWriteError;
  return AppendNewAsyncMessage(0, ASYNC_SYNCDIRECTORY, 0, strlen(aName) + 1, aName);
}














int 
AsyncFileExists(const char *aName)
{
  if (AsyncWriteError != SQLITE_OK)
    return AsyncWriteError;
  nsAutoLock lock(AsyncQueueLock);

  
  int ret = sqliteOrigFileExists(aName);

  for (AsyncMessage* p = AsyncQueueFirst; p != nsnull; p = p->mNext) {
    if (p->mOp == ASYNC_DELETE && 0 == strcmp(p->mBuf, aName)) {
      ret = 0;
    } else if (p->mOp == ASYNC_OPENEXCLUSIVE && 0 == strcmp(p->mBuf, aName)) {
      ret = 1;
    }
  }
  return ret;
}










int 
AsyncClose(OsFile** aFile)
{
  if (AsyncWriteError != SQLITE_OK)
    return AsyncWriteError;
  AsyncOsFile* asyncfile = static_cast<AsyncOsFile*>(*aFile);
  if (! asyncfile->mOpen) {
    NS_NOTREACHED("Attempting to write to a file with a close pending!");
    return SQLITE_INTERNAL;
  }
  asyncfile->mOpen = PR_FALSE;
  return AppendNewAsyncMessage(asyncfile, ASYNC_CLOSE, 0, 0, 0);
}









int 
AsyncWrite(OsFile* aFile, const void* aBuf, int aCount)
{
  if (AsyncWriteError != SQLITE_OK)
    return AsyncWriteError;
  AsyncOsFile* asyncfile = static_cast<AsyncOsFile*>(aFile);
  if (! asyncfile->mOpen) {
    NS_NOTREACHED("Attempting to write to a file with a close pending!");
    return SQLITE_INTERNAL;
  }
  int rc = AppendNewAsyncMessage(asyncfile, ASYNC_WRITE, asyncfile->mOffset,
                                 aCount, static_cast<const char*>(aBuf));
  asyncfile->mOffset += aCount;
  return rc;
}







int 
AsyncTruncate(OsFile* aFile, sqlite_int64 aNumBytes)
{
  if (AsyncWriteError != SQLITE_OK)
    return AsyncWriteError;
  AsyncOsFile* asyncfile = static_cast<AsyncOsFile*>(aFile);
  if (! asyncfile->mOpen) {
    NS_NOTREACHED("Attempting to write to a file with a close pending!");
    return SQLITE_INTERNAL;
  }
  return AppendNewAsyncMessage(asyncfile, ASYNC_TRUNCATE, aNumBytes, 0, 0);
}








int 
AsyncOpenDirectory(OsFile* aFile, const char* aName)
{
  if (AsyncWriteError != SQLITE_OK)
    return AsyncWriteError;
  AsyncOsFile* asyncfile = static_cast<AsyncOsFile*>(aFile);
  if (! asyncfile->mOpen) {
    NS_NOTREACHED("Attempting to write to a file with a close pending!");
    return SQLITE_INTERNAL;
  }
  return AppendNewAsyncMessage(asyncfile, ASYNC_OPENDIRECTORY, 0,
                          strlen(aName) + 1, aName);
}







int 
AsyncSync(OsFile* aFile, int aFullsync)
{
  if (AsyncWriteError != SQLITE_OK)
    return AsyncWriteError;
  AsyncOsFile* asyncfile = static_cast<AsyncOsFile*>(aFile);
  if (! asyncfile->mOpen) {
    NS_NOTREACHED("Attempting to write to a file with a close pending!");
    return SQLITE_INTERNAL;
  }
  return AppendNewAsyncMessage(asyncfile, ASYNC_SYNC, 0, aFullsync, 0);
}







void 
AsyncSetFullSync(OsFile* aFile, int aValue)
{
  if (AsyncWriteError != SQLITE_OK)
    return;
  AsyncOsFile* asyncfile = static_cast<AsyncOsFile*>(aFile);
  if (! asyncfile->mOpen) {
    NS_NOTREACHED("Attempting to write to a file with a close pending!");
    return;
  }
  AppendNewAsyncMessage(asyncfile, ASYNC_SETFULLSYNC, 0, aValue, 0);
}











int 
AsyncRead(OsFile* aFile, void *aBuffer, int aCount)
{
  if (AsyncWriteError != SQLITE_OK)
    return AsyncWriteError;
  int rc = SQLITE_OK;

  
  
  
  nsAutoLock lock(AsyncQueueLock);

  AsyncOsFile* asyncfile = static_cast<AsyncOsFile*>(aFile);
  if (! asyncfile->mOpen) {
    NS_NOTREACHED("Attempting to write to a file with a close pending!");
    return SQLITE_INTERNAL;
  }

  OsFile* pBase = asyncfile->mBaseRead;
  if (pBase) {
    
    
    
    
    
    
    sqlite_int64 filesize;
    NS_ASSERTION(sqliteOrigFileSize, "Original file size pointer uninitialized!");
    rc = sqliteOrigFileSize(pBase, &filesize);
    if (rc != SQLITE_OK)
      goto asyncread_out;

    
    
    
    NS_ASSERTION(sqliteOrigSeek, "Original seek pointer uninitialized!");
    rc = sqliteOrigSeek(pBase, asyncfile->mOffset);
    if (rc != SQLITE_OK)
      goto asyncread_out;

    
    int numread = PR_MIN(filesize - asyncfile->mOffset, aCount);
    if (numread > 0) {
      NS_ASSERTION(pBase, "Original read pointer uninitialized!");
      rc = sqliteOrigRead(pBase, aBuffer, numread);
    }
  }

  if (rc == SQLITE_OK) {
    sqlite_int64 blockOffset = asyncfile->mOffset; 

    
    for (AsyncMessage* p = AsyncQueueFirst; p != nsnull; p = p->mNext) {
      if (p->mFile == asyncfile && p->mOp == ASYNC_WRITE) {

        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        PRInt32 beginIn = PR_MAX(0, blockOffset - p->mOffset);
        PRInt32 beginOut = PR_MAX(0, p->mOffset - blockOffset);
        PRInt32 copycount = PR_MIN(p->mBytes - beginIn, aCount - beginOut);

        if (copycount > 0) {
          memcpy(&static_cast<char*>(aBuffer)[beginOut],
                 &p->mBuf[beginIn], copycount);
        }
      }
    }

    
    asyncfile->mOffset += aCount;
  }

asyncread_out:
  return rc;
}








int 
AsyncSeek(OsFile* aFile, sqlite_int64 aOffset)
{
  if (AsyncWriteError != SQLITE_OK)
    return AsyncWriteError;
  AsyncOsFile* asyncfile = static_cast<AsyncOsFile*>(aFile);
  if (! asyncfile->mOpen) {
    NS_NOTREACHED("Attempting to write to a file with a close pending!");
    return SQLITE_INTERNAL;
  }
  asyncfile->mOffset = aOffset;
  return SQLITE_OK;
}











int 
AsyncFileSize(OsFile* aFile, sqlite_int64* aSize)
{
  nsAutoLock lock(AsyncQueueLock);

  if (AsyncWriteError != SQLITE_OK)
    return AsyncWriteError;

  AsyncOsFile* asyncfile = static_cast<AsyncOsFile*>(aFile);
  if (! asyncfile->mOpen) {
    NS_NOTREACHED("Attempting to write to a file with a close pending!");
    return SQLITE_INTERNAL;
  }
  int rc = SQLITE_OK;
  sqlite_int64 size = 0;

  
  
  
  
  OsFile* pBase = asyncfile->mBaseRead;
  if (pBase) {
    NS_ASSERTION(sqliteOrigFileSize, "Original file size pointer uninitialized!");
    rc = sqliteOrigFileSize(pBase, &size);
  }

  if (rc == SQLITE_OK) {
    for (AsyncMessage* p = AsyncQueueFirst; p != nsnull; p = p->mNext) {
      if (p->mFile == asyncfile) {
        switch (p->mOp) {
          case ASYNC_WRITE:
            size = PR_MAX(p->mOffset + p->mBytes, size);
            break;
          case ASYNC_TRUNCATE:
            size = PR_MIN(size, p->mOffset);
            break;
        }
      }
    }
    *aSize = size;
  }
  return rc;
}








int 
AsyncFileHandle(OsFile* aFile)
{
  if (AsyncWriteError != SQLITE_OK)
    return AsyncWriteError;
  NS_NOTREACHED("Don't call FileHandle in async mode");
  return SQLITE_OK;

  
  
  
}







int 
AsyncLock(OsFile* aFile, int aLockType)
{
  if (AsyncWriteError != SQLITE_OK)
    return AsyncWriteError;
  return SQLITE_OK;
}




int 
AsyncUnlock(OsFile* aFile, int aLockType)
{
  if (AsyncWriteError != SQLITE_OK)
    return AsyncWriteError;
  return SQLITE_OK;
}







int 
AsyncCheckReservedLock(OsFile* aFile)
{
  if (AsyncWriteError != SQLITE_OK)
    return AsyncWriteError;
  return SQLITE_OK;
}







int 
AsyncLockState(OsFile* aFile)
{
  if (AsyncWriteError != SQLITE_OK)
    return AsyncWriteError;
  NS_NOTREACHED("Don't call LockState in async mode");
  return SQLITE_OK;
}







int 
AsyncBarrier(PRLock* aLock, PRCondVar* aCondVar)
{
  AsyncMessageBarrierData bd;

  bd.mLock = aLock;
  bd.mCondVar = aCondVar;

  return AppendNewAsyncMessage(nsnull, ASYNC_BARRIER, 0,
                               sizeof(AsyncMessageBarrierData), (const char*) &bd);
}




















int 
ProcessOneMessage(AsyncMessage* aMessage)
{
  PRBool regainMutex = PR_FALSE;
  OsFile* pBase = nsnull;

  if (aMessage->mFile) {
    pBase = aMessage->mFile->mBaseWrite;
    if (aMessage->mOp == ASYNC_CLOSE || 
        aMessage->mOp == ASYNC_OPENEXCLUSIVE ||
        (pBase && (aMessage->mOp == ASYNC_SYNC ||
                   aMessage->mOp == ASYNC_WRITE))) {
      regainMutex = PR_TRUE;
      PR_Unlock(AsyncQueueLock);
    }
    if (! pBase)
      pBase = aMessage->mFile->mBaseRead;
  }

  int rc = SQLITE_OK;
  switch (aMessage->mOp) {
    case ASYNC_WRITE:
      NS_ASSERTION(pBase, "Must have base writer for writing");
      rc = sqliteOrigSeek(pBase, aMessage->mOffset);
      if (rc == SQLITE_OK)
        rc = sqliteOrigWrite(pBase, (const void *)(aMessage->mBuf), aMessage->mBytes);
      break;

    case ASYNC_SYNC:
      NS_ASSERTION(pBase, "Must have base writer for writing");
      rc = sqliteOrigSync(pBase, aMessage->mBytes);
      break;

    case ASYNC_TRUNCATE:
      NS_ASSERTION(pBase, "Must have base writer for writing");
      NS_ASSERTION(sqliteOrigTruncate, "No truncate pointer");
      rc = sqliteOrigTruncate(pBase, aMessage->mOffset);
      break;

    case ASYNC_CLOSE:
      
      
      
      sqliteOrigClose(&aMessage->mFile->mBaseWrite);
      sqliteOrigClose(&aMessage->mFile->mBaseRead);
      if (aMessage->mFile->mFilename)
        delete aMessage->mFile->mFilename;
      nsMemory::Free(aMessage->mFile);
      aMessage->mFile = nsnull;
      break;

    case ASYNC_OPENDIRECTORY:
      NS_ASSERTION(pBase, "Must have base writer for writing");
      NS_ASSERTION(sqliteOrigOpenDirectory, "No open directory pointer");
      sqliteOrigOpenDirectory(pBase, aMessage->mBuf);
      break;

    case ASYNC_SETFULLSYNC:
      NS_ASSERTION(pBase, "Must have base writer for writing");
      sqliteOrigSetFullSync(pBase, aMessage->mBytes);
      break;

    case ASYNC_DELETE:
      NS_ASSERTION(sqliteOrigDelete, "No delete pointer");
      rc = sqliteOrigDelete(aMessage->mBuf);
#ifdef XP_WIN
      if (SQLITE_IOERR == rc) {
        NS_WARNING("SQLite returned an error when trying to delete.  "
                   "See http://www.sqlite.org/cvstrac/tktview?tn=2441.  "
                   "This warning is safe to ignore on shutdown.");
        
        rc = SQLITE_OK;
      }
#endif
      break;

    case ASYNC_SYNCDIRECTORY:
      NS_ASSERTION(sqliteOrigSyncDirectory, "No sync directory pointer");
      rc = sqliteOrigSyncDirectory(aMessage->mBuf);
      break;

    case ASYNC_OPENEXCLUSIVE: {
      AsyncOsFile *pFile = aMessage->mFile;
      int delFlag = ((aMessage->mOffset) ? 1 : 0);
      OsFile* pBase = nsnull;
      NS_ASSERTION(! pFile->mBaseRead && ! pFile->mBaseWrite,
                   "OpenExclusive expects no file pointers");
      rc = sqliteOrigOpenExclusive(aMessage->mBuf, &pBase, delFlag);

      
      
      
      PR_Lock(AsyncQueueLock);
      regainMutex = PR_FALSE;
      if (rc == SQLITE_OK)
        pFile->mBaseRead = pBase;
      break;
    }

    case ASYNC_BARRIER: {
      AsyncMessageBarrierData *bd = (AsyncMessageBarrierData*) aMessage->mBuf;
      PR_Lock(bd->mLock);
      PR_NotifyCondVar(bd->mCondVar);
      PR_Unlock(bd->mLock);
      break;
    }

    default:
      NS_NOTREACHED("Illegal value for AsyncMessage.mOp");
  }

  if (regainMutex) {
    PR_Lock(AsyncQueueLock);
  }
  return rc;
}






















void 
ProcessAsyncMessages()
{
  AsyncMessage *message = 0;
  int rc = SQLITE_OK;

  while (PR_TRUE) {
    {
      
      nsAutoLock lock(AsyncQueueLock);
      while ((message = AsyncQueueFirst) == 0) {
        if (AsyncWriterHaltWhenIdle) {
          
          return;
        } else {
          
          
          
          NS_ASSERTION(AsyncQueueLock, "We need to be in multi threaded mode if we're going to wait");
          PR_WaitCondVar(AsyncQueueCondition, PR_INTERVAL_NO_TIMEOUT);
        }
      }

      
      
      rc = ProcessOneMessage(message);

      
      if (rc != SQLITE_OK) {
        AsyncWriteError = rc;

        nsAutoString logMessage;
        logMessage.AssignLiteral("mozStorage: error code ");
        logMessage.AppendInt(rc);
        logMessage.AppendLiteral(" for database ");
        if (message->mFile && message->mFile->mFilename)
          logMessage.Append(NS_ConvertUTF8toUTF16(*message->mFile->mFilename));

#ifdef DEBUG
        printf("%s\n", NS_ConvertUTF16toUTF8(logMessage).get());
#endif
          
        
        nsresult rv;
        nsCOMPtr<nsIConsoleService> consoleSvc =
            do_GetService("@mozilla.org/consoleservice;1", &rv);
        if (NS_FAILED(rv)) {
          NS_WARNING("Couldn't get the console service for logging file error");
        } else {
          rv = consoleSvc->LogStringMessage(logMessage.get());
          NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Couldn't log message on async error");
        }

        NS_NOTREACHED("FILE ERROR");

        
        DisplayAsyncWriteError();
        return;
      }

      
      if (message == AsyncQueueLast)
        AsyncQueueLast = nsnull;
      AsyncQueueFirst = message->mNext;
      nsMemory::Free(message);

      
      sqlite3ApiExit(nsnull, 0);
    }
    
    
    
    
    #ifdef IO_DELAY_INTERVAL_MS
      
      PR_Sleep(PR_MillisecondsToInterval(IO_DELAY_INTERVAL_MS));
    #else
      
      PR_Sleep(PR_INTERVAL_NO_WAIT);
    #endif
  }
}







class nsAsyncWriteErrorDisplayer : public nsRunnable
{
public:
  NS_IMETHOD Run()
  {
    nsresult rv;
    nsCOMPtr<nsIPrompt> prompt = do_CreateInstance(
        NS_DEFAULTPROMPT_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIStringBundleService> bundleService = do_GetService(
        "@mozilla.org/intl/stringbundle;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIStringBundle> bundle;
    rv = bundleService->CreateBundle(
        "chrome://global/locale/storage.properties", getter_AddRefs(bundle));
    NS_ENSURE_SUCCESS(rv, rv);

    nsXPIDLString message;
    rv = bundle->GetStringFromName(NS_LITERAL_STRING("storageWriteError").get(),
                                   getter_Copies(message));
    NS_ENSURE_SUCCESS(rv, rv);

    return prompt->Alert(nsnull, message.get());
  }
};








void
DisplayAsyncWriteError()
{
  nsCOMPtr<nsIRunnable> displayer(new nsAsyncWriteErrorDisplayer);
  if (! displayer) {
    NS_WARNING("Unable to create displayer");
    return;
  }
  nsresult rv = NS_DispatchToMainThread(displayer);
  NS_ASSERTION(NS_SUCCEEDED(rv), "Can't call main thread");
}
