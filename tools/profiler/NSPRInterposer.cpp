



#include "IOInterposer.h"
#include "NSPRInterposer.h"

#include "prio.h"
#include "private/pprio.h"

namespace {

using namespace mozilla;


PRReadFN  sReadFn  = nullptr;
PRWriteFN sWriteFn = nullptr;
PRFsyncFN sFSyncFn = nullptr;





class NSPRIOAutoObservation : public IOInterposeObserver::Observation
{
public:
  NSPRIOAutoObservation(IOInterposeObserver::Operation aOp)
    : mShouldObserve(IOInterposer::IsObservedOperation(aOp))
  {
    if (mShouldObserve) {
      mOperation = aOp;
      mStart = TimeStamp::Now(); 
    }
  }

  ~NSPRIOAutoObservation()
  {
    if (mShouldObserve) {
      mEnd  = TimeStamp::Now();
      const char* ref = "NSPRIOInterposing";
      mReference = ref;

      
      IOInterposer::Report(*this);
    }
  }

private:
  bool mShouldObserve;
};

int32_t PR_CALLBACK interposedRead(PRFileDesc* aFd, void* aBuf, int32_t aAmt)
{
  
  NS_ASSERTION(sReadFn, "NSPR IO Interposing: sReadFn is NULL");

  NSPRIOAutoObservation timer(IOInterposeObserver::OpRead);
  return sReadFn(aFd, aBuf, aAmt);
}

int32_t PR_CALLBACK interposedWrite(PRFileDesc* aFd, const void* aBuf,
                                    int32_t aAmt)
{
  
  NS_ASSERTION(sWriteFn, "NSPR IO Interposing: sWriteFn is NULL");

  NSPRIOAutoObservation timer(IOInterposeObserver::OpWrite);
  return sWriteFn(aFd, aBuf, aAmt);
}

PRStatus PR_CALLBACK interposedFSync(PRFileDesc* aFd)
{
  
  NS_ASSERTION(sFSyncFn, "NSPR IO Interposing: sFSyncFn is NULL");

  NSPRIOAutoObservation timer(IOInterposeObserver::OpFSync);
  return sFSyncFn(aFd);
}

} 

namespace mozilla {

void InitNSPRIOInterposing()
{
  
  MOZ_ASSERT(!sReadFn && !sWriteFn && !sFSyncFn);

  
  
  

  
  PRIOMethods* methods = const_cast<PRIOMethods*>(PR_GetFileMethods());

  
  
  
  MOZ_ASSERT(methods);
  if (!methods) {
    return;
  }

  
  sReadFn   = methods->read;
  sWriteFn  = methods->write;
  sFSyncFn  = methods->fsync;

  
  methods->read   = &interposedRead;
  methods->write  = &interposedWrite;
  methods->fsync  = &interposedFSync;
}

void ClearNSPRIOInterposing()
{
  
  
  MOZ_ASSERT(sReadFn && sWriteFn && sFSyncFn);

  
  PRIOMethods* methods = const_cast<PRIOMethods*>(PR_GetFileMethods());

  
  
  
  MOZ_ASSERT(methods);
  if (!methods) {
    return;
  }

  
  methods->read   = sReadFn;
  methods->write  = sWriteFn;
  methods->fsync  = sFSyncFn;

  
  sReadFn   = nullptr;
  sWriteFn  = nullptr;
  sFSyncFn  = nullptr;
}

} 
