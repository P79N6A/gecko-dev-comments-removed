





#include <string.h>
#include "mozilla/Telemetry.h"
#include "mozilla/Preferences.h"
#include "sqlite3.h"
#include "nsThreadUtils.h"
#include "mozilla/dom/quota/PersistenceType.h"
#include "mozilla/dom/quota/QuotaManager.h"
#include "mozilla/dom/quota/QuotaObject.h"
#include "mozilla/IOInterposer.h"


#define LAST_KNOWN_VFS_VERSION 3


#define LAST_KNOWN_IOMETHODS_VERSION 3









#define PREF_NFS_FILESYSTEM   "storage.nfs_filesystem"

namespace {

using namespace mozilla;
using namespace mozilla::dom::quota;

struct Histograms {
  const char *name;
  const Telemetry::ID readB;
  const Telemetry::ID writeB;
  const Telemetry::ID readMS;
  const Telemetry::ID writeMS;
  const Telemetry::ID syncMS;
};

#define SQLITE_TELEMETRY(FILENAME, HGRAM) \
  { FILENAME, \
    Telemetry::MOZ_SQLITE_ ## HGRAM ## _READ_B, \
    Telemetry::MOZ_SQLITE_ ## HGRAM ## _WRITE_B, \
    Telemetry::MOZ_SQLITE_ ## HGRAM ## _READ_MS, \
    Telemetry::MOZ_SQLITE_ ## HGRAM ## _WRITE_MS, \
    Telemetry::MOZ_SQLITE_ ## HGRAM ## _SYNC_MS \
  }

Histograms gHistograms[] = {
  SQLITE_TELEMETRY("places.sqlite", PLACES),
  SQLITE_TELEMETRY("cookies.sqlite", COOKIES),
  SQLITE_TELEMETRY("webappsstore.sqlite", WEBAPPS),
  SQLITE_TELEMETRY(nullptr, OTHER)
};
#undef SQLITE_TELEMETRY



class IOThreadAutoTimer {
public:
  












  explicit IOThreadAutoTimer(Telemetry::ID id,
    IOInterposeObserver::Operation aOp = IOInterposeObserver::OpNone)
    : start(TimeStamp::Now()),
      id(id),
      op(aOp)
  {
  }

  





  explicit IOThreadAutoTimer(IOInterposeObserver::Operation aOp)
    : start(TimeStamp::Now()),
      id(Telemetry::HistogramCount),
      op(aOp)
  {
  }

  ~IOThreadAutoTimer()
  {
    TimeStamp end(TimeStamp::Now());
    uint32_t mainThread = NS_IsMainThread() ? 1 : 0;
    if (id != Telemetry::HistogramCount) {
      Telemetry::AccumulateTimeDelta(static_cast<Telemetry::ID>(id + mainThread),
                                     start, end);
    }
    
    
    
#if defined(MOZ_ENABLE_PROFILER_SPS) && !defined(XP_WIN)
    if (IOInterposer::IsObservedOperation(op)) {
      const char* main_ref  = "sqlite-mainthread";
      const char* other_ref = "sqlite-otherthread";

      
      IOInterposeObserver::Observation ob(op, start, end,
                                          (mainThread ? main_ref : other_ref));
      
      IOInterposer::Report(ob);
    }
#endif 
  }

private:
  const TimeStamp start;
  const Telemetry::ID id;
  IOInterposeObserver::Operation op;
};

struct telemetry_file {
  
