




#ifndef mozilla_ipc_FileDescriptorUtils_h
#define mozilla_ipc_FileDescriptorUtils_h

#include "mozilla/Attributes.h"
#include "mozilla/ipc/FileDescriptor.h"
#include "nsIRunnable.h"

namespace mozilla {
namespace ipc {




class CloseFileRunnable MOZ_FINAL : public nsIRunnable
{
  typedef mozilla::ipc::FileDescriptor FileDescriptor;

  FileDescriptor mFileDescriptor;

public:
  CloseFileRunnable(const FileDescriptor& aFileDescriptor)
#ifdef DEBUG
  ;
#else
  : mFileDescriptor(aFileDescriptor)
  { }
#endif

  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIRUNNABLE

  void Dispatch();

private:
  ~CloseFileRunnable();

  void CloseFile();
};

} 
} 

#endif 
