





#include "File.h"

#include "LockedFile.h"
#include "mozilla/Assertions.h"
#include "nsDebug.h"

namespace mozilla {
namespace dom {

using indexedDB::IndexedDatabaseManager;

  
File::File(const nsAString& aName, const nsAString& aContentType,
           uint64_t aLength, nsIFile* aFile, LockedFile* aLockedFile)
: nsDOMFileCC(aName, aContentType, aLength),
  mFile(aFile), mLockedFile(aLockedFile),
  mWholeFile(true), mStoredFile(false)
{
  MOZ_ASSERT(mFile, "Null file!");
  MOZ_ASSERT(mLockedFile, "Null locked file!");
}


File::File(const nsAString& aName, const nsAString& aContentType,
           uint64_t aLength, nsIFile* aFile, LockedFile* aLockedFile,
           FileInfo* aFileInfo)
: nsDOMFileCC(aName, aContentType, aLength),
  mFile(aFile), mLockedFile(aLockedFile),
  mWholeFile(true), mStoredFile(true)
{
  MOZ_ASSERT(mFile, "Null file!");
  MOZ_ASSERT(mLockedFile, "Null locked file!");
  mFileInfos.AppendElement(aFileInfo);
}


File::File(const File* aOther, uint64_t aStart, uint64_t aLength,
           const nsAString& aContentType)
: nsDOMFileCC(aContentType, aOther->mStart + aStart, aLength),
  mFile(aOther->mFile), mLockedFile(aOther->mLockedFile),
  mWholeFile(false), mStoredFile(aOther->mStoredFile)
{
  NS_ASSERTION(mFile, "Null file!");
  NS_ASSERTION(mLockedFile, "Null locked file!");

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
                                   mLockedFile)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(File)
NS_INTERFACE_MAP_END_INHERITING(nsDOMFileCC)

NS_IMPL_ADDREF_INHERITED(File, nsDOMFileCC)
NS_IMPL_RELEASE_INHERITED(File, nsDOMFileCC)

NS_IMETHODIMP
File::GetInternalStream(nsIInputStream **aStream)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  nsresult rv = mLockedFile->OpenInputStream(mWholeFile, mStart, mLength,
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
