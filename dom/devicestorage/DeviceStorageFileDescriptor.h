





#ifndef DeviceStorageFileDescriptor_h
#define DeviceStorageFileDescriptor_h

#include "mozilla/ipc/FileDescriptor.h"

class DeviceStorageFileDescriptor MOZ_FINAL
  : public mozilla::RefCounted<DeviceStorageFileDescriptor>
{
public:
  nsRefPtr<DeviceStorageFile> mDSFile;
  mozilla::ipc::FileDescriptor mFileDescriptor;
};

#endif
