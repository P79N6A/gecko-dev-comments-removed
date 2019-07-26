





#include "FileHandle.h"

#include "File.h"
#include "FileRequest.h"
#include "FileService.h"
#include "LockedFile.h"
#include "MetadataHelper.h"
#include "mozilla/Assertions.h"
#include "mozilla/dom/FileHandleBinding.h"
#include "mozilla/ErrorResult.h"
#include "nsAutoPtr.h"
#include "nsContentUtils.h"
#include "nsDebug.h"
#include "nsError.h"
#include "nsIDOMFile.h"
#include "nsIFile.h"
#include "nsIFileStorage.h"
#include "nsNetUtil.h"

namespace mozilla {
namespace dom {

namespace {

class GetFileHelper : public MetadataHelper
{
public:
  GetFileHelper(LockedFile* aLockedFile,
                FileRequest* aFileRequest,
                MetadataParameters* aParams,
                FileHandle* aFileHandle)
  : MetadataHelper(aLockedFile, aFileRequest, aParams),
    mFileHandle(aFileHandle)
  { }

  virtual nsresult
  GetSuccessResult(JSContext* aCx,
                   JS::MutableHandle<JS::Value> aVal) MOZ_OVERRIDE;

  virtual void
  ReleaseObjects() MOZ_OVERRIDE
  {
    mFileHandle = nullptr;

    MetadataHelper::ReleaseObjects();
  }

private:
  nsRefPtr<FileHandle> mFileHandle;
};

} 

FileHandle::FileHandle(nsPIDOMWindow* aWindow)
  : DOMEventTargetHelper(aWindow)
{
}

FileHandle::FileHandle(DOMEventTargetHelper* aOwner)
  : DOMEventTargetHelper(aOwner)
{
}

FileHandle::~FileHandle()
{
}

NS_IMPL_CYCLE_COLLECTION_INHERITED(FileHandle, DOMEventTargetHelper,
                                   mFileStorage)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(FileHandle)
NS_INTERFACE_MAP_END_INHERITING(DOMEventTargetHelper)

NS_IMPL_ADDREF_INHERITED(FileHandle, DOMEventTargetHelper)
NS_IMPL_RELEASE_INHERITED(FileHandle, DOMEventTargetHelper)


already_AddRefed<FileHandle>
FileHandle::Create(nsPIDOMWindow* aWindow,
                   nsIFileStorage* aFileStorage,
                   nsIFile* aFile)
{
  MOZ_ASSERT(NS_IsMainThread(), "Wrong thread!");

  nsRefPtr<FileHandle> newFileHandle = new FileHandle(aWindow);

  newFileHandle->mFileStorage = aFileStorage;
  nsresult rv = aFile->GetLeafName(newFileHandle->mName);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return nullptr;
  }

  newFileHandle->mFile = aFile;
  newFileHandle->mFileName = newFileHandle->mName;

  return newFileHandle.forget();
}


already_AddRefed<nsISupports>
FileHandle::CreateStream(nsIFile* aFile, bool aReadOnly)
{
  nsresult rv;

  if (aReadOnly) {
    nsCOMPtr<nsIInputStream> stream;
    rv = NS_NewLocalFileInputStream(getter_AddRefs(stream), aFile, -1, -1,
                                    nsIFileInputStream::DEFER_OPEN);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return nullptr;
    }
    return stream.forget();
  }

  nsCOMPtr<nsIFileStream> stream;
  rv = NS_NewLocalFileStream(getter_AddRefs(stream), aFile, -1, -1,
                             nsIFileStream::DEFER_OPEN);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return nullptr;
  }
  return stream.forget();
}


already_AddRefed<nsIDOMFile>
FileHandle::CreateFileObject(LockedFile* aLockedFile, uint32_t aFileSize)
{
  nsCOMPtr<nsIDOMFile> file =
    new File(mName, mType, aFileSize, mFile, aLockedFile);

  return file.forget();
}


JSObject*
FileHandle::WrapObject(JSContext* aCx)
{
  return FileHandleBinding::Wrap(aCx, this);
}

already_AddRefed<LockedFile>
FileHandle::Open(FileMode aMode, ErrorResult& aError)
{
  MOZ_ASSERT(NS_IsMainThread());

  if (FileService::IsShuttingDown() || mFileStorage->IsShuttingDown()) {
    aError.Throw(NS_ERROR_DOM_FILEHANDLE_UNKNOWN_ERR);
    return nullptr;
  }

  nsRefPtr<LockedFile> lockedFile = LockedFile::Create(this, aMode);
  if (!lockedFile) {
    aError.Throw(NS_ERROR_DOM_FILEHANDLE_UNKNOWN_ERR);
    return nullptr;
  }

  return lockedFile.forget();
}

already_AddRefed<DOMRequest>
FileHandle::GetFile(ErrorResult& aError)
{
  MOZ_ASSERT(NS_IsMainThread());

  
  if (!GetOwner()) {
    return nullptr;
  }

  nsRefPtr<LockedFile> lockedFile =
    LockedFile::Create(this, FileMode::Readonly, LockedFile::PARALLEL);
  if (!lockedFile) {
    aError.Throw(NS_ERROR_DOM_FILEHANDLE_UNKNOWN_ERR);
    return nullptr;
  }

  nsRefPtr<FileRequest> request =
    FileRequest::Create(GetOwner(), lockedFile,  true);

  nsRefPtr<MetadataParameters> params = new MetadataParameters(true, false);

  nsRefPtr<GetFileHelper> helper =
    new GetFileHelper(lockedFile, request, params, this);

  nsresult rv = helper->Enqueue();
  if (NS_FAILED(rv)) {
    aError.Throw(NS_ERROR_DOM_FILEHANDLE_UNKNOWN_ERR);
    return nullptr;
  }

  return request.forget();
}

nsresult
GetFileHelper::GetSuccessResult(JSContext* aCx,
                                JS::MutableHandle<JS::Value> aVal)
{
  nsCOMPtr<nsIDOMFile> domFile =
    mFileHandle->CreateFileObject(mLockedFile, mParams->Size());

  nsresult rv =
    nsContentUtils::WrapNative(aCx, domFile, &NS_GET_IID(nsIDOMFile), aVal);
  NS_ENSURE_SUCCESS(rv, NS_ERROR_DOM_FILEHANDLE_UNKNOWN_ERR);
  return NS_OK;
}

} 
} 
