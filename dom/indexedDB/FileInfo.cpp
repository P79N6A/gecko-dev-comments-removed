





































#include "IndexedDatabaseManager.h"
#include "FileInfo.h"

USING_INDEXEDDB_NAMESPACE


FileInfo*
FileInfo::Create(FileManager* aFileManager, PRInt64 aId)
{
  NS_ASSERTION(aId > 0, "Wrong id!");

  if (aId <= PR_INT16_MAX) {
    return new FileInfo16(aFileManager, aId);
  }

  if (aId <= PR_INT32_MAX) {
    return new FileInfo32(aFileManager, aId);
  }

  return new FileInfo64(aFileManager, aId);
}

void
FileInfo::GetReferences(PRInt32* aRefCnt, PRInt32* aDBRefCnt,
                        PRInt32* aSliceRefCnt)
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
FileInfo::UpdateReferences(nsAutoRefCnt& aRefCount, PRInt32 aDelta,
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
  if (IndexedDatabaseManager::IsShuttingDown() ||
      mFileManager->Invalidated()) {
    return;
  }

  nsRefPtr<IndexedDatabaseManager> mgr = IndexedDatabaseManager::Get();
  NS_ASSERTION(mgr, "Shouldn't be null!");

  if (NS_FAILED(mgr->AsyncDeleteFile(mFileManager, Id()))) {
    NS_WARNING("Failed to delete file asynchronously!");
  }
}
