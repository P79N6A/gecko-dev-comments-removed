





#ifndef DeviceStorageFileDescriptor_h
#define DeviceStorageFileDescriptor_h

#include "mozilla/ipc/FileDescriptor.h"

struct DeviceStorageFileDescriptor MOZ_FINAL
{
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(DeviceStorageFileDescriptor)
  nsRefPtr<DeviceStorageFile> mDSFile;
  mozilla::ipc::FileDescriptor mFileDescriptor;
private:
  ~DeviceStorageFileDescriptor() {}
};

#endif
