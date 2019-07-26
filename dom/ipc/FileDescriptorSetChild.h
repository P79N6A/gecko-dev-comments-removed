



#ifndef mozilla_dom_FileDescriptorSetChild_h__
#define mozilla_dom_FileDescriptorSetChild_h__

#include "mozilla/Assertions.h"
#include "mozilla/Attributes.h"
#include "mozilla/dom/PFileDescriptorSetChild.h"
#include "nsTArray.h"

namespace mozilla {

namespace ipc {

class FileDescriptor;

} 

namespace dom {

class ContentChild;

class FileDescriptorSetChild MOZ_FINAL: public PFileDescriptorSetChild
{
  friend class ContentChild;

public:
  typedef mozilla::ipc::FileDescriptor FileDescriptor;

  void
  ForgetFileDescriptors(nsTArray<FileDescriptor>& aFileDescriptors)
  {
    aFileDescriptors.Clear();
    mFileDescriptors.SwapElements(aFileDescriptors);
  }

private:
  FileDescriptorSetChild(const FileDescriptor& aFileDescriptor)
  {
    mFileDescriptors.AppendElement(aFileDescriptor);
  }

  ~FileDescriptorSetChild()
  {
    MOZ_ASSERT(mFileDescriptors.IsEmpty());
  }

  virtual bool
  RecvAddFileDescriptor(const FileDescriptor& aFileDescriptor) MOZ_OVERRIDE
  {
    mFileDescriptors.AppendElement(aFileDescriptor);
    return true;
  }

  nsAutoTArray<FileDescriptor, 1> mFileDescriptors;
};

} 
} 

#endif 
