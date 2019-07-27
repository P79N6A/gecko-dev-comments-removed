





#ifndef DeviceStorageFileDescriptor_h
#define DeviceStorageFileDescriptor_h

#include "mozilla/ipc/FileDescriptor.h"

struct DeviceStorageFileDescriptor final
{
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(DeviceStorageFileDescriptor)
  nsRefPtr<DeviceStorageFile> mDSFile;
  mozilla::ipc::FileDescriptor mFileDescriptor;
private:
  ~DeviceStorageFileDescriptor() {}
};

#endif
