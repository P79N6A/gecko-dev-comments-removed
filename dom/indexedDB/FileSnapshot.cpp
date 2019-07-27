





#include "FileSnapshot.h"

#include "IDBFileHandle.h"
#include "MainThreadUtils.h"
#include "mozilla/Assertions.h"
#include "mozilla/dom/MetadataHelper.h"

#ifdef DEBUG
#include "nsXULAppAPI.h"
#endif

namespace mozilla {
namespace dom {
namespace indexedDB {


FileImplSnapshot::FileImplSnapshot(const nsAString& aName,
                                   const nsAString& aContentType,
                                   MetadataParameters* aMetadataParams,
                                   nsIFile* aFile,
                                   IDBFileHandle* aFileHandle,
                                   FileInfo* aFileInfo)
  : DOMFileImplBase(aName,
                    aContentType,
                    aMetadataParams->Size(),
                    aMetadataParams->LastModified())
  , mFile(aFile)
  , mFileHandle(aFileHandle)
  , mWholeFile(true)
{
  AssertSanity();
  MOZ_ASSERT(aMetadataParams);
  MOZ_ASSERT(aMetadataParams->Size() != UINT64_MAX);
  MOZ_ASSERT(aMetadataParams->LastModified() != INT64_MAX);
  MOZ_ASSERT(aFile);
  MOZ_ASSERT(aFileHandle);
  MOZ_ASSERT(aFileInfo);

  mFileInfos.AppendElement(aFileInfo);
}


FileImplSnapshot::FileImplSnapshot(const FileImplSnapshot* aOther,
                                   uint64_t aStart,
                                   uint64_t aLength,
                                   const nsAString& aContentType)
  : DOMFileImplBase(aContentType, aOther->mStart + aStart, aLength)
  , mFile(aOther->mFile)
  , mFileHandle(aOther->mFileHandle)
  , mWholeFile(false)
{
  AssertSanity();
  MOZ_ASSERT(aOther);

  FileInfo* fileInfo;

  if (IndexedDatabaseManager::IsClosed()) {
    fileInfo = aOther->GetFileInfo();
  } else {
    MutexAutoLock lock(IndexedDatabaseManager::FileMutex());
    fileInfo = aOther->GetFileInfo();
  }

  mFileInfos.AppendElement(fileInfo);
}

FileImplSnapshot::~FileImplSnapshot()
{
}

#ifdef DEBUG


void
FileImplSnapshot::AssertSanity()
{
  MOZ_ASSERT(XRE_GetProcessType() == GeckoProcessType_Default);
  MOZ_ASSERT(NS_IsMainThread());
}

#endif 

NS_IMPL_ISUPPORTS_INHERITED0(FileImplSnapshot, DOMFileImpl)

void
FileImplSnapshot::Unlink()
{
  AssertSanity();

  FileImplSnapshot* tmp = this;
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mFileHandle);
}

void
FileImplSnapshot::Traverse(nsCycleCollectionTraversalCallback &cb)
{
  AssertSanity();

  FileImplSnapshot* tmp = this;
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mFileHandle);
}

bool
FileImplSnapshot::IsCCed() const
{
  AssertSanity();

  return true;
}

nsresult
FileImplSnapshot::GetInternalStream(nsIInputStream** aStream)
{
  AssertSanity();

  nsresult rv = mFileHandle->OpenInputStream(mWholeFile, mStart, mLength,
                                             aStream);
  if (NS_FAILED(rv)) {
    return rv;
  }

  return NS_OK;
}

already_AddRefed<DOMFileImpl>
FileImplSnapshot::CreateSlice(uint64_t aStart,
                              uint64_t aLength,
                              const nsAString& aContentType)
{
  AssertSanity();

  nsRefPtr<DOMFileImpl> impl =
    new FileImplSnapshot(this, aStart, aLength, aContentType);

  return impl.forget();
}

nsresult
FileImplSnapshot::GetMozFullPathInternal(nsAString& aFilename)
{
  AssertSanity();
  MOZ_ASSERT(mIsFile);

  return mFile->GetPath(aFilename);
}

bool
FileImplSnapshot::IsStoredFile() const
{
  AssertSanity();

  return true;
}

bool
FileImplSnapshot::IsWholeFile() const
{
  AssertSanity();

  return mWholeFile;
}

bool
FileImplSnapshot::IsSnapshot() const
{
  AssertSanity();

  return true;
}

} 
} 
} 
