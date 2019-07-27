





#include "PoisonIOInterposer.h"

#include <algorithm>
#include <stdio.h>
#include <vector>

#include <io.h>
#include <windows.h>
#include <winternl.h>

#include "mozilla/Assertions.h"
#include "mozilla/FileUtilsWin.h"
#include "mozilla/IOInterposer.h"
#include "mozilla/Mutex.h"
#include "mozilla/TimeStamp.h"
#include "nsTArray.h"
#include "nsWindowsDllInterceptor.h"
#include "plstr.h"

#ifdef MOZ_REPLACE_MALLOC
#include "replace_malloc_bridge.h"
#endif

using namespace mozilla;

namespace {





static bool sIOPoisoned = false;







typedef NTSTATUS (NTAPI* NtCreateFileFn)(
  PHANDLE aFileHandle,
  ACCESS_MASK aDesiredAccess,
  POBJECT_ATTRIBUTES aObjectAttributes,
  PIO_STATUS_BLOCK aIoStatusBlock,
  PLARGE_INTEGER aAllocationSize,
  ULONG aFileAttributes,
  ULONG aShareAccess,
  ULONG aCreateDisposition,
  ULONG aCreateOptions,
  PVOID aEaBuffer,
  ULONG aEaLength);





typedef NTSTATUS (NTAPI* NtReadFileFn)(
  HANDLE aFileHandle,
  HANDLE aEvent,
  PIO_APC_ROUTINE aApc,
  PVOID aApcCtx,
  PIO_STATUS_BLOCK aIoStatus,
  PVOID aBuffer,
  ULONG aLength,
  PLARGE_INTEGER aOffset,
  PULONG aKey);





typedef NTSTATUS (NTAPI* NtReadFileScatterFn)(
  HANDLE aFileHandle,
  HANDLE aEvent,
  PIO_APC_ROUTINE aApc,
  PVOID aApcCtx,
  PIO_STATUS_BLOCK aIoStatus,
  FILE_SEGMENT_ELEMENT* aSegments,
  ULONG aLength,
  PLARGE_INTEGER aOffset,
  PULONG aKey);





typedef NTSTATUS (NTAPI* NtWriteFileFn)(
  HANDLE aFileHandle,
  HANDLE aEvent,
  PIO_APC_ROUTINE aApc,
  PVOID aApcCtx,
  PIO_STATUS_BLOCK aIoStatus,
  PVOID aBuffer,
  ULONG aLength,
  PLARGE_INTEGER aOffset,
  PULONG aKey);





typedef NTSTATUS (NTAPI* NtWriteFileGatherFn)(
  HANDLE aFileHandle,
  HANDLE aEvent,
  PIO_APC_ROUTINE aApc,
  PVOID aApcCtx,
  PIO_STATUS_BLOCK aIoStatus,
  FILE_SEGMENT_ELEMENT* aSegments,
  ULONG aLength,
  PLARGE_INTEGER aOffset,
  PULONG aKey);






typedef NTSTATUS (NTAPI* NtFlushBuffersFileFn)(
  HANDLE aFileHandle,
  PIO_STATUS_BLOCK aIoStatusBlock);

typedef struct _FILE_NETWORK_OPEN_INFORMATION* PFILE_NETWORK_OPEN_INFORMATION;




typedef NTSTATUS (NTAPI* NtQueryFullAttributesFileFn)(
  POBJECT_ATTRIBUTES aObjectAttributes,
  PFILE_NETWORK_OPEN_INFORMATION aFileInformation);







class WinIOAutoObservation : public IOInterposeObserver::Observation
{
public:
  WinIOAutoObservation(IOInterposeObserver::Operation aOp,
                       HANDLE aFileHandle, const LARGE_INTEGER* aOffset)
    : IOInterposeObserver::Observation(
        aOp, sReference, !IsDebugFile(reinterpret_cast<intptr_t>(aFileHandle)))
    , mFileHandle(aFileHandle)
    , mHasQueriedFilename(false)
    , mFilename(nullptr)
  {
    if (mShouldReport) {
      mOffset.QuadPart = aOffset ? aOffset->QuadPart : 0;
    }
  }

  WinIOAutoObservation(IOInterposeObserver::Operation aOp, nsAString& aFilename)
    : IOInterposeObserver::Observation(aOp, sReference)
    , mFileHandle(nullptr)
    , mHasQueriedFilename(false)
    , mFilename(nullptr)
  {
    if (mShouldReport) {
      nsAutoString dosPath;
      if (NtPathToDosPath(aFilename, dosPath)) {
        mFilename = ToNewUnicode(dosPath);
        mHasQueriedFilename = true;
      }
      mOffset.QuadPart = 0;
    }
  }

  
  const char16_t* Filename() override;

