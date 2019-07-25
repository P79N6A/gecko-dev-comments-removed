



#include "FileDescriptor.h"

#include "mozilla/Assertions.h"
#include "nsDebug.h"

#ifdef XP_WIN
#include <windows.h>
#define INVALID_HANDLE INVALID_HANDLE_VALUE
#else
#include <unistd.h>
#define INVALID_HANDLE -1
#endif

using mozilla::ipc::FileDescriptor;

FileDescriptor::FileDescriptor()
: mHandle(INVALID_HANDLE)
{ }

FileDescriptor::PickleType
FileDescriptor::ShareTo(const FileDescriptor::IPDLPrivate&,
                        FileDescriptor::ProcessHandle aOtherProcess) const
{
#ifdef XP_WIN
  if (mHandle == INVALID_HANDLE) {
    return INVALID_HANDLE;
  }

  PlatformHandleType newHandle;
  if (!DuplicateHandle(GetCurrentProcess(), mHandle, aOtherProcess, &newHandle,
                       0, FALSE, DUPLICATE_SAME_ACCESS)) {
    NS_WARNING("Failed to duplicate file handle!");
    return INVALID_HANDLE;
  }

  return newHandle;
#else 
  if (mHandle == INVALID_HANDLE) {
    return base::FileDescriptor();
  }

  PlatformHandleType newHandle = dup(mHandle);
  if (newHandle < 0) {
    NS_WARNING("Failed to duplicate file descriptor!");
    return base::FileDescriptor();
  }

  
  
  return base::FileDescriptor(newHandle, true);
#endif

  MOZ_NOT_REACHED("Must not get here!");
  return PickleType();
}

bool
FileDescriptor::IsValid() const
{
  return mHandle != INVALID_HANDLE;
}
