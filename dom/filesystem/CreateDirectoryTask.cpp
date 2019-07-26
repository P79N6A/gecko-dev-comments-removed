





#include "CreateDirectoryTask.h"

#include "DOMError.h"
#include "mozilla/dom/Directory.h"
#include "mozilla/dom/FileSystemBase.h"
#include "mozilla/dom/FileSystemUtils.h"
#include "mozilla/dom/Promise.h"
#include "nsIFile.h"
#include "nsStringGlue.h"

namespace mozilla {
namespace dom {

CreateDirectoryTask::CreateDirectoryTask(FileSystemBase* aFileSystem,
                                         const nsAString& aPath)
  : FileSystemTaskBase(aFileSystem)
  , mTargetRealPath(aPath)
{
  MOZ_ASSERT(NS_IsMainThread(), "Only call on main thread!");
  if (!aFileSystem) {
    return;
  }
  nsCOMPtr<nsIGlobalObject> globalObject =
    do_QueryInterface(aFileSystem->GetWindow());
  if (!globalObject) {
    return;
  }
  mPromise = new Promise(globalObject);
}

CreateDirectoryTask::CreateDirectoryTask(
  FileSystemBase* aFileSystem,
  const FileSystemCreateDirectoryParams& aParam,
  FileSystemRequestParent* aParent)
  : FileSystemTaskBase(aFileSystem, aParam, aParent)
{
  MOZ_ASSERT(FileSystemUtils::IsParentProcess(),
             "Only call from parent process!");
  MOZ_ASSERT(NS_IsMainThread(), "Only call on main thread!");
  mTargetRealPath = aParam.realPath();
}

CreateDirectoryTask::~CreateDirectoryTask()
{
  MOZ_ASSERT(NS_IsMainThread(), "Only call on main thread!");
}

already_AddRefed<Promise>
CreateDirectoryTask::GetPromise()
{
  MOZ_ASSERT(NS_IsMainThread(), "Only call on main thread!");
  return nsRefPtr<Promise>(mPromise).forget();
}

FileSystemParams
CreateDirectoryTask::GetRequestParams(const nsString& aFileSystem) const
{
  MOZ_ASSERT(NS_IsMainThread(), "Only call on main thread!");
  return FileSystemCreateDirectoryParams(aFileSystem, mTargetRealPath);
}

FileSystemResponseValue
CreateDirectoryTask::GetSuccessRequestResult() const
{
  MOZ_ASSERT(NS_IsMainThread(), "Only call on main thread!");
  return FileSystemDirectoryResponse(mTargetRealPath);
}

void
CreateDirectoryTask::SetSuccessRequestResult(const FileSystemResponseValue& aValue)
{
  MOZ_ASSERT(NS_IsMainThread(), "Only call on main thread!");
  FileSystemDirectoryResponse r = aValue;
  mTargetRealPath = r.realPath();
}

void
CreateDirectoryTask::Work()
{
  MOZ_ASSERT(FileSystemUtils::IsParentProcess(),
             "Only call from parent process!");
  MOZ_ASSERT(!NS_IsMainThread(), "Only call on worker thread!");

  nsRefPtr<FileSystemBase> filesystem = do_QueryReferent(mFileSystem);
  if (!filesystem) {
    return;
  }

  nsCOMPtr<nsIFile> file = filesystem->GetLocalFile(mTargetRealPath);
  if (!file) {
    SetError(NS_ERROR_DOM_FILESYSTEM_INVALID_PATH_ERR);
    return;
  }

  bool ret;
  nsresult rv = file->Exists(&ret);
  if (NS_FAILED(rv)) {
    SetError(rv);
    return;
  }

  if (ret) {
    SetError(NS_ERROR_DOM_FILESYSTEM_PATH_EXISTS_ERR);
    return;
  }

  rv = file->Create(nsIFile::DIRECTORY_TYPE, 0777);
  if (NS_FAILED(rv)) {
    SetError(rv);
    return;
  }
}

void
CreateDirectoryTask::HandlerCallback()
{
  MOZ_ASSERT(NS_IsMainThread(), "Only call on main thread!");
  nsRefPtr<FileSystemBase> filesystem = do_QueryReferent(mFileSystem);
  if (!filesystem) {
    return;
  }

  if (HasError()) {
    nsRefPtr<DOMError> domError = new DOMError(filesystem->GetWindow(),
      mErrorValue);
    mPromise->MaybeReject(domError);
    return;
  }
  nsRefPtr<Directory> dir = new Directory(filesystem, mTargetRealPath);
  mPromise->MaybeResolve(dir);
}

} 
} 