  sqlite3_file base;

  
  Histograms *histograms;

  
  nsRefPtr<QuotaObject> quotaObject;

  
  sqlite3_file pReal[1];
};




int
xClose(sqlite3_file *pFile)
{
  telemetry_file *p = (telemetry_file *)pFile;
  int rc;
  { 
    IOThreadAutoTimer ioTimer(IOInterposeObserver::OpClose);
    rc = p->pReal->pMethods->xClose(p->pReal);
  }
  if( rc==SQLITE_OK ){
    delete p->base.pMethods;
    p->base.pMethods = nullptr;
    p->quotaObject = nullptr;
  }
  return rc;
}




int
xRead(sqlite3_file *pFile, void *zBuf, int iAmt, sqlite_int64 iOfst)
{
  telemetry_file *p = (telemetry_file *)pFile;
  IOThreadAutoTimer ioTimer(p->histograms->readMS, IOInterposeObserver::OpRead);
  int rc;
  rc = p->pReal->pMethods->xRead(p->pReal, zBuf, iAmt, iOfst);
  
  if (rc != SQLITE_IOERR_SHORT_READ)
    Telemetry::Accumulate(p->histograms->readB, rc == SQLITE_OK ? iAmt : 0);
  return rc;
}




int
xWrite(sqlite3_file *pFile, const void *zBuf, int iAmt, sqlite_int64 iOfst)
{
  telemetry_file *p = (telemetry_file *)pFile;
  if (p->quotaObject && !p->quotaObject->MaybeAllocateMoreSpace(iOfst, iAmt)) {
    return SQLITE_FULL;
  }
  IOThreadAutoTimer ioTimer(p->histograms->writeMS, IOInterposeObserver::OpWrite);
  int rc;
  rc = p->pReal->pMethods->xWrite(p->pReal, zBuf, iAmt, iOfst);
  Telemetry::Accumulate(p->histograms->writeB, rc == SQLITE_OK ? iAmt : 0);
  return rc;
}




int
xTruncate(sqlite3_file *pFile, sqlite_int64 size)
{
  IOThreadAutoTimer ioTimer(Telemetry::MOZ_SQLITE_TRUNCATE_MS);
  telemetry_file *p = (telemetry_file *)pFile;
  int rc;
  Telemetry::AutoTimer<Telemetry::MOZ_SQLITE_TRUNCATE_MS> timer;
  rc = p->pReal->pMethods->xTruncate(p->pReal, size);
  if (rc == SQLITE_OK && p->quotaObject) {
    p->quotaObject->UpdateSize(size);
  }
  return rc;
}




int
xSync(sqlite3_file *pFile, int flags)
{
  telemetry_file *p = (telemetry_file *)pFile;
  IOThreadAutoTimer ioTimer(p->histograms->syncMS, IOInterposeObserver::OpFSync);
  return p->pReal->pMethods->xSync(p->pReal, flags);
}




int
xFileSize(sqlite3_file *pFile, sqlite_int64 *pSize)
{
  IOThreadAutoTimer ioTimer(IOInterposeObserver::OpStat);
  telemetry_file *p = (telemetry_file *)pFile;
  int rc;
  rc = p->pReal->pMethods->xFileSize(p->pReal, pSize);
  return rc;
}




int
xLock(sqlite3_file *pFile, int eLock)
{
  telemetry_file *p = (telemetry_file *)pFile;
  int rc;
  rc = p->pReal->pMethods->xLock(p->pReal, eLock);
  return rc;
}




int
xUnlock(sqlite3_file *pFile, int eLock)
{
  telemetry_file *p = (telemetry_file *)pFile;
  int rc;
  rc = p->pReal->pMethods->xUnlock(p->pReal, eLock);
  return rc;
}




int
xCheckReservedLock(sqlite3_file *pFile, int *pResOut)
{
  telemetry_file *p = (telemetry_file *)pFile;
  int rc = p->pReal->pMethods->xCheckReservedLock(p->pReal, pResOut);
  return rc;
}




int
xFileControl(sqlite3_file *pFile, int op, void *pArg)
{
  telemetry_file *p = (telemetry_file *)pFile;
  int rc = p->pReal->pMethods->xFileControl(p->pReal, op, pArg);
  return rc;
}




int
xSectorSize(sqlite3_file *pFile)
{
  telemetry_file *p = (telemetry_file *)pFile;
  int rc;
  rc = p->pReal->pMethods->xSectorSize(p->pReal);
  return rc;
}




int
xDeviceCharacteristics(sqlite3_file *pFile)
{
  telemetry_file *p = (telemetry_file *)pFile;
  int rc;
  rc = p->pReal->pMethods->xDeviceCharacteristics(p->pReal);
  return rc;
}




int
xShmLock(sqlite3_file *pFile, int ofst, int n, int flags)
{
  telemetry_file *p = (telemetry_file *)pFile;
  return p->pReal->pMethods->xShmLock(p->pReal, ofst, n, flags);
}

int
xShmMap(sqlite3_file *pFile, int iRegion, int szRegion, int isWrite, void volatile **pp)
{
  telemetry_file *p = (telemetry_file *)pFile;
  int rc;
  rc = p->pReal->pMethods->xShmMap(p->pReal, iRegion, szRegion, isWrite, pp);
  return rc;
}

void
xShmBarrier(sqlite3_file *pFile){
  telemetry_file *p = (telemetry_file *)pFile;
  p->pReal->pMethods->xShmBarrier(p->pReal);
}

int
xShmUnmap(sqlite3_file *pFile, int delFlag){
  telemetry_file *p = (telemetry_file *)pFile;
  int rc;
  rc = p->pReal->pMethods->xShmUnmap(p->pReal, delFlag);
  return rc;
}

int
xFetch(sqlite3_file *pFile, sqlite3_int64 iOff, int iAmt, void **pp)
{
  telemetry_file *p = (telemetry_file *)pFile;
  MOZ_ASSERT(p->pReal->pMethods->iVersion >= 3);
  return p->pReal->pMethods->xFetch(p->pReal, iOff, iAmt, pp);
}

int
xUnfetch(sqlite3_file *pFile, sqlite3_int64 iOff, void *pResOut)
{
  telemetry_file *p = (telemetry_file *)pFile;
  MOZ_ASSERT(p->pReal->pMethods->iVersion >= 3);
  return p->pReal->pMethods->xUnfetch(p->pReal, iOff, pResOut);
}

int
xOpen(sqlite3_vfs* vfs, const char *zName, sqlite3_file* pFile,
          int flags, int *pOutFlags)
{
  IOThreadAutoTimer ioTimer(Telemetry::MOZ_SQLITE_OPEN_MS,
                            IOInterposeObserver::OpCreateOrOpen);
  Telemetry::AutoTimer<Telemetry::MOZ_SQLITE_OPEN_MS> timer;
  sqlite3_vfs *orig_vfs = static_cast<sqlite3_vfs*>(vfs->pAppData);
  int rc;
  telemetry_file *p = (telemetry_file *)pFile;
  Histograms *h = nullptr;
  
  for(size_t i = 0;i < sizeof(gHistograms)/sizeof(gHistograms[0]);i++) {
    h = &gHistograms[i];
    
    if (!h->name)
      break;
    if (!zName)
      continue;
    const char *match = strstr(zName, h->name);
    if (!match)
      continue;
    char c = match[strlen(h->name)];
    
    if (!c || c == '-')
      break;
  }
  p->histograms = h;

  const char* persistenceType;
  const char* group;
  const char* origin;
  if ((flags & SQLITE_OPEN_URI) &&
      (persistenceType = sqlite3_uri_parameter(zName, "persistenceType")) &&
      (group = sqlite3_uri_parameter(zName, "group")) &&
      (origin = sqlite3_uri_parameter(zName, "origin"))) {
    QuotaManager* quotaManager = QuotaManager::Get();
    MOZ_ASSERT(quotaManager);

    p->quotaObject = quotaManager->GetQuotaObject(PersistenceTypeFromText(
      nsDependentCString(persistenceType)), nsDependentCString(group),
      nsDependentCString(origin), NS_ConvertUTF8toUTF16(zName));
  }

  rc = orig_vfs->xOpen(orig_vfs, zName, p->pReal, flags, pOutFlags);
  if( rc != SQLITE_OK )
    return rc;
  if( p->pReal->pMethods ){
    sqlite3_io_methods *pNew = new sqlite3_io_methods;
    const sqlite3_io_methods *pSub = p->pReal->pMethods;
    memset(pNew, 0, sizeof(*pNew));
    
    
    
    pNew->iVersion = pSub->iVersion;
    MOZ_ASSERT(pNew->iVersion <= LAST_KNOWN_IOMETHODS_VERSION);
    pNew->xClose = xClose;
    pNew->xRead = xRead;
    pNew->xWrite = xWrite;
    pNew->xTruncate = xTruncate;
    pNew->xSync = xSync;
    pNew->xFileSize = xFileSize;
    pNew->xLock = xLock;
    pNew->xUnlock = xUnlock;
    pNew->xCheckReservedLock = xCheckReservedLock;
    pNew->xFileControl = xFileControl;
    pNew->xSectorSize = xSectorSize;
    pNew->xDeviceCharacteristics = xDeviceCharacteristics;
    if (pNew->iVersion >= 2) {
      
      pNew->xShmMap = pSub->xShmMap ? xShmMap : 0;
      pNew->xShmLock = pSub->xShmLock ? xShmLock : 0;
      pNew->xShmBarrier = pSub->xShmBarrier ? xShmBarrier : 0;
      pNew->xShmUnmap = pSub->xShmUnmap ? xShmUnmap : 0;
    }
    if (pNew->iVersion >= 3) {
      
      
      
      
      MOZ_ASSERT(pSub->xFetch);
      pNew->xFetch = xFetch;
      MOZ_ASSERT(pSub->xUnfetch);
      pNew->xUnfetch = xUnfetch;
    }
    pFile->pMethods = pNew;
  }
  return rc;
}

int
xDelete(sqlite3_vfs* vfs, const char *zName, int syncDir)
{
  sqlite3_vfs *orig_vfs = static_cast<sqlite3_vfs*>(vfs->pAppData);
  return orig_vfs->xDelete(orig_vfs, zName, syncDir);
}

int
xAccess(sqlite3_vfs *vfs, const char *zName, int flags, int *pResOut)
{
  sqlite3_vfs *orig_vfs = static_cast<sqlite3_vfs*>(vfs->pAppData);
  return orig_vfs->xAccess(orig_vfs, zName, flags, pResOut);
}

int
xFullPathname(sqlite3_vfs *vfs, const char *zName, int nOut, char *zOut)
{
  sqlite3_vfs *orig_vfs = static_cast<sqlite3_vfs*>(vfs->pAppData);
  return orig_vfs->xFullPathname(orig_vfs, zName, nOut, zOut);
}

void*
xDlOpen(sqlite3_vfs *vfs, const char *zFilename)
{
  sqlite3_vfs *orig_vfs = static_cast<sqlite3_vfs*>(vfs->pAppData);
  return orig_vfs->xDlOpen(orig_vfs, zFilename);
}

void
xDlError(sqlite3_vfs *vfs, int nByte, char *zErrMsg)
{
  sqlite3_vfs *orig_vfs = static_cast<sqlite3_vfs*>(vfs->pAppData);
  orig_vfs->xDlError(orig_vfs, nByte, zErrMsg);
}

void 
(*xDlSym(sqlite3_vfs *vfs, void *pHdle, const char *zSym))(void){
  sqlite3_vfs *orig_vfs = static_cast<sqlite3_vfs*>(vfs->pAppData);
  return orig_vfs->xDlSym(orig_vfs, pHdle, zSym);
}

void
xDlClose(sqlite3_vfs *vfs, void *pHandle)
{
  sqlite3_vfs *orig_vfs = static_cast<sqlite3_vfs*>(vfs->pAppData);
  orig_vfs->xDlClose(orig_vfs, pHandle);
}

int
xRandomness(sqlite3_vfs *vfs, int nByte, char *zOut)
{
  sqlite3_vfs *orig_vfs = static_cast<sqlite3_vfs*>(vfs->pAppData);
  return orig_vfs->xRandomness(orig_vfs, nByte, zOut);
}

int
xSleep(sqlite3_vfs *vfs, int microseconds)
{
  sqlite3_vfs *orig_vfs = static_cast<sqlite3_vfs*>(vfs->pAppData);
  return orig_vfs->xSleep(orig_vfs, microseconds);
}

int
xCurrentTime(sqlite3_vfs *vfs, double *prNow)
{
  sqlite3_vfs *orig_vfs = static_cast<sqlite3_vfs*>(vfs->pAppData);
  return orig_vfs->xCurrentTime(orig_vfs, prNow);
}

int
xGetLastError(sqlite3_vfs *vfs, int nBuf, char *zBuf)
{
  sqlite3_vfs *orig_vfs = static_cast<sqlite3_vfs*>(vfs->pAppData);
  return orig_vfs->xGetLastError(orig_vfs, nBuf, zBuf);
}

int
xCurrentTimeInt64(sqlite3_vfs *vfs, sqlite3_int64 *piNow)
{
  sqlite3_vfs *orig_vfs = static_cast<sqlite3_vfs*>(vfs->pAppData);
  return orig_vfs->xCurrentTimeInt64(orig_vfs, piNow);
}

static
int
xSetSystemCall(sqlite3_vfs *vfs, const char *zName, sqlite3_syscall_ptr pFunc)
{
  sqlite3_vfs *orig_vfs = static_cast<sqlite3_vfs*>(vfs->pAppData);
  return orig_vfs->xSetSystemCall(orig_vfs, zName, pFunc);
}

static
sqlite3_syscall_ptr
xGetSystemCall(sqlite3_vfs *vfs, const char *zName)
{
  sqlite3_vfs *orig_vfs = static_cast<sqlite3_vfs*>(vfs->pAppData);
  return orig_vfs->xGetSystemCall(orig_vfs, zName);
}

static
const char *
xNextSystemCall(sqlite3_vfs *vfs, const char *zName)
{
  sqlite3_vfs *orig_vfs = static_cast<sqlite3_vfs*>(vfs->pAppData);
  return orig_vfs->xNextSystemCall(orig_vfs, zName);
}

}

