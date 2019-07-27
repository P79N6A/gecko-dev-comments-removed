



#ifndef mozilla_ipc_FileDescriptorSetParent_h__
#define mozilla_ipc_FileDescriptorSetParent_h__

#include "mozilla/Assertions.h"
#include "mozilla/Attributes.h"
#include "mozilla/ipc/PFileDescriptorSetParent.h"
#include "nsTArray.h"

namespace mozilla {

namespace dom {

class ContentParent;

} 

namespace ipc {

class BackgroundParentImpl;
class FileDescriptor;

class FileDescriptorSetParent MOZ_FINAL
  : public PFileDescriptorSetParent
{
  friend class BackgroundParentImpl;
  friend class mozilla::dom::ContentParent;

  nsTArray<FileDescriptor> mFileDescriptors;

public:
  void
  ForgetFileDescriptors(nsTArray<FileDescriptor>& aFileDescriptors);

private:
  explicit FileDescriptorSetParent(const FileDescriptor& aFileDescriptor);
  ~FileDescriptorSetParent();

  virtual void ActorDestroy(ActorDestroyReason aWhy) MOZ_OVERRIDE;

  virtual bool
  RecvAddFileDescriptor(const FileDescriptor& aFileDescriptor) MOZ_OVERRIDE;
};

} 
} 

#endif 
