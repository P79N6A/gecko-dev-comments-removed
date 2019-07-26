



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
: mHandle(INVALID_HANDLE), mMustCloseHandle(false)
{ }

FileDescriptor::FileDescriptor(PlatformHandleType aHandle)
: mHandle(INVALID_HANDLE), mMustCloseHandle(false)
{
  DuplicateInCurrentProcess(aHandle);
}

FileDescriptor::~FileDescriptor()
{
  if (mMustCloseHandle) {
    MOZ_ASSERT(mHandle != INVALID_HANDLE);
#ifdef XP_WIN
    if (!CloseHandle(mHandle)) {
      NS_WARNING("Failed to close file handle!");
    }
#else 
    HANDLE_EINTR(close(mHandle));
#endif
  }
}

void
FileDescriptor::DuplicateInCurrentProcess(PlatformHandleType aHandle)
{
  if (aHandle != INVALID_HANDLE) {
    PlatformHandleType newHandle;
#ifdef XP_WIN
    if (DuplicateHandle(GetCurrentProcess(), aHandle, GetCurrentProcess(),
                        &newHandle, 0, FALSE, DUPLICATE_SAME_ACCESS)) {
#else
    if ((newHandle = dup(aHandle)) != INVALID_HANDLE) {
#endif
      mHandle = newHandle;
      mMustCloseHandle = true;
      return;
    }
    NS_WARNING("Failed to duplicate file descriptor!");
  }

  mHandle = INVALID_HANDLE;
  mMustCloseHandle = false;
}

FileDescriptor::PickleType
FileDescriptor::ShareTo(const FileDescriptor::IPDLPrivate&,
                        FileDescriptor::ProcessHandle aOtherProcess) const
{
  PlatformHandleType newHandle;
#ifdef XP_WIN
  if (mHandle != INVALID_HANDLE) {
    if (DuplicateHandle(GetCurrentProcess(), mHandle, aOtherProcess,
                        &newHandle, 0, FALSE, DUPLICATE_SAME_ACCESS)) {
      
      
      return newHandle;
    }
    NS_WARNING("Failed to duplicate file handle!");
  }
  return INVALID_HANDLE;
#else 
  if (mHandle != INVALID_HANDLE) {
    newHandle = dup(mHandle);
    return base::FileDescriptor(newHandle,  true);
  }
  return base::FileDescriptor();
#endif

  MOZ_NOT_REACHED("Must not get here!");
  return PickleType();
}

bool
FileDescriptor::IsValid() const
{
  return mHandle != INVALID_HANDLE;
}
