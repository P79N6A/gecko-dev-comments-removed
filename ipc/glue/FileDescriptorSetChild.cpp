





#include "FileDescriptorSetChild.h"

namespace mozilla {
namespace ipc {

FileDescriptorSetChild::FileDescriptorSetChild(
                                         const FileDescriptor& aFileDescriptor)
{
  mFileDescriptors.AppendElement(aFileDescriptor);
}

FileDescriptorSetChild::~FileDescriptorSetChild()
{
  MOZ_ASSERT(mFileDescriptors.IsEmpty());
}

void
FileDescriptorSetChild::ForgetFileDescriptors(
                                    nsTArray<FileDescriptor>& aFileDescriptors)
{
  aFileDescriptors.Clear();
  mFileDescriptors.SwapElements(aFileDescriptors);
}

bool
FileDescriptorSetChild::RecvAddFileDescriptor(
                                         const FileDescriptor& aFileDescriptor)
{
  mFileDescriptors.AppendElement(aFileDescriptor);
  return true;
}

} 
} 
