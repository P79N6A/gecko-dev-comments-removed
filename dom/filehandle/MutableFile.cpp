





#include "MutableFile.h"

#include "File.h"
#include "FileHandle.h"
#include "FileRequest.h"
#include "FileService.h"
#include "MetadataHelper.h"
#include "mozilla/Assertions.h"
#include "mozilla/dom/MutableFileBinding.h"
#include "mozilla/ErrorResult.h"
#include "nsAutoPtr.h"
#include "nsContentUtils.h"
#include "nsDebug.h"
#include "nsError.h"
#include "nsIDOMFile.h"
#include "nsIFile.h"
#include "nsNetUtil.h"

namespace mozilla {
namespace dom {

namespace {

class GetFileHelper : public MetadataHelper
{
public:
  GetFileHelper(FileHandle* aFileHandle,
                FileRequest* aFileRequest,
                MetadataParameters* aParams,
                MutableFile* aMutableFile)
  : MetadataHelper(aFileHandle, aFileRequest, aParams),
    mMutableFile(aMutableFile)
  { }

  virtual nsresult
  GetSuccessResult(JSContext* aCx,
                   JS::MutableHandle<JS::Value> aVal) MOZ_OVERRIDE;

  virtual void
  ReleaseObjects() MOZ_OVERRIDE
  {
    mMutableFile = nullptr;

    MetadataHelper::ReleaseObjects();
  }

private:
  nsRefPtr<MutableFile> mMutableFile;
};

} 

MutableFile::MutableFile(nsPIDOMWindow* aWindow)
  : DOMEventTargetHelper(aWindow)
{
}

MutableFile::MutableFile(DOMEventTargetHelper* aOwner)
  : DOMEventTargetHelper(aOwner)
{
}

MutableFile::~MutableFile()
{
}

bool
MutableFile::IsShuttingDown()
{
  return FileService::IsShuttingDown();
}


already_AddRefed<nsISupports>
MutableFile::CreateStream(nsIFile* aFile, bool aReadOnly)
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
MutableFile::CreateFileObject(FileHandle* aFileHandle, uint32_t aFileSize)
{
  nsCOMPtr<nsIDOMFile> file =
    new File(mName, mType, aFileSize, mFile, aFileHandle);

  return file.forget();
}


JSObject*
MutableFile::WrapObject(JSContext* aCx)
{
  return MutableFileBinding::Wrap(aCx, this);
}

already_AddRefed<FileHandle>
MutableFile::Open(FileMode aMode, ErrorResult& aError)
{
  MOZ_ASSERT(NS_IsMainThread());

  if (IsShuttingDown()) {
    aError.Throw(NS_ERROR_DOM_FILEHANDLE_UNKNOWN_ERR);
    return nullptr;
  }

  nsRefPtr<FileHandle> fileHandle = FileHandle::Create(this, aMode);
  if (!fileHandle) {
    aError.Throw(NS_ERROR_DOM_FILEHANDLE_UNKNOWN_ERR);
    return nullptr;
  }

  return fileHandle.forget();
}

already_AddRefed<DOMRequest>
MutableFile::GetFile(ErrorResult& aError)
{
  MOZ_ASSERT(NS_IsMainThread());

  
  if (!GetOwner()) {
    return nullptr;
  }

  nsRefPtr<FileHandle> fileHandle =
    FileHandle::Create(this, FileMode::Readonly, FileHandle::PARALLEL);
  if (!fileHandle) {
    aError.Throw(NS_ERROR_DOM_FILEHANDLE_UNKNOWN_ERR);
    return nullptr;
  }

  nsRefPtr<FileRequest> request =
    FileRequest::Create(GetOwner(), fileHandle,  true);

  nsRefPtr<MetadataParameters> params = new MetadataParameters(true, false);

  nsRefPtr<GetFileHelper> helper =
    new GetFileHelper(fileHandle, request, params, this);

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
    mMutableFile->CreateFileObject(mFileHandle, mParams->Size());

  nsresult rv =
    nsContentUtils::WrapNative(aCx, domFile, &NS_GET_IID(nsIDOMFile), aVal);
  NS_ENSURE_SUCCESS(rv, NS_ERROR_DOM_FILEHANDLE_UNKNOWN_ERR);
  return NS_OK;
}

} 
} 
