





#include "FileBlockCache.h"
#include "VideoUtils.h"
#include "prio.h"
#include <algorithm>

namespace mozilla {

nsresult FileBlockCache::Open(PRFileDesc* aFD)
{
  NS_ASSERTION(NS_IsMainThread(), "Only call on main thread");
  NS_ENSURE_TRUE(aFD != nullptr, NS_ERROR_FAILURE);
  {
    MonitorAutoLock mon(mFileMonitor);
    mFD = aFD;
  }
  {
    MonitorAutoLock mon(mDataMonitor);
    nsresult res = NS_NewThread(getter_AddRefs(mThread),
                                nullptr,
                                MEDIA_THREAD_STACK_SIZE);
    mIsOpen = NS_SUCCEEDED(res);
    return res;
  }
}

FileBlockCache::FileBlockCache()
  : mFileMonitor("MediaCache.Writer.IO.Monitor"),
    mFD(nullptr),
    mFDCurrentPos(0),
    mDataMonitor("MediaCache.Writer.Data.Monitor"),
    mIsWriteScheduled(false),
    mIsOpen(false)
{
  MOZ_COUNT_CTOR(FileBlockCache);
}

FileBlockCache::~FileBlockCache()
{
  NS_ASSERTION(!mIsOpen, "Should Close() FileBlockCache before destroying");
  {
    
    
    MonitorAutoLock mon(mFileMonitor);
    if (mFD) {
      PRStatus prrc;
      prrc = PR_Close(mFD);
      if (prrc != PR_SUCCESS) {
        NS_WARNING("PR_Close() failed.");
      }
      mFD = nullptr;
    }
  }
  MOZ_COUNT_DTOR(FileBlockCache);
}


void FileBlockCache::Close()
{
  NS_ASSERTION(NS_IsMainThread(), "Only call on main thread");
  MonitorAutoLock mon(mDataMonitor);

  mIsOpen = false;

  if (mThread) {
    
    
    
    
    
    
    nsCOMPtr<nsIRunnable> event = new ShutdownThreadEvent(mThread);
    mThread = nullptr;
    NS_DispatchToMainThread(event);
  }
}

nsresult FileBlockCache::WriteBlock(uint32_t aBlockIndex, const uint8_t* aData)
{
  MonitorAutoLock mon(mDataMonitor);

  if (!mIsOpen)
    return NS_ERROR_FAILURE;

  
  mBlockChanges.EnsureLengthAtLeast(aBlockIndex + 1);
  bool blockAlreadyHadPendingChange = mBlockChanges[aBlockIndex] != nullptr;
  mBlockChanges[aBlockIndex] = new BlockChange(aData);

  if (!blockAlreadyHadPendingChange || !mChangeIndexList.Contains(aBlockIndex)) {
    
    
    
    
    
    
    mChangeIndexList.PushBack(aBlockIndex);
  }
  NS_ASSERTION(mChangeIndexList.Contains(aBlockIndex), "Must have entry for new block");

  EnsureWriteScheduled();

  return NS_OK;
}

void FileBlockCache::EnsureWriteScheduled()
{
  mDataMonitor.AssertCurrentThreadOwns();

  if (!mIsWriteScheduled) {
    mThread->Dispatch(this, NS_DISPATCH_NORMAL);
    mIsWriteScheduled = true;
  }
}

nsresult FileBlockCache::Seek(int64_t aOffset)
{
  mFileMonitor.AssertCurrentThreadOwns();

  if (mFDCurrentPos != aOffset) {
    int64_t result = PR_Seek64(mFD, aOffset, PR_SEEK_SET);
    if (result != aOffset) {
      NS_WARNING("Failed to seek media cache file");
      return NS_ERROR_FAILURE;
    }
    mFDCurrentPos = result;
  }
  return NS_OK;
}

nsresult FileBlockCache::ReadFromFile(int64_t aOffset,
                                      uint8_t* aDest,
                                      int32_t aBytesToRead,
                                      int32_t& aBytesRead)
{
  mFileMonitor.AssertCurrentThreadOwns();

  nsresult res = Seek(aOffset);
  if (NS_FAILED(res)) return res;

  aBytesRead = PR_Read(mFD, aDest, aBytesToRead);
  if (aBytesRead <= 0)
    return NS_ERROR_FAILURE;
  mFDCurrentPos += aBytesRead;

  return NS_OK;
}

nsresult FileBlockCache::WriteBlockToFile(int32_t aBlockIndex,
                                          const uint8_t* aBlockData)
{
  mFileMonitor.AssertCurrentThreadOwns();

  nsresult rv = Seek(BlockIndexToOffset(aBlockIndex));
  if (NS_FAILED(rv)) return rv;

  int32_t amount = PR_Write(mFD, aBlockData, BLOCK_SIZE);
  if (amount < BLOCK_SIZE) {
    NS_WARNING("Failed to write media cache block!");
    return NS_ERROR_FAILURE;
  }
  mFDCurrentPos += BLOCK_SIZE;

  return NS_OK;
}

nsresult FileBlockCache::MoveBlockInFile(int32_t aSourceBlockIndex,
                                         int32_t aDestBlockIndex)
{
  mFileMonitor.AssertCurrentThreadOwns();

  uint8_t buf[BLOCK_SIZE];
  int32_t bytesRead = 0;
  if (NS_FAILED(ReadFromFile(BlockIndexToOffset(aSourceBlockIndex),
                             buf,
                             BLOCK_SIZE,
                             bytesRead))) {
    return NS_ERROR_FAILURE;
  }
  return WriteBlockToFile(aDestBlockIndex, buf);
}

nsresult FileBlockCache::Run()
{
  MonitorAutoLock mon(mDataMonitor);
  NS_ASSERTION(!NS_IsMainThread(), "Don't call on main thread");
  NS_ASSERTION(!mChangeIndexList.IsEmpty(), "Only dispatch when there's work to do");
  NS_ASSERTION(mIsWriteScheduled, "Should report write running or scheduled.");

  while (!mChangeIndexList.IsEmpty()) {
    if (!mIsOpen) {
      
      mIsWriteScheduled = false;
      return NS_ERROR_FAILURE;
    }

    
    
    
    
    
    
    

    
    
    
    int32_t blockIndex = mChangeIndexList.PopFront();
    nsRefPtr<BlockChange> change = mBlockChanges[blockIndex];
    NS_ABORT_IF_FALSE(change,
      "Change index list should only contain entries for blocks with changes");
    {
      MonitorAutoUnlock unlock(mDataMonitor);
      MonitorAutoLock lock(mFileMonitor);
      if (change->IsWrite()) {
        WriteBlockToFile(blockIndex, change->mData.get());
      } else if (change->IsMove()) {
        MoveBlockInFile(change->mSourceBlockIndex, blockIndex);
      }
    }
    
    
    
    if (mBlockChanges[blockIndex] == change) {
      mBlockChanges[blockIndex] = nullptr;
    }
  }

  mIsWriteScheduled = false;

  return NS_OK;
}

nsresult FileBlockCache::Read(int64_t aOffset,
                              uint8_t* aData,
                              int32_t aLength,
                              int32_t* aBytes)
{
  MonitorAutoLock mon(mDataMonitor);

  if (!mFD || (aOffset / BLOCK_SIZE) > INT32_MAX)
    return NS_ERROR_FAILURE;

  int32_t bytesToRead = aLength;
  int64_t offset = aOffset;
  uint8_t* dst = aData;
  while (bytesToRead > 0) {
    int32_t blockIndex = static_cast<int32_t>(offset / BLOCK_SIZE);
    int32_t start = offset % BLOCK_SIZE;
    int32_t amount = std::min(BLOCK_SIZE - start, bytesToRead);

    
    
    int32_t bytesRead = 0;
    nsRefPtr<BlockChange> change = mBlockChanges[blockIndex];
    if (change && change->IsWrite()) {
      
      const uint8_t* blockData = change->mData.get();
      memcpy(dst, blockData + start, amount);
      bytesRead = amount;
    } else {
      if (change && change->IsMove()) {
        
        
        
        
        
        
        blockIndex = mBlockChanges[blockIndex]->mSourceBlockIndex;
      }
      
      
      
      nsresult res;
      {
        MonitorAutoUnlock unlock(mDataMonitor);
        MonitorAutoLock lock(mFileMonitor);
        res = ReadFromFile(BlockIndexToOffset(blockIndex) + start,
                           dst,
                           amount,
                           bytesRead);
      }
      NS_ENSURE_SUCCESS(res,res);
    }
    dst += bytesRead;
    offset += bytesRead;
    bytesToRead -= bytesRead;
  }
  *aBytes = aLength - bytesToRead;
  return NS_OK;
}

nsresult FileBlockCache::MoveBlock(int32_t aSourceBlockIndex, int32_t aDestBlockIndex)
{
  NS_ASSERTION(NS_IsMainThread(), "Only call on main thread");
  MonitorAutoLock mon(mDataMonitor);

  if (!mIsOpen)
    return NS_ERROR_FAILURE;

  mBlockChanges.EnsureLengthAtLeast(std::max(aSourceBlockIndex, aDestBlockIndex) + 1);

  
  
  
  
  
  int32_t sourceIndex = aSourceBlockIndex;
  BlockChange* sourceBlock = nullptr;
  while ((sourceBlock = mBlockChanges[sourceIndex]) &&
          sourceBlock->IsMove()) {
    sourceIndex = sourceBlock->mSourceBlockIndex;
  }

  if (mBlockChanges[aDestBlockIndex] == nullptr ||
      !mChangeIndexList.Contains(aDestBlockIndex)) {
    
    
    
    
    
    
    mChangeIndexList.PushBack(aDestBlockIndex);
  }

  
  
  if (sourceBlock && sourceBlock->IsWrite()) {
    mBlockChanges[aDestBlockIndex] = new BlockChange(sourceBlock->mData.get());
  } else {
    mBlockChanges[aDestBlockIndex] = new BlockChange(sourceIndex);
  }

  EnsureWriteScheduled();

  NS_ASSERTION(mChangeIndexList.Contains(aDestBlockIndex),
    "Should have scheduled block for change");

  return NS_OK;
}

} 
