





#include "FileInfo.h"

#include "mozilla/dom/quota/QuotaManager.h"

USING_INDEXEDDB_NAMESPACE


FileInfo*
FileInfo::Create(FileManager* aFileManager, int64_t aId)
{
  NS_ASSERTION(aId > 0, "Wrong id!");

  if (aId <= INT16_MAX) {
    return new FileInfo16(aFileManager, aId);
  }

  if (aId <= INT32_MAX) {
    return new FileInfo32(aFileManager, aId);
  }

  return new FileInfo64(aFileManager, aId);
}

void
FileInfo::GetReferences(int32_t* aRefCnt, int32_t* aDBRefCnt,
                        int32_t* aSliceRefCnt)
{
  if (IndexedDatabaseManager::IsClosed()) {
    NS_ERROR("Shouldn't be called after shutdown!");

    if (aRefCnt) {
      *aRefCnt = -1;
    }

    if (aDBRefCnt) {
      *aDBRefCnt = -1;
    }

    if (aSliceRefCnt) {
      *aSliceRefCnt = -1;
    }

    return;
  }

  MutexAutoLock lock(IndexedDatabaseManager::FileMutex());

  if (aRefCnt) {
    *aRefCnt = mRefCnt;
  }

  if (aDBRefCnt) {
    *aDBRefCnt = mDBRefCnt;
  }

  if (aSliceRefCnt) {
    *aSliceRefCnt = mSliceRefCnt;
  }
}

void
FileInfo::UpdateReferences(nsAutoRefCnt& aRefCount, int32_t aDelta,
                           bool aClear)
{
  if (IndexedDatabaseManager::IsClosed()) {
    NS_ERROR("Shouldn't be called after shutdown!");
    return;
  }

  {
    MutexAutoLock lock(IndexedDatabaseManager::FileMutex());

    aRefCount = aClear ? 0 : aRefCount + aDelta;

    if (mRefCnt + mDBRefCnt + mSliceRefCnt > 0) {
      return;
    }

    mFileManager->mFileInfos.Remove(Id());
  }

  Cleanup();

  delete this;
}

void
FileInfo::Cleanup()
{
  if (quota::QuotaManager::IsShuttingDown() ||
      mFileManager->Invalidated()) {
    return;
  }

  nsRefPtr<IndexedDatabaseManager> mgr = IndexedDatabaseManager::Get();
  NS_ASSERTION(mgr, "Shouldn't be null!");

  if (NS_FAILED(mgr->AsyncDeleteFile(mFileManager, Id()))) {
    NS_WARNING("Failed to delete file asynchronously!");
  }
}
