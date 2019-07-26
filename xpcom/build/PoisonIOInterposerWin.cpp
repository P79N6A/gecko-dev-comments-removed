





#include "PoisonIOInterposer.h"

#include <algorithm>
#include <stdio.h>
#include <vector>

#include <io.h>
#include <windows.h>
#include <winternl.h>

#include "mozilla/Assertions.h"
#include "mozilla/IOInterposer.h"
#include "mozilla/Mutex.h"
#include "mozilla/TimeStamp.h"
#include "nsTArray.h"
#include "nsWindowsDllInterceptor.h"
#include "plstr.h"

using namespace mozilla;

namespace {





static bool sIOPoisoned = false;







typedef NTSTATUS (WINAPI *NtWriteFileFn)(
  IN    HANDLE                  aFileHandle,
  IN    HANDLE                  aEvent,
  IN    PIO_APC_ROUTINE         aApc,
  IN    PVOID                   aApcCtx,
  OUT   PIO_STATUS_BLOCK        aIoStatus,
  IN    PVOID                   aBuffer,
  IN    ULONG                   aLength,
  IN    PLARGE_INTEGER          aOffset,
  IN    PULONG                  aKey
);





typedef NTSTATUS (WINAPI *NtWriteFileGatherFn)(
  IN    HANDLE                  aFileHandle,
  IN    HANDLE                  aEvent,
  IN    PIO_APC_ROUTINE         aApc,
  IN    PVOID                   aApcCtx,
  OUT   PIO_STATUS_BLOCK        aIoStatus,
  IN    FILE_SEGMENT_ELEMENT*   aSegments,
  IN    ULONG                   aLength,
  IN    PLARGE_INTEGER          aOffset,
  IN    PULONG                  aKey
);







class WinIOAutoObservation : public IOInterposeObserver::Observation
{
public:
  WinIOAutoObservation(IOInterposeObserver::Operation aOp,
                       const char* aReference, HANDLE aFileHandle)
    : mFileHandle(aFileHandle),
      mShouldObserve(IOInterposer::IsObservedOperation(aOp) &&
                     !IsDebugFile(reinterpret_cast<intptr_t>(aFileHandle)))
  {
    if (mShouldObserve) {
      mOperation = aOp;
      mReference = aReference;
      mStart = TimeStamp::Now();
    }
  }

  ~WinIOAutoObservation()
  {
    if (mShouldObserve) {
      mEnd = TimeStamp::Now();
      
      IOInterposer::Report(*this);
    }
  }

private:
  HANDLE              mFileHandle;
  bool                mShouldObserve;
};




static NtWriteFileFn          gOriginalNtWriteFile;
static NtWriteFileGatherFn    gOriginalNtWriteFileGather;


static NTSTATUS WINAPI InterposedNtWriteFile(
  HANDLE                        aFileHandle,
  HANDLE                        aEvent,
  PIO_APC_ROUTINE               aApc,
  PVOID                         aApcCtx,
  PIO_STATUS_BLOCK              aIoStatus,
  PVOID                         aBuffer,
  ULONG                         aLength,
  PLARGE_INTEGER                aOffset,
  PULONG                        aKey)
{
  
  const char* ref = "NtWriteFile";
  WinIOAutoObservation timer(IOInterposeObserver::OpWrite, ref, aFileHandle);

  
  MOZ_ASSERT(gOriginalNtWriteFile);

  
  return gOriginalNtWriteFile(
    aFileHandle,
    aEvent,
    aApc,
    aApcCtx,
    aIoStatus,
    aBuffer,
    aLength,
    aOffset,
    aKey
  );
}


static NTSTATUS WINAPI InterposedNtWriteFileGather(
  HANDLE                        aFileHandle,
  HANDLE                        aEvent,
  PIO_APC_ROUTINE               aApc,
  PVOID                         aApcCtx,
  PIO_STATUS_BLOCK              aIoStatus,
  FILE_SEGMENT_ELEMENT*         aSegments,
  ULONG                         aLength,
  PLARGE_INTEGER                aOffset,
  PULONG                        aKey)
{
  
  const char* ref = "NtWriteFileGather";
  WinIOAutoObservation timer(IOInterposeObserver::OpWrite, ref, aFileHandle);

  
  MOZ_ASSERT(gOriginalNtWriteFileGather);

  
  return gOriginalNtWriteFileGather(
    aFileHandle,
    aEvent,
    aApc,
    aApcCtx,
    aIoStatus,
    aSegments,
    aLength,
    aOffset,
    aKey
  );
}

} 




static WindowsDllInterceptor sNtDllInterceptor;

namespace mozilla {

void InitPoisonIOInterposer() {
  
  
  
  if (sIOPoisoned) {
    return;
  }
  sIOPoisoned = true;

  
  MozillaRegisterDebugFD(1);
  MozillaRegisterDebugFD(2);

  
  sNtDllInterceptor.Init("ntdll.dll");
  sNtDllInterceptor.AddHook(
    "NtWriteFile",
    reinterpret_cast<intptr_t>(InterposedNtWriteFile),
    reinterpret_cast<void**>(&gOriginalNtWriteFile)
  );
  sNtDllInterceptor.AddHook(
    "NtWriteFileGather",
    reinterpret_cast<intptr_t>(InterposedNtWriteFileGather),
    reinterpret_cast<void**>(&gOriginalNtWriteFileGather)
  );
}

void ClearPoisonIOInterposer() {
  MOZ_ASSERT(false);
  if (sIOPoisoned) {
    
    sIOPoisoned = false;
    sNtDllInterceptor = WindowsDllInterceptor();
  }
}

} 