  ~WinIOAutoObservation()
  {
    Report();
    if (mFilename) {
      MOZ_ASSERT(mHasQueriedFilename);
      free(mFilename);
      mFilename = nullptr;
    }
  }

private:
  HANDLE              mFileHandle;
  LARGE_INTEGER       mOffset;
  bool                mHasQueriedFilename;
  char16_t*           mFilename;
  static const char*  sReference;
};

const char* WinIOAutoObservation::sReference = "PoisonIOInterposer";


const char16_t*
WinIOAutoObservation::Filename()
{
  
  if (mHasQueriedFilename) {
    return mFilename;
  }

  nsAutoString utf16Filename;
  if (HandleToFilename(mFileHandle, mOffset, utf16Filename)) {
    
    mFilename = ToNewUnicode(utf16Filename);
  }
  mHasQueriedFilename = true;

  
  return mFilename;
}




static NtCreateFileFn         gOriginalNtCreateFile;
static NtReadFileFn           gOriginalNtReadFile;
static NtReadFileScatterFn    gOriginalNtReadFileScatter;
static NtWriteFileFn          gOriginalNtWriteFile;
static NtWriteFileGatherFn    gOriginalNtWriteFileGather;
static NtFlushBuffersFileFn   gOriginalNtFlushBuffersFile;
static NtQueryFullAttributesFileFn gOriginalNtQueryFullAttributesFile;

static NTSTATUS NTAPI
InterposedNtCreateFile(PHANDLE aFileHandle,
                       ACCESS_MASK aDesiredAccess,
                       POBJECT_ATTRIBUTES aObjectAttributes,
                       PIO_STATUS_BLOCK aIoStatusBlock,
                       PLARGE_INTEGER aAllocationSize,
                       ULONG aFileAttributes,
                       ULONG aShareAccess,
                       ULONG aCreateDisposition,
                       ULONG aCreateOptions,
                       PVOID aEaBuffer,
                       ULONG aEaLength)
{
  
  const wchar_t* buf =
    aObjectAttributes ? aObjectAttributes->ObjectName->Buffer : L"";
  uint32_t len =
    aObjectAttributes ? aObjectAttributes->ObjectName->Length / sizeof(WCHAR) :
                        0;
  nsDependentSubstring filename(buf, len);
  WinIOAutoObservation timer(IOInterposeObserver::OpCreateOrOpen, filename);

  
  MOZ_ASSERT(gOriginalNtCreateFile);

  
  return gOriginalNtCreateFile(aFileHandle,
                               aDesiredAccess,
                               aObjectAttributes,
                               aIoStatusBlock,
                               aAllocationSize,
                               aFileAttributes,
                               aShareAccess,
                               aCreateDisposition,
                               aCreateOptions,
                               aEaBuffer,
                               aEaLength);
}

static NTSTATUS NTAPI
InterposedNtReadFile(HANDLE aFileHandle,
                     HANDLE aEvent,
                     PIO_APC_ROUTINE aApc,
                     PVOID aApcCtx,
                     PIO_STATUS_BLOCK aIoStatus,
                     PVOID aBuffer,
                     ULONG aLength,
                     PLARGE_INTEGER aOffset,
                     PULONG aKey)
{
  
  WinIOAutoObservation timer(IOInterposeObserver::OpRead, aFileHandle, aOffset);

  
  MOZ_ASSERT(gOriginalNtReadFile);

  
  return gOriginalNtReadFile(aFileHandle,
                             aEvent,
                             aApc,
                             aApcCtx,
                             aIoStatus,
                             aBuffer,
                             aLength,
                             aOffset,
                             aKey);
}

static NTSTATUS NTAPI
InterposedNtReadFileScatter(HANDLE aFileHandle,
                            HANDLE aEvent,
                            PIO_APC_ROUTINE aApc,
                            PVOID aApcCtx,
                            PIO_STATUS_BLOCK aIoStatus,
                            FILE_SEGMENT_ELEMENT* aSegments,
                            ULONG aLength,
                            PLARGE_INTEGER aOffset,
                            PULONG aKey)
{
  
  WinIOAutoObservation timer(IOInterposeObserver::OpRead, aFileHandle, aOffset);

  
  MOZ_ASSERT(gOriginalNtReadFileScatter);

  
  return gOriginalNtReadFileScatter(aFileHandle,
                                    aEvent,
                                    aApc,
                                    aApcCtx,
                                    aIoStatus,
                                    aSegments,
                                    aLength,
                                    aOffset,
                                    aKey);
}


static NTSTATUS NTAPI
InterposedNtWriteFile(HANDLE aFileHandle,
                      HANDLE aEvent,
                      PIO_APC_ROUTINE aApc,
                      PVOID aApcCtx,
                      PIO_STATUS_BLOCK aIoStatus,
                      PVOID aBuffer,
                      ULONG aLength,
                      PLARGE_INTEGER aOffset,
                      PULONG aKey)
{
  
  WinIOAutoObservation timer(IOInterposeObserver::OpWrite, aFileHandle,
                             aOffset);

  
  MOZ_ASSERT(gOriginalNtWriteFile);

  
  return gOriginalNtWriteFile(aFileHandle,
                              aEvent,
                              aApc,
                              aApcCtx,
                              aIoStatus,
                              aBuffer,
                              aLength,
                              aOffset,
                              aKey);
}


static NTSTATUS NTAPI
InterposedNtWriteFileGather(HANDLE aFileHandle,
                            HANDLE aEvent,
                            PIO_APC_ROUTINE aApc,
                            PVOID aApcCtx,
                            PIO_STATUS_BLOCK aIoStatus,
                            FILE_SEGMENT_ELEMENT* aSegments,
                            ULONG aLength,
                            PLARGE_INTEGER aOffset,
                            PULONG aKey)
{
  
  WinIOAutoObservation timer(IOInterposeObserver::OpWrite, aFileHandle,
                             aOffset);

  
  MOZ_ASSERT(gOriginalNtWriteFileGather);

  
  return gOriginalNtWriteFileGather(aFileHandle,
                                    aEvent,
                                    aApc,
                                    aApcCtx,
                                    aIoStatus,
                                    aSegments,
                                    aLength,
                                    aOffset,
                                    aKey);
}

static NTSTATUS NTAPI
InterposedNtFlushBuffersFile(HANDLE aFileHandle,
                             PIO_STATUS_BLOCK aIoStatusBlock)
{
  
  WinIOAutoObservation timer(IOInterposeObserver::OpFSync, aFileHandle,
                             nullptr);

  
  MOZ_ASSERT(gOriginalNtFlushBuffersFile);

  
  return gOriginalNtFlushBuffersFile(aFileHandle,
                                     aIoStatusBlock);
}

static NTSTATUS NTAPI
InterposedNtQueryFullAttributesFile(
    POBJECT_ATTRIBUTES aObjectAttributes,
    PFILE_NETWORK_OPEN_INFORMATION aFileInformation)
{
  
  const wchar_t* buf =
    aObjectAttributes ? aObjectAttributes->ObjectName->Buffer : L"";
  uint32_t len =
    aObjectAttributes ? aObjectAttributes->ObjectName->Length / sizeof(WCHAR) :
                        0;
  nsDependentSubstring filename(buf, len);
  WinIOAutoObservation timer(IOInterposeObserver::OpStat, filename);

  
  MOZ_ASSERT(gOriginalNtQueryFullAttributesFile);

  
  return gOriginalNtQueryFullAttributesFile(aObjectAttributes,
                                            aFileInformation);
}

} 




