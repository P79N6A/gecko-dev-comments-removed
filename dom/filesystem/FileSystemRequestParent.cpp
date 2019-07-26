




#include "mozilla/dom/FileSystemRequestParent.h"

#include "CreateDirectoryTask.h"
#include "GetFileOrDirectoryTask.h"

#include "mozilla/dom/FileSystemBase.h"

namespace mozilla {
namespace dom {

FileSystemRequestParent::FileSystemRequestParent()
{
}

FileSystemRequestParent::~FileSystemRequestParent()
{
}

#define FILESYSTEM_REQUEST_PARENT_DISPATCH_ENTRY(name)                         \
    case FileSystemParams::TFileSystem##name##Params: {                        \
      const FileSystem##name##Params& p = aParams;                             \
      mFileSystem = FileSystemBase::FromString(p.filesystem());                \
      task = new name##Task(mFileSystem, p, this);                             \
      break;                                                                   \
    }

bool
FileSystemRequestParent::Dispatch(ContentParent* aParent,
                                  const FileSystemParams& aParams)
{
  MOZ_ASSERT(aParent, "aParent should not be null.");
  nsRefPtr<FileSystemTaskBase> task;
  switch (aParams.type()) {

    FILESYSTEM_REQUEST_PARENT_DISPATCH_ENTRY(CreateDirectory)
    FILESYSTEM_REQUEST_PARENT_DISPATCH_ENTRY(GetFileOrDirectory)

    default: {
      NS_RUNTIMEABORT("not reached");
      break;
    }
  }

  if (NS_WARN_IF(!task || !mFileSystem)) {
    
    return false;
  }

  task->Start();
  return true;
}

void
FileSystemRequestParent::ActorDestroy(ActorDestroyReason why)
{
  if (!mFileSystem) {
    return;
  }
  mFileSystem->Shutdown();
  mFileSystem = nullptr;
}

} 
} 
