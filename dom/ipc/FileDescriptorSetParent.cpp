





#include "mozilla/dom/FileDescriptorSetParent.h"

namespace mozilla {
namespace dom {

FileDescriptorSetParent::FileDescriptorSetParent(
                                          const FileDescriptor& aFileDescriptor)
{
  mFileDescriptors.AppendElement(aFileDescriptor);
}

FileDescriptorSetParent::~FileDescriptorSetParent()
{
}

void
FileDescriptorSetParent::ForgetFileDescriptors(
                                     nsTArray<FileDescriptor>& aFileDescriptors)
{
  aFileDescriptors.Clear();
  mFileDescriptors.SwapElements(aFileDescriptors);
}

void
FileDescriptorSetParent::ActorDestroy(ActorDestroyReason aWhy)
{
  
}

bool
FileDescriptorSetParent::RecvAddFileDescriptor(
                                          const FileDescriptor& aFileDescriptor)
{
  mFileDescriptors.AppendElement(aFileDescriptor);
  return true;
}

} 
} 
