





#include "File.h"

#include "FileHandle.h"
#include "mozilla/Assertions.h"
#include "nsDebug.h"

namespace mozilla {
namespace dom {

using indexedDB::IndexedDatabaseManager;

  
File::File(const nsAString& aName, const nsAString& aContentType,
           uint64_t aLength, nsIFile* aFile, FileHandle* aFileHandle)
: nsDOMFileCC(aName, aContentType, aLength),
  mFile(aFile), mFileHandle(aFileHandle),
  mWholeFile(true), mStoredFile(false)
{
  MOZ_ASSERT(mFile, "Null file!");
  MOZ_ASSERT(mFileHandle, "Null file handle!");
}


File::File(const nsAString& aName, const nsAString& aContentType,
           uint64_t aLength, nsIFile* aFile, FileHandle* aFileHandle,
           FileInfo* aFileInfo)
: nsDOMFileCC(aName, aContentType, aLength),
  mFile(aFile), mFileHandle(aFileHandle),
  mWholeFile(true), mStoredFile(true)
{
  MOZ_ASSERT(mFile, "Null file!");
  MOZ_ASSERT(mFileHandle, "Null file handle!");
  mFileInfos.AppendElement(aFileInfo);
}


File::File(const File* aOther, uint64_t aStart, uint64_t aLength,
           const nsAString& aContentType)
: nsDOMFileCC(aContentType, aOther->mStart + aStart, aLength),
  mFile(aOther->mFile), mFileHandle(aOther->mFileHandle),
  mWholeFile(false), mStoredFile(aOther->mStoredFile)
{
  MOZ_ASSERT(mFile, "Null file!");
  MOZ_ASSERT(mFileHandle, "Null file handle!");

  if (mStoredFile) {
    FileInfo* fileInfo;

    if (IndexedDatabaseManager::IsClosed()) {
      fileInfo = aOther->GetFileInfo();
    }
    else {
      MutexAutoLock lock(IndexedDatabaseManager::FileMutex());
      fileInfo = aOther->GetFileInfo();
    }

    mFileInfos.AppendElement(fileInfo);
  }
}

File::~File()
{
}

NS_IMPL_CYCLE_COLLECTION_INHERITED(File, nsDOMFileCC,
                                   mFileHandle)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(File)
NS_INTERFACE_MAP_END_INHERITING(nsDOMFileCC)

NS_IMPL_ADDREF_INHERITED(File, nsDOMFileCC)
NS_IMPL_RELEASE_INHERITED(File, nsDOMFileCC)

NS_IMETHODIMP
File::GetInternalStream(nsIInputStream **aStream)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  nsresult rv = mFileHandle->OpenInputStream(mWholeFile, mStart, mLength,
                                             aStream);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

already_AddRefed<nsIDOMBlob>
File::CreateSlice(uint64_t aStart, uint64_t aLength,
                  const nsAString& aContentType)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  nsCOMPtr<nsIDOMBlob> t =
    new File(this, aStart, aLength, aContentType);
  return t.forget();
}

NS_IMETHODIMP
File::GetMozFullPathInternal(nsAString &aFilename)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(mIsFile, "Should only be called on files");

  return mFile->GetPath(aFilename);
}

} 
} 