static WindowsDllInterceptor sNtDllInterceptor;

namespace mozilla {

void
InitPoisonIOInterposer()
{
  
  
  
  if (sIOPoisoned) {
    return;
  }
  sIOPoisoned = true;

  
  MozillaRegisterDebugFD(1);
  MozillaRegisterDebugFD(2);

#ifdef MOZ_REPLACE_MALLOC
  
  
  
  static DebugFdRegistry registry;
  ReplaceMalloc::InitDebugFd(registry);
#endif

  
  sNtDllInterceptor.Init("ntdll.dll");
  sNtDllInterceptor.AddHook(
    "NtCreateFile",
    reinterpret_cast<intptr_t>(InterposedNtCreateFile),
    reinterpret_cast<void**>(&gOriginalNtCreateFile));
  sNtDllInterceptor.AddHook(
    "NtReadFile",
    reinterpret_cast<intptr_t>(InterposedNtReadFile),
    reinterpret_cast<void**>(&gOriginalNtReadFile));
  sNtDllInterceptor.AddHook(
    "NtReadFileScatter",
    reinterpret_cast<intptr_t>(InterposedNtReadFileScatter),
    reinterpret_cast<void**>(&gOriginalNtReadFileScatter));
  sNtDllInterceptor.AddHook(
    "NtWriteFile",
    reinterpret_cast<intptr_t>(InterposedNtWriteFile),
    reinterpret_cast<void**>(&gOriginalNtWriteFile));
  sNtDllInterceptor.AddHook(
    "NtWriteFileGather",
    reinterpret_cast<intptr_t>(InterposedNtWriteFileGather),
    reinterpret_cast<void**>(&gOriginalNtWriteFileGather));
  sNtDllInterceptor.AddHook(
    "NtFlushBuffersFile",
    reinterpret_cast<intptr_t>(InterposedNtFlushBuffersFile),
    reinterpret_cast<void**>(&gOriginalNtFlushBuffersFile));
  sNtDllInterceptor.AddHook(
    "NtQueryFullAttributesFile",
    reinterpret_cast<intptr_t>(InterposedNtQueryFullAttributesFile),
    reinterpret_cast<void**>(&gOriginalNtQueryFullAttributesFile));
}

void
ClearPoisonIOInterposer()
{
  MOZ_ASSERT(false);
  if (sIOPoisoned) {
    
    sIOPoisoned = false;
    sNtDllInterceptor = WindowsDllInterceptor();
  }
}

} 
