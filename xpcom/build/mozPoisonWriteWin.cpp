





#include <stdio.h>
#include <windows.h>
#include <winternl.h> 
#include <io.h>
#include "nsWindowsDllInterceptor.h"
#include "mozilla/mozPoisonWrite.h"
#include "mozPoisonWriteBase.h"
#include "mozilla/Assertions.h"

namespace mozilla {

intptr_t FileDescriptorToID(int aFd) {
  return _get_osfhandle(aFd);
}

static WindowsDllInterceptor sNtDllInterceptor;

void AbortOnBadWrite(HANDLE);
bool ValidWriteAssert(bool ok);

typedef NTSTATUS (WINAPI* WriteFile_fn)(HANDLE, HANDLE, PIO_APC_ROUTINE,
                                        void*, PIO_STATUS_BLOCK,
                                        const void*, ULONG, PLARGE_INTEGER,
                                        PULONG);
WriteFile_fn gOriginalWriteFile;

static NTSTATUS WINAPI
patched_WriteFile(HANDLE aFile, HANDLE aEvent, PIO_APC_ROUTINE aApc,
                  void* aApcUser, PIO_STATUS_BLOCK aIoStatus,
                  const void* aBuffer, ULONG aLength,
                  PLARGE_INTEGER aOffset, PULONG aKey)
{
  AbortOnBadWrite(aFile);
  return gOriginalWriteFile(aFile, aEvent, aApc, aApcUser, aIoStatus,
                            aBuffer, aLength, aOffset, aKey);
}


typedef NTSTATUS (WINAPI* WriteFileGather_fn)(HANDLE, HANDLE, PIO_APC_ROUTINE,
                                              void*, PIO_STATUS_BLOCK,
                                              FILE_SEGMENT_ELEMENT*,
                                              ULONG, PLARGE_INTEGER, PULONG);
WriteFileGather_fn gOriginalWriteFileGather;

static NTSTATUS WINAPI
patched_WriteFileGather(HANDLE aFile, HANDLE aEvent, PIO_APC_ROUTINE aApc,
                        void* aApcUser, PIO_STATUS_BLOCK aIoStatus,
                        FILE_SEGMENT_ELEMENT* aSegments, ULONG aLength,
                        PLARGE_INTEGER aOffset, PULONG aKey)
{
  AbortOnBadWrite(aFile);
  return gOriginalWriteFileGather(aFile, aEvent, aApc, aApcUser, aIoStatus,
                                  aSegments, aLength, aOffset, aKey);
}

void AbortOnBadWrite(HANDLE aFile)
{
  if (!PoisonWriteEnabled())
    return;

  
  if (IsDebugFile(reinterpret_cast<intptr_t>(aFile)))
    return;

  ValidWriteAssert(false);
}

void PoisonWrite() {
  
  static bool WritesArePoisoned = false;
  MOZ_ASSERT(!WritesArePoisoned);
  if (WritesArePoisoned)
    return;
  WritesArePoisoned = true;

  if (!PoisonWriteEnabled())
    return;

  sNtDllInterceptor.Init("ntdll.dll");
  sNtDllInterceptor.AddHook("NtWriteFile", reinterpret_cast<intptr_t>(patched_WriteFile), reinterpret_cast<void**>(&gOriginalWriteFile));
  sNtDllInterceptor.AddHook("NtWriteFileGather", reinterpret_cast<intptr_t>(patched_WriteFileGather), reinterpret_cast<void**>(&gOriginalWriteFileGather));
}
}

