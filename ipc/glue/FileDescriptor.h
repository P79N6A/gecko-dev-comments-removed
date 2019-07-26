



#ifndef mozilla_ipc_FileDescriptor_h
#define mozilla_ipc_FileDescriptor_h

#include "base/basictypes.h"
#include "base/process.h"
#include "mozilla/DebugOnly.h"
#include "nscore.h"

#ifdef XP_WIN

#include <winnt.h>
#else
#include "base/file_descriptor_posix.h"
#endif

namespace mozilla {
namespace ipc {













class FileDescriptor
{
public:
  typedef base::ProcessHandle ProcessHandle;

#ifdef XP_WIN
  typedef HANDLE PlatformHandleType;
  typedef HANDLE PickleType;
#else
  typedef int PlatformHandleType;
  typedef base::FileDescriptor PickleType;
#endif

  
  struct IPDLPrivate
  {};

  FileDescriptor();

  FileDescriptor(const FileDescriptor& aOther)
  {
    
    
    Assign(aOther);
  }

  FileDescriptor(PlatformHandleType aHandle);

  FileDescriptor(const IPDLPrivate&, const PickleType& aPickle)
#ifdef XP_WIN
  : mHandle(aPickle)
#else
  : mHandle(aPickle.fd)
#endif
  , mHandleCreatedByOtherProcess(true)
  , mHandleCreatedByOtherProcessWasUsed(false)
  { }

  ~FileDescriptor()
  {
    CloseCurrentProcessHandle();
  }

  FileDescriptor&
  operator=(const FileDescriptor& aOther)
  {
    CloseCurrentProcessHandle();
    Assign(aOther);
    return *this;
  }

  
  
  
  PickleType
  ShareTo(const IPDLPrivate&, ProcessHandle aOtherProcess) const;

  
  
  bool
  IsValid() const
  {
    return IsValid(mHandle);
  }

  PlatformHandleType
  PlatformHandle() const
  {
    if (mHandleCreatedByOtherProcess) {
      mHandleCreatedByOtherProcessWasUsed = true;
    }
    return mHandle;
  }

  bool
  operator==(const FileDescriptor& aOther) const
  {
    return mHandle == aOther.mHandle;
  }

private:
  void
  Assign(const FileDescriptor& aOther)
  {
    if (aOther.mHandleCreatedByOtherProcess) {
      mHandleCreatedByOtherProcess = true;
      mHandleCreatedByOtherProcessWasUsed =
        aOther.mHandleCreatedByOtherProcessWasUsed;
      mHandle = aOther.PlatformHandle();
    } else {
      DuplicateInCurrentProcess(aOther.PlatformHandle());
      mHandleCreatedByOtherProcess = false;
      mHandleCreatedByOtherProcessWasUsed = false;
    }
  }

  static bool
  IsValid(PlatformHandleType aHandle);

  void
  DuplicateInCurrentProcess(PlatformHandleType aHandle);

  void
  CloseCurrentProcessHandle();

  PlatformHandleType mHandle;

  
  
  
  
  bool mHandleCreatedByOtherProcess;

  
  
  mutable DebugOnly<bool> mHandleCreatedByOtherProcessWasUsed;
};

} 
} 

#endif 
