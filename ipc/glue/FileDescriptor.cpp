



#include "FileDescriptor.h"

#include "mozilla/Assertions.h"
#include "nsDebug.h"

#ifdef XP_WIN

#include <windows.h>
#define INVALID_HANDLE INVALID_HANDLE_VALUE

#else 

#include <unistd.h>

#ifndef OS_POSIX
#define OS_POSIX
#endif

#include "base/eintr_wrapper.h"
#define INVALID_HANDLE -1

#endif 

using mozilla::ipc::FileDescriptor;

FileDescriptor::FileDescriptor()
: mHandle(INVALID_HANDLE), mHandleCreatedByOtherProcess(false),
  mHandleCreatedByOtherProcessWasUsed(false)
{ }

FileDescriptor::FileDescriptor(PlatformHandleType aHandle)
: mHandle(INVALID_HANDLE), mHandleCreatedByOtherProcess(false),
  mHandleCreatedByOtherProcessWasUsed(false)
{
  DuplicateInCurrentProcess(aHandle);
}

void
FileDescriptor::DuplicateInCurrentProcess(PlatformHandleType aHandle)
{
  MOZ_ASSERT_IF(mHandleCreatedByOtherProcess && IsValid(),
                mHandleCreatedByOtherProcessWasUsed);

  if (IsValid(aHandle)) {
    PlatformHandleType newHandle;
#ifdef XP_WIN
    if (DuplicateHandle(GetCurrentProcess(), aHandle, GetCurrentProcess(),
                        &newHandle, 0, FALSE, DUPLICATE_SAME_ACCESS)) {
#else
    if ((newHandle = dup(aHandle)) != INVALID_HANDLE) {
#endif
      mHandle = newHandle;
      return;
    }
    NS_WARNING("Failed to duplicate file handle for current process!");
  }

  mHandle = INVALID_HANDLE;
}

void
FileDescriptor::CloseCurrentProcessHandle()
{
  MOZ_ASSERT_IF(mHandleCreatedByOtherProcess && IsValid(),
                mHandleCreatedByOtherProcessWasUsed);

  
  if (mHandleCreatedByOtherProcess) {
    return;
  }

  if (IsValid()) {
#ifdef XP_WIN
    if (!CloseHandle(mHandle)) {
      NS_WARNING("Failed to close file handle for current process!");
    }
#else 
    HANDLE_EINTR(close(mHandle));
#endif
    mHandle = INVALID_HANDLE;
  }
}

FileDescriptor::PickleType
FileDescriptor::ShareTo(const FileDescriptor::IPDLPrivate&,
                        FileDescriptor::ProcessHandle aOtherProcess) const
{
  PlatformHandleType newHandle;
#ifdef XP_WIN
  if (IsValid()) {
    if (DuplicateHandle(GetCurrentProcess(), mHandle, aOtherProcess,
                        &newHandle, 0, FALSE, DUPLICATE_SAME_ACCESS)) {
      return newHandle;
    }
    NS_WARNING("Failed to duplicate file handle for other process!");
  }
  return INVALID_HANDLE;
#else 
  if (IsValid()) {
    newHandle = dup(mHandle);
    if (IsValid(newHandle)) {
      return base::FileDescriptor(newHandle,  true);
    }
    NS_WARNING("Failed to duplicate file handle for other process!");
  }
  return base::FileDescriptor();
#endif

  MOZ_CRASH("Must not get here!");
}


bool
FileDescriptor::IsValid(PlatformHandleType aHandle)
{
  return aHandle != INVALID_HANDLE;
}
