



#ifndef mozilla_dom_FileDescriptorSetParent_h__
#define mozilla_dom_FileDescriptorSetParent_h__

#include "mozilla/Assertions.h"
#include "mozilla/Attributes.h"
#include "mozilla/dom/PFileDescriptorSetParent.h"
#include "nsTArray.h"

namespace mozilla {

namespace ipc {

class FileDescriptor;

} 

namespace dom {

class ContentParent;

class FileDescriptorSetParent MOZ_FINAL: public PFileDescriptorSetParent
{
  friend class ContentParent;

public:
  typedef mozilla::ipc::FileDescriptor FileDescriptor;

  void
  ForgetFileDescriptors(nsTArray<FileDescriptor>& aFileDescriptors);

private:
  FileDescriptorSetParent(const FileDescriptor& aFileDescriptor);
  ~FileDescriptorSetParent();

  virtual bool
  RecvAddFileDescriptor(const FileDescriptor& aFileDescriptor) MOZ_OVERRIDE;

  nsTArray<FileDescriptor> mFileDescriptors;
};

} 
} 

#endif 
