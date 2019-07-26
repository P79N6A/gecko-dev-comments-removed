





#ifndef DeviceStorageFileDescriptor_h
#define DeviceStorageFileDescriptor_h

#include "mozilla/ipc/FileDescriptor.h"

class DeviceStorageFileDescriptor MOZ_FINAL
  : public mozilla::RefCounted<DeviceStorageFileDescriptor>
{
public:
  MOZ_DECLARE_REFCOUNTED_TYPENAME(DeviceStorageFileDescriptor)
  nsRefPtr<DeviceStorageFile> mDSFile;
  mozilla::ipc::FileDescriptor mFileDescriptor;
};

#endif
