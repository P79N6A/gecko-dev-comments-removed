





#include "GetFileOrDirectoryTask.h"

#include "js/Value.h"
#include "mozilla/dom/Directory.h"
#include "mozilla/dom/FileSystemBase.h"
#include "mozilla/dom/FileSystemUtils.h"
#include "mozilla/dom/Promise.h"
#include "nsDOMFile.h"
#include "nsIFile.h"
#include "nsStringGlue.h"

namespace mozilla {
namespace dom {

GetFileOrDirectoryTask::GetFileOrDirectoryTask(
  FileSystemBase* aFileSystem,
  const nsAString& aTargetPath,
  bool aDirectoryOnly,
  ErrorResult& aRv)
  : FileSystemTaskBase(aFileSystem)
  , mTargetRealPath(aTargetPath)
  , mIsDirectory(aDirectoryOnly)
{
  MOZ_ASSERT(NS_IsMainThread(), "Only call on main thread!");
  MOZ_ASSERT(aFileSystem);
  nsCOMPtr<nsIGlobalObject> globalObject =
    do_QueryInterface(aFileSystem->GetWindow());
  if (!globalObject) {
    return;
  }
  mPromise = Promise::Create(globalObject, aRv);
}

GetFileOrDirectoryTask::GetFileOrDirectoryTask(
  FileSystemBase* aFileSystem,
  const FileSystemGetFileOrDirectoryParams& aParam,
  FileSystemRequestParent* aParent)
  : FileSystemTaskBase(aFileSystem, aParam, aParent)
  , mIsDirectory(false)
{
  MOZ_ASSERT(FileSystemUtils::IsParentProcess(),
             "Only call from parent process!");
  MOZ_ASSERT(NS_IsMainThread(), "Only call on main thread!");
  MOZ_ASSERT(aFileSystem);
  mTargetRealPath = aParam.realPath();
}

GetFileOrDirectoryTask::~GetFileOrDirectoryTask()
{
  MOZ_ASSERT(!mPromise || NS_IsMainThread(),
             "mPromise should be released on main thread!");
}

already_AddRefed<Promise>
GetFileOrDirectoryTask::GetPromise()
{
  MOZ_ASSERT(NS_IsMainThread(), "Only call on main thread!");
  return nsRefPtr<Promise>(mPromise).forget();
}

FileSystemParams
GetFileOrDirectoryTask::GetRequestParams(const nsString& aFileSystem) const
{
  MOZ_ASSERT(NS_IsMainThread(), "Only call on main thread!");
  return FileSystemGetFileOrDirectoryParams(aFileSystem, mTargetRealPath);
}

FileSystemResponseValue
GetFileOrDirectoryTask::GetSuccessRequestResult() const
{
  MOZ_ASSERT(NS_IsMainThread(), "Only call on main thread!");
  if (mIsDirectory) {
    return FileSystemDirectoryResponse(mTargetRealPath);
  }

  nsRefPtr<DOMFile> file = new DOMFile(mTargetFileImpl);
  BlobParent* actor = GetBlobParent(file);
  if (!actor) {
    return FileSystemErrorResponse(NS_ERROR_DOM_FILESYSTEM_UNKNOWN_ERR);
  }
  FileSystemFileResponse response;
  response.blobParent() = actor;
  return response;
}

void
GetFileOrDirectoryTask::SetSuccessRequestResult(const FileSystemResponseValue& aValue)
{
  MOZ_ASSERT(NS_IsMainThread(), "Only call on main thread!");
  switch (aValue.type()) {
    case FileSystemResponseValue::TFileSystemFileResponse: {
      FileSystemFileResponse r = aValue;
      BlobChild* actor = static_cast<BlobChild*>(r.blobChild());
      nsCOMPtr<nsIDOMBlob> blob = actor->GetBlob();
      mTargetFileImpl = static_cast<DOMFile*>(blob.get())->Impl();
      mIsDirectory = false;
      break;
    }
    case FileSystemResponseValue::TFileSystemDirectoryResponse: {
      FileSystemDirectoryResponse r = aValue;
      mTargetRealPath = r.realPath();
      mIsDirectory = true;
      break;
    }
    default: {
      NS_RUNTIMEABORT("not reached");
      break;
    }
  }
}

nsresult
GetFileOrDirectoryTask::Work()
{
  MOZ_ASSERT(FileSystemUtils::IsParentProcess(),
             "Only call from parent process!");
  MOZ_ASSERT(!NS_IsMainThread(), "Only call on worker thread!");

  if (mFileSystem->IsShutdown()) {
    return NS_ERROR_FAILURE;
  }

  
  bool getRoot = mTargetRealPath.IsEmpty();

  nsCOMPtr<nsIFile> file = mFileSystem->GetLocalFile(mTargetRealPath);
  if (!file) {
    return NS_ERROR_DOM_FILESYSTEM_INVALID_PATH_ERR;
  }

  bool exists;
  nsresult rv = file->Exists(&exists);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  if (!exists) {
    if (!getRoot) {
      return NS_ERROR_DOM_FILE_NOT_FOUND_ERR;
    }

    
    rv = file->Create(nsIFile::DIRECTORY_TYPE, 0777);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }
  }

  
  rv = file->IsDirectory(&mIsDirectory);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  if (mIsDirectory) {
    return NS_OK;
  }

  
  if (getRoot) {
    return NS_ERROR_DOM_FILESYSTEM_TYPE_MISMATCH_ERR;
  }

  bool isFile;
  
  rv = file->IsFile(&isFile);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  if (!isFile) {
    
    return NS_ERROR_DOM_FILESYSTEM_TYPE_MISMATCH_ERR;
  }

  if (!mFileSystem->IsSafeFile(file)) {
    return NS_ERROR_DOM_SECURITY_ERR;
  }

  mTargetFileImpl = new DOMFileImplFile(file);

  return NS_OK;
}

void
GetFileOrDirectoryTask::HandlerCallback()
{
  MOZ_ASSERT(NS_IsMainThread(), "Only call on main thread!");
  if (mFileSystem->IsShutdown()) {
    mPromise = nullptr;
    return;
  }

  if (HasError()) {
    nsRefPtr<DOMError> domError = new DOMError(mFileSystem->GetWindow(),
      mErrorValue);
    mPromise->MaybeRejectBrokenly(domError);
    mPromise = nullptr;
    return;
  }

  if (mIsDirectory) {
    nsRefPtr<Directory> dir = new Directory(mFileSystem, mTargetRealPath);
    mPromise->MaybeResolve(dir);
    mPromise = nullptr;
    return;
  }

  nsCOMPtr<nsIDOMFile> file = new DOMFile(mTargetFileImpl);
  mPromise->MaybeResolve(file);
  mPromise = nullptr;
}

void
GetFileOrDirectoryTask::GetPermissionAccessType(nsCString& aAccess) const
{
  aAccess.AssignLiteral("read");
}

} 
} 
