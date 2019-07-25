





#include "mozilla/XPCOM.h"
#include "FileBlockCache.h"
#include "VideoUtils.h"

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
  : mFileMonitor("nsMediaCache.Writer.IO.Monitor"),
    mFD(nullptr),
    mFDCurrentPos(0),
    mDataMonitor("nsMediaCache.Writer.Data.Monitor"),
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
      PR_Close(mFD);
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

nsresult FileBlockCache::WriteBlock(PRUint32 aBlockIndex, const PRUint8* aData)
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

nsresult FileBlockCache::Seek(PRInt64 aOffset)
{
  mFileMonitor.AssertCurrentThreadOwns();

  if (mFDCurrentPos != aOffset) {
    PROffset64 result = PR_Seek64(mFD, aOffset, PR_SEEK_SET);
    if (result != aOffset) {
      NS_WARNING("Failed to seek media cache file");
      return NS_ERROR_FAILURE;
    }
    mFDCurrentPos = result;
  }
  return NS_OK;
}

nsresult FileBlockCache::ReadFromFile(PRInt32 aOffset,
                                      PRUint8* aDest,
                                      PRInt32 aBytesToRead,
                                      PRInt32& aBytesRead)
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

nsresult FileBlockCache::WriteBlockToFile(PRInt32 aBlockIndex,
                                          const PRUint8* aBlockData)
{
  mFileMonitor.AssertCurrentThreadOwns();

  nsresult rv = Seek(aBlockIndex * BLOCK_SIZE);
  if (NS_FAILED(rv)) return rv;

  PRInt32 amount = PR_Write(mFD, aBlockData, BLOCK_SIZE);
  if (amount < BLOCK_SIZE) {
    NS_WARNING("Failed to write media cache block!");
    return NS_ERROR_FAILURE;
  }
  mFDCurrentPos += BLOCK_SIZE;

  return NS_OK;
}

nsresult FileBlockCache::MoveBlockInFile(PRInt32 aSourceBlockIndex,
                                         PRInt32 aDestBlockIndex)
{
  mFileMonitor.AssertCurrentThreadOwns();

  PRUint8 buf[BLOCK_SIZE];
  PRInt32 bytesRead = 0;
  if (NS_FAILED(ReadFromFile(aSourceBlockIndex * BLOCK_SIZE,
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

    
    
    
    
    
    
    

    
    
    
    PRInt32 blockIndex = mChangeIndexList.PopFront();
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

nsresult FileBlockCache::Read(PRInt64 aOffset,
                              PRUint8* aData,
                              PRInt32 aLength,
                              PRInt32* aBytes)
{
  MonitorAutoLock mon(mDataMonitor);

  if (!mFD || (aOffset / BLOCK_SIZE) > PR_INT32_MAX)
    return NS_ERROR_FAILURE;

  PRInt32 bytesToRead = aLength;
  PRInt64 offset = aOffset;
  PRUint8* dst = aData;
  while (bytesToRead > 0) {
    PRInt32 blockIndex = static_cast<PRInt32>(offset / BLOCK_SIZE);
    PRInt32 start = offset % BLOCK_SIZE;
    PRInt32 amount = NS_MIN(BLOCK_SIZE - start, bytesToRead);

    
    
    PRInt32 bytesRead = 0;
    nsRefPtr<BlockChange> change = mBlockChanges[blockIndex];
    if (change && change->IsWrite()) {
      
      const PRUint8* blockData = change->mData.get();
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
        res = ReadFromFile(blockIndex * BLOCK_SIZE + start,
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

nsresult FileBlockCache::MoveBlock(PRInt32 aSourceBlockIndex, PRInt32 aDestBlockIndex)
{
  NS_ASSERTION(NS_IsMainThread(), "Only call on main thread");
  MonitorAutoLock mon(mDataMonitor);

  if (!mIsOpen)
    return NS_ERROR_FAILURE;

  mBlockChanges.EnsureLengthAtLeast(NS_MAX(aSourceBlockIndex, aDestBlockIndex) + 1);

  
  
  
  
  
  PRInt32 sourceIndex = aSourceBlockIndex;
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
