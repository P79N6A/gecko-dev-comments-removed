





#include <stdio.h>
#include <windows.h>
#include <winternl.h> 
#include "nsWindowsDllInterceptor.h"
#include "mozilla/mozPoisonWrite.h"
#include "mozPoisonWriteBase.h"
#include "mozilla/Assertions.h"

namespace mozilla {

static WindowsDllInterceptor sNtDllInterceptor;

void AbortOnBadWrite();
bool ValidWriteAssert(bool ok);

typedef NTSTATUS (WINAPI* FlushBuffersFile_fn)(HANDLE, PIO_STATUS_BLOCK);
FlushBuffersFile_fn gOriginalFlushBuffersFile;

NTSTATUS WINAPI
patched_FlushBuffersFile(HANDLE aFileHandle, PIO_STATUS_BLOCK aIoStatusBlock)
{
  AbortOnBadWrite();
  return gOriginalFlushBuffersFile(aFileHandle, aIoStatusBlock);
}

void AbortOnBadWrite()
{
  if (!PoisonWriteEnabled())
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

  PoisonWriteBase();

  sNtDllInterceptor.Init("ntdll.dll");
  sNtDllInterceptor.AddHook("NtFlushBuffersFile", reinterpret_cast<intptr_t>(patched_FlushBuffersFile), reinterpret_cast<void**>(&gOriginalFlushBuffersFile));
}
}

