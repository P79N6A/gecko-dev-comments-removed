




#include "mozilla/dom/FileSystemRequestParent.h"

#include "CreateDirectoryTask.h"
#include "GetFileOrDirectoryTask.h"

#include "mozilla/AppProcessChecker.h"
#include "mozilla/dom/FileSystemBase.h"

namespace mozilla {
namespace dom {

FileSystemRequestParent::FileSystemRequestParent()
{
}

FileSystemRequestParent::~FileSystemRequestParent()
{
}

bool
FileSystemRequestParent::Dispatch(ContentParent* aParent,
                                  const FileSystemParams& aParams)
{
  MOZ_ASSERT(aParent, "aParent should not be null.");
  nsRefPtr<FileSystemTaskBase> task;
  switch (aParams.type()) {

    case FileSystemParams::TFileSystemCreateDirectoryParams: {
      const FileSystemCreateDirectoryParams& p = aParams;
      mFileSystem = FileSystemBase::FromString(p.filesystem());
      task = new CreateDirectoryTask(mFileSystem, p, this);
      break;
    }

    case FileSystemParams::TFileSystemGetFileOrDirectoryParams: {
      const FileSystemGetFileOrDirectoryParams& p = aParams;
      mFileSystem = FileSystemBase::FromString(p.filesystem());
      task  = new GetFileOrDirectoryTask(mFileSystem, p, this);
      break;
    }

    default: {
      NS_RUNTIMEABORT("not reached");
      break;
    }
  }

  if (NS_WARN_IF(!task || !mFileSystem)) {
    
    return false;
  }

  if (!mFileSystem->IsTesting()) {
    

    nsCString access;
    task->GetPermissionAccessType(access);

    nsAutoCString permissionName;
    permissionName = mFileSystem->GetPermission();
    permissionName.AppendLiteral("-");
    permissionName.Append(access);

    if (!AssertAppProcessPermission(aParent, permissionName.get())) {
      return false;
    }
  }

  task->Start();
  return true;
}

} 
} 
