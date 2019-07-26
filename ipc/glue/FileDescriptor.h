



#ifndef mozilla_ipc_FileDescriptor_h
#define mozilla_ipc_FileDescriptor_h

#include "base/basictypes.h"
#include "base/process.h"
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
    *this = aOther;
  }

  FileDescriptor(PlatformHandleType aHandle);

  FileDescriptor(const IPDLPrivate&, const PickleType& aPickle)
#ifdef XP_WIN
  : mHandle(aPickle), mMustCloseHandle(false)
#else
  : mHandle(aPickle.fd), mMustCloseHandle(false)
#endif
  { }

  ~FileDescriptor();

  FileDescriptor&
  operator=(const FileDescriptor& aOther)
  {
    DuplicateInCurrentProcess(aOther.mHandle);
    return *this;
  }

  
  
  
  PickleType
  ShareTo(const IPDLPrivate&, ProcessHandle aOtherProcess) const;

  
  
  bool
  IsValid() const;

  PlatformHandleType
  PlatformHandle() const
  {
    return mHandle;
  }

  bool
  operator==(const FileDescriptor& aOther) const
  {
    return mHandle == aOther.mHandle;
  }

private:
  void
  DuplicateInCurrentProcess(PlatformHandleType aHandle);

  PlatformHandleType mHandle;
  bool mMustCloseHandle;
};

} 
} 

#endif 
