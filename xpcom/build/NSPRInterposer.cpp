





#include "IOInterposer.h"
#include "NSPRInterposer.h"

#include "prio.h"
#include "private/pprio.h"

namespace {

using namespace mozilla;


PRCloseFN sCloseFn = nullptr;
PRReadFN  sReadFn  = nullptr;
PRWriteFN sWriteFn = nullptr;
PRFsyncFN sFSyncFn = nullptr;
PRFileInfoFN sFileInfoFn = nullptr;
PRFileInfo64FN sFileInfo64Fn = nullptr;





class NSPRIOAutoObservation : public IOInterposeObserver::Observation
{
public:
  explicit NSPRIOAutoObservation(IOInterposeObserver::Operation aOp)
    : IOInterposeObserver::Observation(aOp, "NSPRIOInterposer")
  {
  }

  ~NSPRIOAutoObservation()
  {
    Report();
  }
};

PRStatus PR_CALLBACK
interposedClose(PRFileDesc* aFd)
{
  
  NS_ASSERTION(sCloseFn, "NSPR IO Interposing: sCloseFn is NULL");

  NSPRIOAutoObservation timer(IOInterposeObserver::OpClose);
  return sCloseFn(aFd);
}

int32_t PR_CALLBACK
interposedRead(PRFileDesc* aFd, void* aBuf, int32_t aAmt)
{
  
  NS_ASSERTION(sReadFn, "NSPR IO Interposing: sReadFn is NULL");

  NSPRIOAutoObservation timer(IOInterposeObserver::OpRead);
  return sReadFn(aFd, aBuf, aAmt);
}

int32_t PR_CALLBACK
interposedWrite(PRFileDesc* aFd, const void* aBuf, int32_t aAmt)
{
  
  NS_ASSERTION(sWriteFn, "NSPR IO Interposing: sWriteFn is NULL");

  NSPRIOAutoObservation timer(IOInterposeObserver::OpWrite);
  return sWriteFn(aFd, aBuf, aAmt);
}

PRStatus PR_CALLBACK
interposedFSync(PRFileDesc* aFd)
{
  
  NS_ASSERTION(sFSyncFn, "NSPR IO Interposing: sFSyncFn is NULL");

  NSPRIOAutoObservation timer(IOInterposeObserver::OpFSync);
  return sFSyncFn(aFd);
}

PRStatus PR_CALLBACK
interposedFileInfo(PRFileDesc* aFd, PRFileInfo* aInfo)
{
  
  NS_ASSERTION(sFileInfoFn, "NSPR IO Interposing: sFileInfoFn is NULL");

  NSPRIOAutoObservation timer(IOInterposeObserver::OpStat);
  return sFileInfoFn(aFd, aInfo);
}

PRStatus PR_CALLBACK
interposedFileInfo64(PRFileDesc* aFd, PRFileInfo64* aInfo)
{
  
  NS_ASSERTION(sFileInfo64Fn, "NSPR IO Interposing: sFileInfo64Fn is NULL");

  NSPRIOAutoObservation timer(IOInterposeObserver::OpStat);
  return sFileInfo64Fn(aFd, aInfo);
}

} 

namespace mozilla {

void
InitNSPRIOInterposing()
{
  
  MOZ_ASSERT(!sCloseFn && !sReadFn && !sWriteFn && !sFSyncFn && !sFileInfoFn &&
             !sFileInfo64Fn);

  
  
  

  
  PRIOMethods* methods = const_cast<PRIOMethods*>(PR_GetFileMethods());

  
  
  
  MOZ_ASSERT(methods);
  if (!methods) {
    return;
  }

  
  sCloseFn      = methods->close;
  sReadFn       = methods->read;
  sWriteFn      = methods->write;
  sFSyncFn      = methods->fsync;
  sFileInfoFn   = methods->fileInfo;
  sFileInfo64Fn = methods->fileInfo64;

  
  methods->close      = &interposedClose;
  methods->read       = &interposedRead;
  methods->write      = &interposedWrite;
  methods->fsync      = &interposedFSync;
  methods->fileInfo   = &interposedFileInfo;
  methods->fileInfo64 = &interposedFileInfo64;
}

void
ClearNSPRIOInterposing()
{
  
  
  MOZ_ASSERT(sCloseFn && sReadFn && sWriteFn && sFSyncFn && sFileInfoFn &&
             sFileInfo64Fn);

  
  PRIOMethods* methods = const_cast<PRIOMethods*>(PR_GetFileMethods());

  
  
  
  MOZ_ASSERT(methods);
  if (!methods) {
    return;
  }

  
  methods->close      = sCloseFn;
  methods->read       = sReadFn;
  methods->write      = sWriteFn;
  methods->fsync      = sFSyncFn;
  methods->fileInfo   = sFileInfoFn;
  methods->fileInfo64 = sFileInfo64Fn;

  
  sCloseFn      = nullptr;
  sReadFn       = nullptr;
  sWriteFn      = nullptr;
  sFSyncFn      = nullptr;
  sFileInfoFn   = nullptr;
  sFileInfo64Fn = nullptr;
}

} 