namespace mozilla {
namespace storage {

sqlite3_vfs* ConstructTelemetryVFS()
{
#if defined(XP_WIN)
#define EXPECTED_VFS     "win32"
#define EXPECTED_VFS_NFS "win32"
#else
#define EXPECTED_VFS     "unix"
#define EXPECTED_VFS_NFS "unix-excl"
#endif
  
  bool expected_vfs;
  sqlite3_vfs *vfs;
  if (Preferences::GetBool(PREF_NFS_FILESYSTEM)) {
    vfs = sqlite3_vfs_find(EXPECTED_VFS_NFS);
    expected_vfs = (vfs != nullptr);
  }
  else {
    vfs = sqlite3_vfs_find(nullptr);
    expected_vfs = vfs->zName && !strcmp(vfs->zName, EXPECTED_VFS);
  }
  if (!expected_vfs) {
    return nullptr;
  }

  sqlite3_vfs *tvfs = new ::sqlite3_vfs;
  memset(tvfs, 0, sizeof(::sqlite3_vfs));
  
  
  
  tvfs->iVersion = vfs->iVersion;
  MOZ_ASSERT(vfs->iVersion <= LAST_KNOWN_VFS_VERSION);
  tvfs->szOsFile = sizeof(telemetry_file) - sizeof(sqlite3_file) + vfs->szOsFile;
  tvfs->mxPathname = vfs->mxPathname;
  tvfs->zName = "telemetry-vfs";
  tvfs->pAppData = vfs;
  tvfs->xOpen = xOpen;
  tvfs->xDelete = xDelete;
  tvfs->xAccess = xAccess;
  tvfs->xFullPathname = xFullPathname;
  tvfs->xDlOpen = xDlOpen;
  tvfs->xDlError = xDlError;
  tvfs->xDlSym = xDlSym;
  tvfs->xDlClose = xDlClose;
  tvfs->xRandomness = xRandomness;
  tvfs->xSleep = xSleep;
  tvfs->xCurrentTime = xCurrentTime;
  tvfs->xGetLastError = xGetLastError;
  if (tvfs->iVersion >= 2) {
    
    tvfs->xCurrentTimeInt64 = xCurrentTimeInt64;
  }
  if (tvfs->iVersion >= 3) {
    
    tvfs->xSetSystemCall = xSetSystemCall;
    tvfs->xGetSystemCall = xGetSystemCall;
    tvfs->xNextSystemCall = xNextSystemCall;
  }
  return tvfs;
}

}
}
