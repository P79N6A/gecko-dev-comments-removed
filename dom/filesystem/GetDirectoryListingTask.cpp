





#include "GetDirectoryListingTask.h"

#include "js/Value.h"
#include "mozilla/dom/Directory.h"
#include "mozilla/dom/DOMError.h"
#include "mozilla/dom/File.h"
#include "mozilla/dom/FileSystemBase.h"
#include "mozilla/dom/FileSystemUtils.h"
#include "mozilla/dom/Promise.h"
#include "mozilla/dom/ipc/BlobChild.h"
#include "mozilla/dom/ipc/BlobParent.h"
#include "nsIFile.h"
#include "nsStringGlue.h"

namespace mozilla {
namespace dom {

GetDirectoryListingTask::GetDirectoryListingTask(FileSystemBase* aFileSystem,
                                                 const nsAString& aTargetPath,
                                                 ErrorResult& aRv)
  : FileSystemTaskBase(aFileSystem)
  , mTargetRealPath(aTargetPath)
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

GetDirectoryListingTask::GetDirectoryListingTask(FileSystemBase* aFileSystem,
                                                 const FileSystemGetDirectoryListingParams& aParam,
                                                 FileSystemRequestParent* aParent)
  : FileSystemTaskBase(aFileSystem, aParam, aParent)
{
  MOZ_ASSERT(FileSystemUtils::IsParentProcess(),
             "Only call from parent process!");
  MOZ_ASSERT(NS_IsMainThread(), "Only call on main thread!");
  MOZ_ASSERT(aFileSystem);
  mTargetRealPath = aParam.realPath();
}

GetDirectoryListingTask::~GetDirectoryListingTask()
{
  MOZ_ASSERT(!mPromise || NS_IsMainThread(),
             "mPromise should be released on main thread!");
}

already_AddRefed<Promise>
GetDirectoryListingTask::GetPromise()
{
  MOZ_ASSERT(NS_IsMainThread(), "Only call on main thread!");
  return nsRefPtr<Promise>(mPromise).forget();
}

FileSystemParams
GetDirectoryListingTask::GetRequestParams(const nsString& aFileSystem) const
{
  MOZ_ASSERT(NS_IsMainThread(), "Only call on main thread!");
  return FileSystemGetDirectoryListingParams(aFileSystem, mTargetRealPath);
}

FileSystemResponseValue
GetDirectoryListingTask::GetSuccessRequestResult() const
{
  MOZ_ASSERT(NS_IsMainThread(), "Only call on main thread!");

  InfallibleTArray<PBlobParent*> blobs;

  for (unsigned i = 0; i < mTargetBlobImpls.Length(); i++) {
    BlobParent* blobParent = GetBlobParent(mTargetBlobImpls[i]);
    if (blobParent) {
      blobs.AppendElement(blobParent);
    }
  }
  FileSystemDirectoryListingResponse response;
  response.blobsParent().SwapElements(blobs);
  return response;
}

void
GetDirectoryListingTask::SetSuccessRequestResult(const FileSystemResponseValue& aValue)
{
  MOZ_ASSERT(NS_IsMainThread(), "Only call on main thread!");
  MOZ_ASSERT(aValue.type() ==
               FileSystemResponseValue::TFileSystemDirectoryListingResponse);

  FileSystemDirectoryListingResponse r = aValue;
  nsTArray<PBlobChild*>& blobs = r.blobsChild();

  for (unsigned i = 0; i < blobs.Length(); i++) {
    mTargetBlobImpls.AppendElement(static_cast<BlobChild*>(blobs[i])->GetBlobImpl());
  }
}

nsresult
GetDirectoryListingTask::Work()
{
  MOZ_ASSERT(FileSystemUtils::IsParentProcess(),
             "Only call from parent process!");
  MOZ_ASSERT(!NS_IsMainThread(), "Only call on worker thread!");

  if (mFileSystem->IsShutdown()) {
    return NS_ERROR_FAILURE;
  }

  
  bool getRoot = mTargetRealPath.IsEmpty();

  nsCOMPtr<nsIFile> dir = mFileSystem->GetLocalFile(mTargetRealPath);
  if (!dir) {
    return NS_ERROR_DOM_FILESYSTEM_INVALID_PATH_ERR;
  }

  bool exists;
  nsresult rv = dir->Exists(&exists);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  if (!exists) {
    if (!getRoot) {
      return NS_ERROR_DOM_FILE_NOT_FOUND_ERR;
    }

    
    rv = dir->Create(nsIFile::DIRECTORY_TYPE, 0777);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }
  }

  
  bool isDir;
  rv = dir->IsDirectory(&isDir);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  if (!isDir) {
    return NS_ERROR_DOM_FILESYSTEM_TYPE_MISMATCH_ERR;
  }

  nsCOMPtr<nsISimpleEnumerator> entries;
  rv = dir->GetDirectoryEntries(getter_AddRefs(entries));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  for (;;) {
    bool hasMore = false;
    if (NS_WARN_IF(NS_FAILED(entries->HasMoreElements(&hasMore))) || !hasMore) {
      break;
    }
    nsCOMPtr<nsISupports> supp;
    if (NS_WARN_IF(NS_FAILED(entries->GetNext(getter_AddRefs(supp))))) {
      break;
    }

    nsCOMPtr<nsIFile> currFile = do_QueryInterface(supp);
    MOZ_ASSERT(currFile);
    
    bool isLink, isSpecial, isFile;
    if (NS_WARN_IF(NS_FAILED(currFile->IsSymlink(&isLink)) ||
                   NS_FAILED(currFile->IsSpecial(&isSpecial))) ||
        isLink || isSpecial) {
      continue;
    };
    if (NS_WARN_IF(NS_FAILED(currFile->IsFile(&isFile)) ||
                   NS_FAILED(currFile->IsDirectory(&isDir))) ||
        !(isFile || isDir)) {
      continue;
    }
    BlobImplFile* impl = new BlobImplFile(currFile);
    impl->LookupAndCacheIsDirectory();
    mTargetBlobImpls.AppendElement(impl);
  }
  return NS_OK;
}

void
GetDirectoryListingTask::HandlerCallback()
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

  size_t count = mTargetBlobImpls.Length();

  Sequence<OwningFileOrDirectory> listing;

  if (!listing.SetLength(count, mozilla::fallible_t())) {
    mPromise->MaybeReject(NS_ERROR_FAILURE);
    mPromise = nullptr;
    return;
  }

  for (unsigned i = 0; i < count; i++) {
    if (mTargetBlobImpls[i]->IsDirectory()) {
      nsAutoString name;
      mTargetBlobImpls[i]->GetName(name);
      nsAutoString path(mTargetRealPath);
      path.AppendLiteral(FILESYSTEM_DOM_PATH_SEPARATOR);
      path.Append(name);
#ifdef DEBUG
      if (FileSystemUtils::IsParentProcess()) {
        nsCOMPtr<nsIFile> file = mFileSystem->GetLocalFile(path);
        bool exist;
        file->Exists(&exist);
        MOZ_ASSERT(exist);
      }
#endif
      listing[i].SetAsDirectory() = new Directory(mFileSystem, path);
    } else {
      listing[i].SetAsFile() = File::Create(mFileSystem->GetWindow(), mTargetBlobImpls[i]);
    }
  }

  mPromise->MaybeResolve(listing);
  mPromise = nullptr;
}

void
GetDirectoryListingTask::GetPermissionAccessType(nsCString& aAccess) const
{
  aAccess.AssignLiteral("read");
}

} 
} 
