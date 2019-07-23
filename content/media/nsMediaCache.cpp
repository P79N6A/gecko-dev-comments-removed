 





































#include "mozilla/XPCOM.h"

#include "nsMediaCache.h"
#include "nsAutoLock.h"
#include "nsContentUtils.h"
#include "nsDirectoryServiceUtils.h"
#include "nsDirectoryServiceDefs.h"
#include "nsNetUtil.h"
#include "prio.h"
#include "nsThreadUtils.h"
#include "nsMediaStream.h"
#include "nsMathUtils.h"
#include "prlog.h"

#ifdef PR_LOGGING
PRLogModuleInfo* gMediaCacheLog;
#define LOG(type, msg) PR_LOG(gMediaCacheLog, type, msg)
#else
#define LOG(type, msg)
#endif





static const double NONSEEKABLE_READAHEAD_MAX = 0.5;





static const PRUint32 REPLAY_DELAY = 30;






static const PRUint32 FREE_BLOCK_SCAN_LIMIT = 16;

using mozilla::TimeStamp;
using mozilla::TimeDuration;

#ifdef DEBUG


#endif

class nsMediaCache {
public:
  friend class nsMediaCacheStream::BlockList;
  typedef nsMediaCacheStream::BlockList BlockList;
  enum {
    BLOCK_SIZE = nsMediaCacheStream::BLOCK_SIZE
  };

  nsMediaCache() : mMonitor(nsAutoMonitor::NewMonitor("media.cache")),
    mFD(nsnull), mFDCurrentPos(0), mUpdateQueued(PR_FALSE)
#ifdef DEBUG
    , mInUpdate(PR_FALSE)
#endif
  {
    MOZ_COUNT_CTOR(nsMediaCache);
  }
  ~nsMediaCache() {
    NS_ASSERTION(mStreams.IsEmpty(), "Stream(s) still open!");
    Truncate();
    NS_ASSERTION(mIndex.Length() == 0, "Blocks leaked?");
    if (mFD) {
      PR_Close(mFD);
    }
    if (mMonitor) {
      nsAutoMonitor::DestroyMonitor(mMonitor);
    }
    MOZ_COUNT_DTOR(nsMediaCache);
  }

  
  nsresult Init();
  
  
  
  
  
  static void MaybeShutdown();

  
  
  
  nsresult ReadCacheFile(PRInt64 aOffset, void* aData, PRInt32 aLength,
                         PRInt32* aBytes);
  
  nsresult ReadCacheFileAllBytes(PRInt64 aOffset, void* aData, PRInt32 aLength);
  
  nsresult WriteCacheFile(PRInt64 aOffset, const void* aData, PRInt32 aLength);

  
  
  
  
  void OpenStream(nsMediaCacheStream* aStream);
  
  void ReleaseStream(nsMediaCacheStream* aStream);
  
  void ReleaseStreamBlocks(nsMediaCacheStream* aStream);
  
  void AllocateAndWriteBlock(nsMediaCacheStream* aStream, const void* aData,
                             nsMediaCacheStream::ReadMode aMode);

  
  
  
  
  
  void NoteSeek(nsMediaCacheStream* aStream, PRInt64 aOldOffset);
  
  
  
  
  
  
  void NoteBlockUsage(PRInt32 aBlockIndex, nsMediaCacheStream::ReadMode aMode,
                      TimeStamp aNow);

  
  void QueueUpdate();

  
  
  
  
  
  
  
  void Update();

#ifdef DEBUG_VERIFY_CACHE
  
  void Verify();
#else
  void Verify() {}
#endif

  PRMonitor* Monitor() { return mMonitor; }

protected:
  
  
  
  PRInt32 FindBlockForIncomingData(TimeStamp aNow, nsMediaCacheStream* aStream);
  
  
  
  
  
  
  
  PRInt32 FindReusableBlock(TimeStamp aNow,
                            nsMediaCacheStream* aForStream,
                            PRInt32 aForStreamBlock,
                            PRInt32 aMaxSearchBlockIndex);
  
  
  
  
  void AppendMostReusableBlock(BlockList* aBlockList,
                               nsTArray<PRUint32>* aResult,
                               PRInt32 aBlockIndexLimit);

  enum BlockClass {
    
    FREE_BLOCK,
    
    
    
    
    METADATA_BLOCK,
    
    
    PLAYED_BLOCK,
    
    
    
    READAHEAD_BLOCK
  };

  struct Block {
    Block() : mStream(nsnull), mClass(FREE_BLOCK) {}

    
    nsMediaCacheStream* mStream;
    
    PRUint32            mStreamBlock;
    
    
    TimeStamp           mLastUseTime;
    
    BlockClass          mClass;
  };

  
  
  BlockList* GetListForBlock(Block* aBlock);
  
  
  void FreeBlock(PRInt32 aBlock);
  
  
  void SwapBlocks(PRInt32 aBlockIndex1, PRInt32 aBlockIndex2);
  
  
  void InsertReadaheadBlock(PRInt32 aBlockIndex);

  
  TimeDuration PredictNextUse(TimeStamp aNow, PRInt32 aBlock);
  
  TimeDuration PredictNextUseForIncomingData(nsMediaCacheStream* aStream);

  
  
  void Truncate();

  
  nsTArray<nsMediaCacheStream*> mStreams;

  
  
  
  PRMonitor*      mMonitor;
  
  nsTArray<Block> mIndex;
  
  
  PRFileDesc*     mFD;
  
  PRInt64         mFDCurrentPos;
  
  BlockList       mFreeBlocks;
  
  BlockList       mMetadataBlocks;
  
  BlockList       mPlayedBlocks;
  
  PRPackedBool    mUpdateQueued;
#ifdef DEBUG
  PRPackedBool    mInUpdate;
#endif
};




static nsMediaCache* gMediaCache;

void nsMediaCacheStream::BlockList::AddFirstBlock(PRInt32 aBlock)
{
  NS_ASSERTION(!mEntries.GetEntry(aBlock), "Block already in list");
  Entry* entry = mEntries.PutEntry(aBlock);

  if (mFirstBlock < 0) {
    entry->mNextBlock = entry->mPrevBlock = aBlock;
  } else {
    entry->mNextBlock = mFirstBlock;
    entry->mPrevBlock = mEntries.GetEntry(mFirstBlock)->mPrevBlock;
    mEntries.GetEntry(entry->mNextBlock)->mPrevBlock = aBlock;
    mEntries.GetEntry(entry->mPrevBlock)->mNextBlock = aBlock;
  }
  mFirstBlock = aBlock;
  ++mCount;
}

void nsMediaCacheStream::BlockList::AddAfter(PRInt32 aBlock, PRInt32 aBefore)
{
  NS_ASSERTION(!mEntries.GetEntry(aBlock), "Block already in list");
  Entry* entry = mEntries.PutEntry(aBlock);

  Entry* addAfter = mEntries.GetEntry(aBefore);
  NS_ASSERTION(addAfter, "aBefore not in list");

  entry->mNextBlock = addAfter->mNextBlock;
  entry->mPrevBlock = aBefore;
  mEntries.GetEntry(entry->mNextBlock)->mPrevBlock = aBlock;
  mEntries.GetEntry(entry->mPrevBlock)->mNextBlock = aBlock;
  ++mCount;
}

void nsMediaCacheStream::BlockList::RemoveBlock(PRInt32 aBlock)
{
  Entry* entry = mEntries.GetEntry(aBlock);
  NS_ASSERTION(entry, "Block not in list");

  if (entry->mNextBlock == aBlock) {
    NS_ASSERTION(entry->mPrevBlock == aBlock, "Linked list inconsistency");
    NS_ASSERTION(mFirstBlock == aBlock, "Linked list inconsistency");
    mFirstBlock = -1;
  } else {
    if (mFirstBlock == aBlock) {
      mFirstBlock = entry->mNextBlock;
    }
    mEntries.GetEntry(entry->mNextBlock)->mPrevBlock = entry->mPrevBlock;
    mEntries.GetEntry(entry->mPrevBlock)->mNextBlock = entry->mNextBlock;
  }
  mEntries.RemoveEntry(aBlock);
  --mCount;
}

PRInt32 nsMediaCacheStream::BlockList::GetLastBlock() const
{
  if (mFirstBlock < 0)
    return -1;
  return mEntries.GetEntry(mFirstBlock)->mPrevBlock;
}

PRInt32 nsMediaCacheStream::BlockList::GetNextBlock(PRInt32 aBlock) const
{
  PRInt32 block = mEntries.GetEntry(aBlock)->mNextBlock;
  if (block == mFirstBlock)
    return -1;
  return block;
}

PRInt32 nsMediaCacheStream::BlockList::GetPrevBlock(PRInt32 aBlock) const
{
  if (aBlock == mFirstBlock)
    return -1;
  return mEntries.GetEntry(aBlock)->mPrevBlock;
}

#ifdef DEBUG
void nsMediaCacheStream::BlockList::Verify()
{
  PRInt32 count = 0;
  if (mFirstBlock >= 0) {
    PRInt32 block = mFirstBlock;
    do {
      Entry* entry = mEntries.GetEntry(block);
      NS_ASSERTION(mEntries.GetEntry(entry->mNextBlock)->mPrevBlock == block,
                   "Bad prev link");
      NS_ASSERTION(mEntries.GetEntry(entry->mPrevBlock)->mNextBlock == block,
                   "Bad next link");
      block = entry->mNextBlock;
      ++count;
    } while (block != mFirstBlock);
  }
  NS_ASSERTION(count == mCount, "Bad count");
}
#endif

static void UpdateSwappedBlockIndex(PRInt32* aBlockIndex,
    PRInt32 aBlock1Index, PRInt32 aBlock2Index)
{
  PRInt32 index = *aBlockIndex;
  if (index == aBlock1Index) {
    *aBlockIndex = aBlock2Index;
  } else if (index == aBlock2Index) {
    *aBlockIndex = aBlock1Index;
  }
}

void
nsMediaCacheStream::BlockList::NotifyBlockSwapped(PRInt32 aBlockIndex1,
                                                  PRInt32 aBlockIndex2)
{
  Entry* e1 = mEntries.GetEntry(aBlockIndex1);
  Entry* e2 = mEntries.GetEntry(aBlockIndex2);
  PRInt32 e1Prev = -1, e1Next = -1, e2Prev = -1, e2Next = -1;

  
  UpdateSwappedBlockIndex(&mFirstBlock, aBlockIndex1, aBlockIndex2);

  
  
  if (e1) {
    e1Prev = e1->mPrevBlock;
    e1Next = e1->mNextBlock;
  }
  if (e2) {
    e2Prev = e2->mPrevBlock;
    e2Next = e2->mNextBlock;
  }
  
  if (e1) {
    mEntries.GetEntry(e1Prev)->mNextBlock = aBlockIndex2;
    mEntries.GetEntry(e1Next)->mPrevBlock = aBlockIndex2;
  }
  if (e2) {
    mEntries.GetEntry(e2Prev)->mNextBlock = aBlockIndex1;
    mEntries.GetEntry(e2Next)->mPrevBlock = aBlockIndex1;
  }

  
  if (e1) {
    e1Prev = e1->mPrevBlock;
    e1Next = e1->mNextBlock;
    mEntries.RemoveEntry(aBlockIndex1);
  }
  if (e2) {
    e2Prev = e2->mPrevBlock;
    e2Next = e2->mNextBlock;
    mEntries.RemoveEntry(aBlockIndex2);
  }
  
  if (e1) {
    e1 = mEntries.PutEntry(aBlockIndex2);
    e1->mNextBlock = e1Next;
    e1->mPrevBlock = e1Prev;
  }
  if (e2) {
    e2 = mEntries.PutEntry(aBlockIndex1);
    e2->mNextBlock = e2Next;
    e2->mPrevBlock = e2Prev;
  }
}

nsresult
nsMediaCache::Init()
{
  NS_ASSERTION(NS_IsMainThread(), "Only call on main thread");

  if (!mMonitor) {
    
    return NS_ERROR_OUT_OF_MEMORY;
  }

  nsCOMPtr<nsIFile> tmp;
  nsresult rv = NS_GetSpecialDirectory(NS_OS_TEMP_DIR, getter_AddRefs(tmp));
  if (NS_FAILED(rv))
    return rv;
  nsCOMPtr<nsILocalFile> tmpFile = do_QueryInterface(tmp);
  if (!tmpFile)
    return NS_ERROR_FAILURE;
  rv = tmpFile->AppendNative(nsDependentCString("moz_media_cache"));
  if (NS_FAILED(rv))
    return rv;
  rv = tmpFile->CreateUnique(nsIFile::NORMAL_FILE_TYPE, 0600);
  if (NS_FAILED(rv))
    return rv;
  rv = tmpFile->OpenNSPRFileDesc(PR_RDWR | nsILocalFile::DELETE_ON_CLOSE,
                                 PR_IRWXU, &mFD);
  if (NS_FAILED(rv))
    return rv;

#ifdef PR_LOGGING
  if (!gMediaCacheLog) {
    gMediaCacheLog = PR_NewLogModule("nsMediaCache");
  }
#endif

  return NS_OK;
}

void
nsMediaCache::MaybeShutdown()
{
  NS_ASSERTION(NS_IsMainThread(), 
               "nsMediaCache::MaybeShutdown called on non-main thread");
  if (!gMediaCache->mStreams.IsEmpty()) {
    
    return;
  }

  
  
  
  delete gMediaCache;
  gMediaCache = nsnull;
}

static void
InitMediaCache()
{
  if (gMediaCache)
    return;

  gMediaCache = new nsMediaCache();
  if (!gMediaCache)
    return;

  nsresult rv = gMediaCache->Init();
  if (NS_FAILED(rv)) {
    delete gMediaCache;
    gMediaCache = nsnull;
  }
}

nsresult
nsMediaCache::ReadCacheFile(PRInt64 aOffset, void* aData, PRInt32 aLength,
                            PRInt32* aBytes)
{
  PR_ASSERT_CURRENT_THREAD_IN_MONITOR(mMonitor);

  if (!mFD)
    return NS_ERROR_FAILURE;

  if (mFDCurrentPos != aOffset) {
    PROffset64 offset = PR_Seek64(mFD, aOffset, PR_SEEK_SET);
    if (offset != aOffset)
      return NS_ERROR_FAILURE;
    mFDCurrentPos = aOffset;
  }
  PRInt32 amount = PR_Read(mFD, aData, aLength);
  if (amount <= 0)
    return NS_ERROR_FAILURE;
  mFDCurrentPos += amount;
  *aBytes = amount;
  return NS_OK;
}

nsresult
nsMediaCache::ReadCacheFileAllBytes(PRInt64 aOffset, void* aData, PRInt32 aLength)
{
  PR_ASSERT_CURRENT_THREAD_IN_MONITOR(mMonitor);

  PRInt64 offset = aOffset;
  PRInt32 count = aLength;
  
  char* data = static_cast<char*>(aData);
  while (count > 0) {
    PRInt32 bytes;
    nsresult rv = ReadCacheFile(offset, data, count, &bytes);
    if (NS_FAILED(rv))
      return rv;
    if (bytes == 0)
      return NS_ERROR_FAILURE;
    count -= bytes;
    data += bytes;
    offset += bytes;
  }
  return NS_OK;
}

nsresult
nsMediaCache::WriteCacheFile(PRInt64 aOffset, const void* aData, PRInt32 aLength)
{
  PR_ASSERT_CURRENT_THREAD_IN_MONITOR(mMonitor);

  if (!mFD)
    return NS_ERROR_FAILURE;

  if (mFDCurrentPos != aOffset) {
    PROffset64 offset = PR_Seek64(mFD, aOffset, PR_SEEK_SET);
    if (offset != aOffset)
      return NS_ERROR_FAILURE;
    mFDCurrentPos = aOffset;
  }

  const char* data = static_cast<const char*>(aData);
  PRInt32 length = aLength;
  while (length > 0) {
    PRInt32 amount = PR_Write(mFD, data, length);
    if (amount <= 0)
      return NS_ERROR_FAILURE;
    mFDCurrentPos += amount;
    length -= amount;
    data += amount;
  }

  return NS_OK;
}

static PRInt32 GetMaxBlocks()
{
  
  
  
  PRInt32 cacheSize = nsContentUtils::GetIntPref("media.cache_size", 50*1024);
  PRInt64 maxBlocks = PRInt64(cacheSize)*1024/nsMediaCache::BLOCK_SIZE;
  maxBlocks = PR_MAX(maxBlocks, 1);
  return PRInt32(PR_MIN(maxBlocks, PR_INT32_MAX));
}

PRInt32
nsMediaCache::FindBlockForIncomingData(TimeStamp aNow,
                                       nsMediaCacheStream* aStream)
{
  PR_ASSERT_CURRENT_THREAD_IN_MONITOR(mMonitor);

  PRInt32 blockIndex = FindReusableBlock(aNow, aStream,
      aStream->mChannelOffset/BLOCK_SIZE, PR_INT32_MAX);

  if (blockIndex < 0 || mIndex[blockIndex].mStream) {
    
    
    
    
    
    if ((mIndex.Length() < PRUint32(GetMaxBlocks()) || blockIndex < 0 ||
         PredictNextUseForIncomingData(aStream) >= PredictNextUse(aNow, blockIndex))) {
      blockIndex = mIndex.Length();
      if (!mIndex.AppendElement())
        return -1;
      mFreeBlocks.AddFirstBlock(blockIndex);
      return blockIndex;
    }
  }

  return blockIndex;
}

void
nsMediaCache::AppendMostReusableBlock(BlockList* aBlockList,
                                      nsTArray<PRUint32>* aResult,
                                      PRInt32 aBlockIndexLimit)
{
  PR_ASSERT_CURRENT_THREAD_IN_MONITOR(mMonitor);

  PRInt32 blockIndex = aBlockList->GetLastBlock();
  if (blockIndex < 0)
    return;
  do {
    
    
    
    
    nsMediaCacheStream* stream = mIndex[blockIndex].mStream;
    if (stream->mPinCount == 0 && blockIndex < aBlockIndexLimit &&
        stream->mStreamOffset/BLOCK_SIZE != mIndex[blockIndex].mStreamBlock) {
      aResult->AppendElement(blockIndex);
      return;
    }
    blockIndex = aBlockList->GetPrevBlock(blockIndex);
  } while (blockIndex >= 0);
}

PRInt32
nsMediaCache::FindReusableBlock(TimeStamp aNow,
                                nsMediaCacheStream* aForStream,
                                PRInt32 aForStreamBlock,
                                PRInt32 aMaxSearchBlockIndex)
{
  PR_ASSERT_CURRENT_THREAD_IN_MONITOR(mMonitor);

  PRUint32 length = PR_MIN(PRUint32(aMaxSearchBlockIndex), mIndex.Length());

  if (aForStream && aForStreamBlock > 0 &&
      PRUint32(aForStreamBlock) <= aForStream->mBlocks.Length()) {
    PRInt32 prevCacheBlock = aForStream->mBlocks[aForStreamBlock - 1];
    if (prevCacheBlock >= 0) {
      PRUint32 freeBlockScanEnd =
        PR_MIN(length, prevCacheBlock + FREE_BLOCK_SCAN_LIMIT);
      for (PRUint32 i = prevCacheBlock; i < freeBlockScanEnd; ++i) {
        if (mIndex[i].mClass == FREE_BLOCK)
          return i;
      }
    }
  }

  if (!mFreeBlocks.IsEmpty()) {
    PRInt32 blockIndex = mFreeBlocks.GetFirstBlock();
    do {
      if (blockIndex < aMaxSearchBlockIndex)
        return blockIndex;
      blockIndex = mFreeBlocks.GetNextBlock(blockIndex);
    } while (blockIndex >= 0);
  }

  
  
  
  
  nsAutoTArray<PRUint32,8> candidates;
  AppendMostReusableBlock(&mMetadataBlocks, &candidates, length);
  AppendMostReusableBlock(&mPlayedBlocks, &candidates, length);
  for (PRUint32 i = 0; i < mStreams.Length(); ++i) {
    nsMediaCacheStream* stream = mStreams[i];
    
    
    
    
    if (!stream->mReadaheadBlocks.IsEmpty() && stream->mIsSeekable &&
        stream->mPinCount == 0) {
      
      PRInt32 blockIndex = stream->mReadaheadBlocks.GetLastBlock();
      do {
        if (PRUint32(blockIndex) < length) {
          candidates.AppendElement(blockIndex);
          break;
        }
        blockIndex = stream->mReadaheadBlocks.GetPrevBlock(blockIndex);
      } while (blockIndex >= 0);
    }
  }

  TimeDuration latestUse;
  PRInt32 latestUseBlock = -1;
  for (PRUint32 i = 0; i < candidates.Length(); ++i) {
    TimeDuration nextUse = PredictNextUse(aNow, candidates[i]);
    if (nextUse > latestUse) {
      latestUse = nextUse;
      latestUseBlock = candidates[i];
    }
  }

#ifdef DEBUG
  for (PRUint32 blockIndex = 0; blockIndex < length; ++blockIndex) {
    Block* block = &mIndex[blockIndex];
    nsMediaCacheStream* stream = block->mStream;
    NS_ASSERTION(!stream || stream->mPinCount > 0 ||
                 (!stream->mIsSeekable && block->mClass == READAHEAD_BLOCK) ||
                 stream->mStreamOffset/BLOCK_SIZE == block->mStreamBlock ||
                 PredictNextUse(aNow, blockIndex) <= latestUse,
                 "We missed a block that should be replaced");
  }
#endif

  return latestUseBlock;
}

nsMediaCache::BlockList*
nsMediaCache::GetListForBlock(Block* aBlock)
{
  switch (aBlock->mClass) {
  case FREE_BLOCK:
    NS_ASSERTION(!aBlock->mStream, "Free block has a stream?");
    return &mFreeBlocks;
  case METADATA_BLOCK:
    NS_ASSERTION(aBlock->mStream, "Metadata block has no stream?");
    return &mMetadataBlocks;
  case PLAYED_BLOCK:
    NS_ASSERTION(aBlock->mStream, "Metadata block has no stream?");
    return &mPlayedBlocks;
  case READAHEAD_BLOCK:
    NS_ASSERTION(aBlock->mStream, "Readahead block has no stream?");
    return &aBlock->mStream->mReadaheadBlocks;
  default:
    NS_ERROR("Invalid block class");
    return nsnull;
  }
}

void
nsMediaCache::SwapBlocks(PRInt32 aBlockIndex1, PRInt32 aBlockIndex2)
{
  PR_ASSERT_CURRENT_THREAD_IN_MONITOR(mMonitor);

  Block* block1 = &mIndex[aBlockIndex1];
  Block* block2 = &mIndex[aBlockIndex2];

  Block tmp = *block1;
  *block1 = *block2;
  *block2 = tmp;

  
  

  if (block1->mStream) {
    block1->mStream->mBlocks[block1->mStreamBlock] = aBlockIndex1;
  }
  if (block2->mStream) {
    block2->mStream->mBlocks[block2->mStreamBlock] = aBlockIndex2;
  }

  BlockList* list1 = GetListForBlock(block1);
  list1->NotifyBlockSwapped(aBlockIndex1, aBlockIndex2);
  BlockList* list2 = GetListForBlock(block2);
  
  if (list1 != list2) {
    list2->NotifyBlockSwapped(aBlockIndex1, aBlockIndex2);
  }

  Verify();
}

void
nsMediaCache::FreeBlock(PRInt32 aBlock)
{
  PR_ASSERT_CURRENT_THREAD_IN_MONITOR(mMonitor);

  Block* block = &mIndex[aBlock];
  GetListForBlock(block)->RemoveBlock(aBlock);
  if (block->mStream) {
    block->mStream->mBlocks[block->mStreamBlock] = -1;
  }
  block->mStream = nsnull;
  block->mClass = FREE_BLOCK;
  mFreeBlocks.AddFirstBlock(aBlock);
  Verify();
}

TimeDuration
nsMediaCache::PredictNextUse(TimeStamp aNow, PRInt32 aBlock)
{
  PR_ASSERT_CURRENT_THREAD_IN_MONITOR(mMonitor);

  Block* block = &mIndex[aBlock];

  switch (block->mClass) {
  case METADATA_BLOCK:
    
    
    return aNow - block->mLastUseTime;
  case PLAYED_BLOCK:
    
    
    NS_ASSERTION(PRInt64(block->mStreamBlock)*BLOCK_SIZE <
                 block->mStream->mStreamOffset,
                 "Played block after the current stream position?");
    return aNow - block->mLastUseTime +
        TimeDuration::FromSeconds(REPLAY_DELAY);
  case READAHEAD_BLOCK: {
    PRInt64 bytesAhead =
      PRInt64(block->mStreamBlock)*BLOCK_SIZE - block->mStream->mStreamOffset;
    NS_ASSERTION(bytesAhead >= 0,
                 "Readahead block before the current stream position?");
    PRInt64 millisecondsAhead =
      bytesAhead*1000/block->mStream->mPlaybackBytesPerSecond;
    return TimeDuration::FromMilliseconds(
        PR_MIN(millisecondsAhead, PR_INT32_MAX));
  }
  default:
    NS_ERROR("Invalid class for predicting next use");
    return TimeDuration(0);
  }
}

TimeDuration
nsMediaCache::PredictNextUseForIncomingData(nsMediaCacheStream* aStream)
{
  PR_ASSERT_CURRENT_THREAD_IN_MONITOR(mMonitor);

  PRInt64 bytesAhead = aStream->mChannelOffset - aStream->mStreamOffset;
  if (bytesAhead <= -BLOCK_SIZE) {
    
    return TimeDuration::FromSeconds(24*60*60);
  }
  if (bytesAhead <= 0)
    return TimeDuration(0);
  PRInt64 millisecondsAhead = bytesAhead*1000/aStream->mPlaybackBytesPerSecond;
  return TimeDuration::FromMilliseconds(
      PR_MIN(millisecondsAhead, PR_INT32_MAX));
}

void
nsMediaCache::Update()
{
  NS_ASSERTION(NS_IsMainThread(), "Only call on main thread");

  nsAutoMonitor mon(mMonitor);
  mUpdateQueued = PR_FALSE;
#ifdef DEBUG
  mInUpdate = PR_TRUE;
#endif

  PRInt32 maxBlocks = GetMaxBlocks();
  TimeStamp now = TimeStamp::Now();

  PRInt32 freeBlockCount = mFreeBlocks.GetCount();
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  TimeDuration latestPredictedUseForOverflow = 0;
  for (PRInt32 blockIndex = mIndex.Length() - 1; blockIndex >= maxBlocks;
       --blockIndex) {
    nsMediaCacheStream* stream = mIndex[blockIndex].mStream;
    if (!stream) {
      
      --freeBlockCount;
      continue;
    }
    TimeDuration predictedUse = PredictNextUse(now, blockIndex);
    latestPredictedUseForOverflow = PR_MAX(latestPredictedUseForOverflow, predictedUse);
  }

  
  for (PRInt32 blockIndex = mIndex.Length() - 1; blockIndex >= maxBlocks;
       --blockIndex) {
    Block* block = &mIndex[blockIndex];
    nsMediaCacheStream* stream = block->mStream;
    if (!stream)
      continue;

    PRInt32 destinationBlockIndex =
      FindReusableBlock(now, stream, block->mStreamBlock, maxBlocks);
    if (destinationBlockIndex < 0) {
      
      
      break;
    }

    Block* destinationBlock = &mIndex[destinationBlockIndex];
    if (destinationBlock->mClass == FREE_BLOCK ||
        PredictNextUse(now, destinationBlockIndex) > latestPredictedUseForOverflow) {
      
      
      char buf[BLOCK_SIZE];
      nsresult rv = ReadCacheFileAllBytes(blockIndex*BLOCK_SIZE, buf, sizeof(buf));
      if (NS_SUCCEEDED(rv)) {
        rv = WriteCacheFile(destinationBlockIndex*BLOCK_SIZE, buf, BLOCK_SIZE);
        if (NS_SUCCEEDED(rv)) {
          
          LOG(PR_LOG_DEBUG, ("Swapping blocks %d and %d (trimming cache)",
              blockIndex, destinationBlockIndex));
          
          
          SwapBlocks(blockIndex, destinationBlockIndex);
        } else {
          
          
          LOG(PR_LOG_DEBUG, ("Released block %d from stream %p block %d(%lld) (trimming cache)",
              destinationBlockIndex, destinationBlock->mStream,
              destinationBlock->mStreamBlock,
              (long long)destinationBlock->mStreamBlock*BLOCK_SIZE));
          FreeBlock(destinationBlockIndex);
        }
        
        if (block->mClass != FREE_BLOCK) {
          LOG(PR_LOG_DEBUG, ("Released block %d from stream %p block %d(%lld) (trimming cache)",
              blockIndex, block->mStream, block->mStreamBlock,
              (long long)block->mStreamBlock*BLOCK_SIZE));
          FreeBlock(blockIndex);
        }
      }
    }
  }
  
  Truncate();

  
  
  
  PRInt32 nonSeekableReadaheadBlockCount = 0;
  for (PRUint32 i = 0; i < mStreams.Length(); ++i) {
    nsMediaCacheStream* stream = mStreams[i];
    if (!stream->mIsSeekable) {
      nonSeekableReadaheadBlockCount += stream->mReadaheadBlocks.GetCount();
    }
  }

  
  
  TimeDuration latestNextUse;
  if (freeBlockCount == 0) {
    PRInt32 reusableBlock = FindReusableBlock(now, nsnull, 0, maxBlocks);
    if (reusableBlock >= 0) {
      latestNextUse = PredictNextUse(now, reusableBlock);
    }
  }

  
  
  
  nsTArray<nsMediaCacheStream*> streamsToClose;
  for (PRUint32 i = 0; i < mStreams.Length(); ++i) {
    nsMediaCacheStream* stream = mStreams[i];
    if (stream->mClosed)
      continue;

    
    
    PRInt64 desiredOffset = stream->GetCachedDataEndInternal(stream->mStreamOffset);
    if (stream->mIsSeekable) {
      if (desiredOffset > stream->mChannelOffset &&
          desiredOffset <= stream->mChannelOffset + SEEK_VS_READ_THRESHOLD) {
        
        
        desiredOffset = stream->mChannelOffset;
      }
    } else {
      
      if (stream->mChannelOffset > desiredOffset) {
        
        
        
        NS_WARNING("Can't seek backwards, so seeking to 0");
        desiredOffset = 0;
        
        
        
        
        ReleaseStreamBlocks(stream);
      } else {
        
        
        desiredOffset = stream->mChannelOffset;
      }
    }

    
    
    PRBool enableReading;
    if (stream->mStreamLength >= 0 &&
        desiredOffset >= stream->mStreamLength) {
      
      
      
      LOG(PR_LOG_DEBUG, ("Stream %p at end of stream", stream));
      enableReading = PR_TRUE;
    } else if (desiredOffset < stream->mStreamOffset) {
      
      
      LOG(PR_LOG_DEBUG, ("Stream %p catching up", stream));
      enableReading = PR_TRUE;
    } else if (desiredOffset < stream->mStreamOffset + BLOCK_SIZE) {
      
      LOG(PR_LOG_DEBUG, ("Stream %p feeding reader", stream));
      enableReading = PR_TRUE;
    } else if (!stream->mIsSeekable &&
               nonSeekableReadaheadBlockCount >= maxBlocks*NONSEEKABLE_READAHEAD_MAX) {
      
      
      
      LOG(PR_LOG_DEBUG, ("Stream %p throttling non-seekable readahead", stream));
      enableReading = PR_FALSE;
    } else if (mIndex.Length() > PRUint32(maxBlocks)) {
      
      
      LOG(PR_LOG_DEBUG, ("Stream %p throttling to reduce cache size", stream));
      enableReading = PR_FALSE;
    } else if (freeBlockCount > 0 || mIndex.Length() < PRUint32(maxBlocks)) {
      
      LOG(PR_LOG_DEBUG, ("Stream %p reading since there are free blocks", stream));
      enableReading = PR_TRUE;
    } else if (latestNextUse <= TimeDuration(0)) {
      
      LOG(PR_LOG_DEBUG, ("Stream %p throttling due to no reusable blocks", stream));
      enableReading = PR_FALSE;
    } else {
      
      
      TimeDuration predictedNewDataUse = PredictNextUseForIncomingData(stream);
      LOG(PR_LOG_DEBUG, ("Stream %p predict next data in %f, current worst block is %f",
          stream, predictedNewDataUse.ToSeconds(), latestNextUse.ToSeconds()));
      enableReading = predictedNewDataUse < latestNextUse;
    }

    nsresult rv = NS_OK;
    if (stream->mChannelOffset != desiredOffset && enableReading) {
      
      NS_ASSERTION(stream->mIsSeekable || desiredOffset == 0,
                   "Trying to seek in a non-seekable stream!");
      
      stream->mChannelOffset = (desiredOffset/BLOCK_SIZE)*BLOCK_SIZE;
      LOG(PR_LOG_DEBUG, ("Stream %p CacheSeek to %lld (resume=%d)", stream,
          (long long)stream->mChannelOffset, stream->mCacheSuspended));
      rv = stream->mClient->CacheClientSeek(stream->mChannelOffset,
                                            stream->mCacheSuspended);
      stream->mCacheSuspended = PR_FALSE;
    } else if (enableReading && stream->mCacheSuspended) {
      LOG(PR_LOG_DEBUG, ("Stream %p Resumed", stream));
      rv = stream->mClient->CacheClientResume();
      stream->mCacheSuspended = PR_FALSE;
    } else if (!enableReading && !stream->mCacheSuspended) {
      LOG(PR_LOG_DEBUG, ("Stream %p Suspended", stream));
      rv = stream->mClient->CacheClientSuspend();
      stream->mCacheSuspended = PR_TRUE;
    }

    if (NS_FAILED(rv)) {
      streamsToClose.AppendElement(stream);
    }
  }

  
  
  
  for (PRUint32 i = 0; i < streamsToClose.Length(); ++i) {
    streamsToClose[i]->CloseInternal(&mon);
  }
#ifdef DEBUG
  mInUpdate = PR_FALSE;
#endif
}

class UpdateEvent : public nsRunnable
{
public:
  NS_IMETHOD Run()
  {
    if (gMediaCache) {
      gMediaCache->Update();
    }
    return NS_OK;
  }
};

void
nsMediaCache::QueueUpdate()
{
  PR_ASSERT_CURRENT_THREAD_IN_MONITOR(mMonitor);

  
  
  NS_ASSERTION(!mInUpdate,
               "Queuing an update while we're in an update");
  if (mUpdateQueued)
    return;
  mUpdateQueued = PR_TRUE;
  nsCOMPtr<nsIRunnable> event = new UpdateEvent();
  NS_DispatchToMainThread(event);
}

#ifdef DEBUG_VERIFY_CACHE
void
nsMediaCache::Verify()
{
  PR_ASSERT_CURRENT_THREAD_IN_MONITOR(mMonitor);

  mFreeBlocks.Verify();
  mPlayedBlocks.Verify();
  mMetadataBlocks.Verify();
  for (PRUint32 i = 0; i < mStreams.Length(); ++i) {
    nsMediaCacheStream* stream = mStreams[i];
    stream->mReadaheadBlocks.Verify();
    if (!stream->mReadaheadBlocks.IsEmpty()) {
      
      PRInt32 block = stream->mReadaheadBlocks.GetFirstBlock();
      PRInt32 lastStreamBlock = -1;
      do {
        NS_ASSERTION(mIndex[block].mStream == stream, "Bad stream");
        NS_ASSERTION(lastStreamBlock < PRInt32(mIndex[block].mStreamBlock),
                     "Blocks not increasing in readahead stream");
        lastStreamBlock = PRInt32(mIndex[block].mStreamBlock);
        block = stream->mReadaheadBlocks.GetNextBlock(block);
      } while (block >= 0);
    }
  }
}
#endif

void
nsMediaCache::InsertReadaheadBlock(PRInt32 aBlockIndex)
{
  PR_ASSERT_CURRENT_THREAD_IN_MONITOR(mMonitor);

  Block* block = &mIndex[aBlockIndex];
  nsMediaCacheStream* stream = block->mStream;
  if (stream->mReadaheadBlocks.IsEmpty()) {
    stream->mReadaheadBlocks.AddFirstBlock(aBlockIndex);
    return;
  }

  
  
  PRInt32 readaheadIndex = stream->mReadaheadBlocks.GetLastBlock();
  do {
    if (mIndex[readaheadIndex].mStreamBlock < block->mStreamBlock) {
      stream->mReadaheadBlocks.AddAfter(aBlockIndex, readaheadIndex);
      return;
    }
    NS_ASSERTION(mIndex[readaheadIndex].mStreamBlock > block->mStreamBlock,
                 "Duplicated blocks??");
    readaheadIndex = stream->mReadaheadBlocks.GetPrevBlock(readaheadIndex);
  } while (readaheadIndex >= 0);
  stream->mReadaheadBlocks.AddFirstBlock(aBlockIndex);
  Verify();
}

void
nsMediaCache::AllocateAndWriteBlock(nsMediaCacheStream* aStream, const void* aData,
                                    nsMediaCacheStream::ReadMode aMode)
{
  PR_ASSERT_CURRENT_THREAD_IN_MONITOR(mMonitor);

  PRInt32 streamBlockIndex = aStream->mChannelOffset/BLOCK_SIZE;
  
  while (streamBlockIndex >= PRInt32(aStream->mBlocks.Length())) {
    aStream->mBlocks.AppendElement(-1);
  }
  if (aStream->mBlocks[streamBlockIndex] >= 0) {
    
    PRInt32 globalBlockIndex = aStream->mBlocks[streamBlockIndex];
    LOG(PR_LOG_DEBUG, ("Released block %d from stream %p block %d(%lld)",
        globalBlockIndex, aStream, streamBlockIndex, (long long)streamBlockIndex*BLOCK_SIZE));
    FreeBlock(globalBlockIndex);
  }

  TimeStamp now = TimeStamp::Now();
  PRInt32 blockIndex = FindBlockForIncomingData(now, aStream);
  if (blockIndex >= 0) {
    Block* block = &mIndex[blockIndex];
    if (block->mClass != FREE_BLOCK) {
      LOG(PR_LOG_DEBUG, ("Released block %d from stream %p block %d(%lld)",
          blockIndex, block->mStream, block->mStreamBlock, (long long)block->mStreamBlock*BLOCK_SIZE));
      FreeBlock(blockIndex);
    }
    NS_ASSERTION(block->mClass == FREE_BLOCK, "Block should be free now!");

    LOG(PR_LOG_DEBUG, ("Allocated block %d to stream %p block %d(%lld)",
        blockIndex, aStream, streamBlockIndex, (long long)streamBlockIndex*BLOCK_SIZE));
    block->mStream = aStream;
    block->mStreamBlock = streamBlockIndex;
    block->mLastUseTime = now;
    aStream->mBlocks[streamBlockIndex] = blockIndex;
    mFreeBlocks.RemoveBlock(blockIndex);
    if (streamBlockIndex*BLOCK_SIZE < aStream->mStreamOffset) {
      block->mClass = aMode == nsMediaCacheStream::MODE_PLAYBACK
        ? PLAYED_BLOCK : METADATA_BLOCK;
      
      
      
      GetListForBlock(block)->AddFirstBlock(blockIndex);
      Verify();
    } else {
      
      
      
      block->mClass = READAHEAD_BLOCK;
      InsertReadaheadBlock(blockIndex);
    }

    nsresult rv = WriteCacheFile(blockIndex*BLOCK_SIZE, aData, BLOCK_SIZE);
    if (NS_FAILED(rv)) {
      LOG(PR_LOG_DEBUG, ("Released block %d from stream %p block %d(%lld)",
          blockIndex, aStream, streamBlockIndex, (long long)streamBlockIndex*BLOCK_SIZE));
      FreeBlock(blockIndex);
    }
  }

  
  
  QueueUpdate();
}

void
nsMediaCache::OpenStream(nsMediaCacheStream* aStream)
{
  NS_ASSERTION(NS_IsMainThread(), "Only call on main thread");

  nsAutoMonitor mon(mMonitor);
  mStreams.AppendElement(aStream);
}

void
nsMediaCache::ReleaseStream(nsMediaCacheStream* aStream)
{
  NS_ASSERTION(NS_IsMainThread(), "Only call on main thread");

  nsAutoMonitor mon(mMonitor);
  mStreams.RemoveElement(aStream);
}

void
nsMediaCache::ReleaseStreamBlocks(nsMediaCacheStream* aStream)
{
  PR_ASSERT_CURRENT_THREAD_IN_MONITOR(mMonitor);

  
  
  
  PRUint32 length = aStream->mBlocks.Length();
  for (PRUint32 i = 0; i < length; ++i) {
    PRInt32 blockIndex = aStream->mBlocks[i];
    if (blockIndex >= 0) {
      LOG(PR_LOG_DEBUG, ("Released block %d from stream %p block %d(%lld)",
          blockIndex, aStream, i, (long long)i*BLOCK_SIZE));
      FreeBlock(blockIndex);
    }
  }
}

void
nsMediaCache::Truncate()
{
  PRUint32 end;
  for (end = mIndex.Length(); end > 0; --end) {
    if (mIndex[end - 1].mStream)
      break;
    mFreeBlocks.RemoveBlock(end - 1);
  }

  if (end < mIndex.Length()) {
    mIndex.TruncateLength(end);
    
    
    
    
  }
}

void
nsMediaCache::NoteBlockUsage(PRInt32 aBlockIndex,
                             nsMediaCacheStream::ReadMode aMode,
                             TimeStamp aNow)
{
  PR_ASSERT_CURRENT_THREAD_IN_MONITOR(mMonitor);

  if (aBlockIndex < 0) {
    
    return;
  }

  Block* block = &mIndex[aBlockIndex];
  if (block->mClass == FREE_BLOCK) {
    
    return;
  }

  
  
  NS_ASSERTION(block->mStreamBlock*BLOCK_SIZE <= block->mStream->mStreamOffset,
               "Using a block that's behind the read position?");

  GetListForBlock(block)->RemoveBlock(aBlockIndex);
  block->mClass =
    (aMode == nsMediaCacheStream::MODE_METADATA || block->mClass == METADATA_BLOCK)
    ? METADATA_BLOCK : PLAYED_BLOCK;
  
  
  GetListForBlock(block)->AddFirstBlock(aBlockIndex);
  block->mLastUseTime = aNow;
  Verify();
}

void
nsMediaCache::NoteSeek(nsMediaCacheStream* aStream, PRInt64 aOldOffset)
{
  PR_ASSERT_CURRENT_THREAD_IN_MONITOR(mMonitor);

  if (aOldOffset < aStream->mStreamOffset) {
    
    
    
    PRInt32 blockIndex = aOldOffset/BLOCK_SIZE;
    PRInt32 endIndex =
      PR_MIN((aStream->mStreamOffset + BLOCK_SIZE - 1)/BLOCK_SIZE,
             aStream->mBlocks.Length());
    TimeStamp now = TimeStamp::Now();
    while (blockIndex < endIndex) {
      PRInt32 cacheBlockIndex = aStream->mBlocks[blockIndex];
      if (cacheBlockIndex >= 0) {
        
        
        NoteBlockUsage(cacheBlockIndex, nsMediaCacheStream::MODE_PLAYBACK,
                       now);
      }
      ++blockIndex;
    }
  } else {
    
    
    
    PRInt32 blockIndex =
      (aStream->mStreamOffset + BLOCK_SIZE - 1)/BLOCK_SIZE;
    PRInt32 endIndex =
      PR_MIN((aOldOffset + BLOCK_SIZE - 1)/BLOCK_SIZE,
             aStream->mBlocks.Length());
    while (blockIndex < endIndex) {
      PRInt32 cacheBlockIndex = aStream->mBlocks[endIndex - 1];
      if (cacheBlockIndex >= 0) {
        Block* block = &mIndex[cacheBlockIndex];
        if (block->mClass != METADATA_BLOCK) {
          mPlayedBlocks.RemoveBlock(cacheBlockIndex);
          block->mClass = READAHEAD_BLOCK;
          
          
          
          
          GetListForBlock(block)->AddFirstBlock(cacheBlockIndex);
          Verify();
        }
      }
      --endIndex;
    }
  }
}

void
nsMediaCacheStream::NotifyDataLength(PRInt64 aLength)
{
  NS_ASSERTION(NS_IsMainThread(), "Only call on main thread");

  nsAutoMonitor mon(gMediaCache->Monitor());
  mStreamLength = aLength;
}

void
nsMediaCacheStream::NotifyDataStarted(PRInt64 aOffset) 
{
  NS_ASSERTION(NS_IsMainThread(), "Only call on main thread");

  nsAutoMonitor mon(gMediaCache->Monitor());
  NS_WARN_IF_FALSE(aOffset == mChannelOffset,
                   "Server is giving us unexpected offset");
  mChannelOffset = aOffset;
  if (mStreamLength >= 0) {
    
    
    mStreamLength = PR_MAX(mStreamLength, mChannelOffset);
  }
}

void
nsMediaCacheStream::UpdatePrincipal(nsIPrincipal* aPrincipal)
{
  if (!mPrincipal) {
    NS_ASSERTION(!mUsingNullPrincipal, "Are we using a null principal or not?");
    if (mUsingNullPrincipal) {
      
      return;
    }
    mPrincipal = aPrincipal;
    return;
  }

  if (mPrincipal == aPrincipal) {
    
    NS_ASSERTION(!mUsingNullPrincipal, "We can't receive data from a null principal");
    return;
  }
  if (mUsingNullPrincipal) {
    
    
    return;
  }

  PRBool equal;
  nsresult rv = mPrincipal->Equals(aPrincipal, &equal);
  if (NS_SUCCEEDED(rv) && equal)
    return;

  
  mPrincipal = do_CreateInstance("@mozilla.org/nullprincipal;1");
  mUsingNullPrincipal = PR_TRUE;
}

void
nsMediaCacheStream::NotifyDataReceived(PRInt64 aSize, const char* aData,
    nsIPrincipal* aPrincipal)
{
  NS_ASSERTION(NS_IsMainThread(), "Only call on main thread");

  UpdatePrincipal(aPrincipal);

  nsAutoMonitor mon(gMediaCache->Monitor());
  PRInt64 size = aSize;
  const char* data = aData;

  LOG(PR_LOG_DEBUG, ("Stream %p DataReceived at %lld count=%lld",
      this, (long long)mChannelOffset, (long long)aSize));

  
  while (size > 0) {
    PRUint32 blockIndex = mChannelOffset/BLOCK_SIZE;
    PRInt32 blockOffset = PRInt32(mChannelOffset - blockIndex*BLOCK_SIZE);
    PRInt32 chunkSize = PRInt32(PR_MIN(BLOCK_SIZE - blockOffset, size));

    
    
    const char* blockDataToStore = nsnull;
    ReadMode mode = MODE_PLAYBACK;
    if (blockOffset == 0 && chunkSize == BLOCK_SIZE) {
      
      
      blockDataToStore = data;
    } else {
      if (blockOffset == 0) {
        
        
        mMetadataInPartialBlockBuffer = PR_FALSE;
      }
      memcpy(reinterpret_cast<char*>(mPartialBlockBuffer) + blockOffset,
             data, chunkSize);

      if (blockOffset + chunkSize == BLOCK_SIZE) {
        
        blockDataToStore = reinterpret_cast<char*>(mPartialBlockBuffer);
        if (mMetadataInPartialBlockBuffer) {
          mode = MODE_METADATA;
        }
      }
    }

    if (blockDataToStore) {
      gMediaCache->AllocateAndWriteBlock(this, blockDataToStore, mode);
    }

    mChannelOffset += chunkSize;
    if (mStreamLength >= 0) {
      
      mStreamLength = PR_MAX(mStreamLength, mChannelOffset);
    }
    size -= chunkSize;
    data += chunkSize;
  }

  
  
  
  mon.NotifyAll();
}

void
nsMediaCacheStream::NotifyDataEnded(nsresult aStatus) 
{
  NS_ASSERTION(NS_IsMainThread(), "Only call on main thread");

  nsAutoMonitor mon(gMediaCache->Monitor());
  if (NS_SUCCEEDED(aStatus)) {
    
    mStreamLength = mChannelOffset;
  }

  PRInt32 blockOffset = PRInt32(mChannelOffset%BLOCK_SIZE);
  if (blockOffset > 0) {
    
    memset(reinterpret_cast<char*>(mPartialBlockBuffer) + blockOffset, 0,
           BLOCK_SIZE - blockOffset);
    gMediaCache->AllocateAndWriteBlock(this, mPartialBlockBuffer,
        mMetadataInPartialBlockBuffer ? MODE_METADATA : MODE_PLAYBACK);
    
    mon.NotifyAll();
  }
}

nsMediaCacheStream::~nsMediaCacheStream()
{
  NS_ASSERTION(NS_IsMainThread(), "Only call on main thread");
  NS_ASSERTION(mClosed, "Stream was not closed");
  NS_ASSERTION(!mPinCount, "Unbalanced Pin");

  gMediaCache->ReleaseStream(this);
  nsMediaCache::MaybeShutdown();
}

void
nsMediaCacheStream::SetSeekable(PRBool aIsSeekable)
{
  nsAutoMonitor mon(gMediaCache->Monitor());
  NS_ASSERTION(mIsSeekable || aIsSeekable ||
               mChannelOffset == 0, "channel offset must be zero when we become non-seekable");
  mIsSeekable = aIsSeekable;
  
  
  gMediaCache->QueueUpdate();
}

PRBool
nsMediaCacheStream::IsSeekable()
{
  nsAutoMonitor mon(gMediaCache->Monitor());
  return mIsSeekable;
}

void
nsMediaCacheStream::Close()
{
  NS_ASSERTION(NS_IsMainThread(), "Only call on main thread");

  nsAutoMonitor mon(gMediaCache->Monitor());
  CloseInternal(&mon);
  
  
  
  gMediaCache->QueueUpdate();
}

void
nsMediaCacheStream::CloseInternal(nsAutoMonitor* aMonitor)
{
  NS_ASSERTION(NS_IsMainThread(), "Only call on main thread");

  if (mClosed)
    return;
  mClosed = PR_TRUE;
  gMediaCache->ReleaseStreamBlocks(this);
  
  aMonitor->NotifyAll();
}

void
nsMediaCacheStream::Pin()
{
  nsAutoMonitor mon(gMediaCache->Monitor());
  ++mPinCount;
  
  
  gMediaCache->QueueUpdate();
}

void
nsMediaCacheStream::Unpin()
{
  nsAutoMonitor mon(gMediaCache->Monitor());
  NS_ASSERTION(mPinCount > 0, "Unbalanced Unpin");
  --mPinCount;
  
  
  gMediaCache->QueueUpdate();
}

PRInt64
nsMediaCacheStream::GetLength()
{
  nsAutoMonitor mon(gMediaCache->Monitor());
  return mStreamLength;
}

PRInt64
nsMediaCacheStream::GetNextCachedData(PRInt64 aOffset)
{
  nsAutoMonitor mon(gMediaCache->Monitor());
  return GetNextCachedDataInternal(aOffset);
}

PRInt64
nsMediaCacheStream::GetCachedDataEnd(PRInt64 aOffset)
{
  nsAutoMonitor mon(gMediaCache->Monitor());
  return GetCachedDataEndInternal(aOffset);
}

PRBool
nsMediaCacheStream::IsDataCachedToEndOfStream(PRInt64 aOffset)
{
  nsAutoMonitor mon(gMediaCache->Monitor());
  if (mStreamLength < 0)
    return PR_FALSE;
  return GetCachedDataEndInternal(aOffset) >= mStreamLength;
}

PRInt64
nsMediaCacheStream::GetCachedDataEndInternal(PRInt64 aOffset)
{
  PR_ASSERT_CURRENT_THREAD_IN_MONITOR(gMediaCache->Monitor());
  PRUint32 startBlockIndex = aOffset/BLOCK_SIZE;
  PRUint32 blockIndex = startBlockIndex;
  while (blockIndex < mBlocks.Length() && mBlocks[blockIndex] != -1) {
    ++blockIndex;
  }
  PRInt64 result = blockIndex*BLOCK_SIZE;
  if (blockIndex == mChannelOffset/BLOCK_SIZE) {
    
    
    result = mChannelOffset;
  }
  if (mStreamLength >= 0) {
    
    
    result = PR_MIN(result, mStreamLength);
  }
  return PR_MAX(result, aOffset);
}

PRInt64
nsMediaCacheStream::GetNextCachedDataInternal(PRInt64 aOffset)
{
  PR_ASSERT_CURRENT_THREAD_IN_MONITOR(gMediaCache->Monitor());
  if (aOffset == mStreamLength)
    return -1;
  
  PRUint32 startBlockIndex = aOffset/BLOCK_SIZE;
  PRUint32 channelBlockIndex = mChannelOffset/BLOCK_SIZE;

  if (startBlockIndex == channelBlockIndex &&
      aOffset < mChannelOffset) {
    
    
    
    return aOffset;
  }

  if (startBlockIndex >= mBlocks.Length())
    return -1;

  
  if (mBlocks[startBlockIndex] != -1)
    return aOffset;

  
  PRBool hasPartialBlock = (mChannelOffset % BLOCK_SIZE) != 0;
  PRUint32 blockIndex = startBlockIndex + 1;
  while (PR_TRUE) {
    if ((hasPartialBlock && blockIndex == channelBlockIndex) ||
        (blockIndex < mBlocks.Length() && mBlocks[blockIndex] != -1)) {
      
      
      return blockIndex * BLOCK_SIZE;
    }

    
    if (blockIndex >= mBlocks.Length())
      return -1;

    ++blockIndex;
  }

  NS_NOTREACHED("Should return in loop");
  return -1;
}

void
nsMediaCacheStream::SetReadMode(ReadMode aMode)
{
  nsAutoMonitor mon(gMediaCache->Monitor());
  if (aMode == mCurrentMode)
    return;
  mCurrentMode = aMode;
  gMediaCache->QueueUpdate();
}

void
nsMediaCacheStream::SetPlaybackRate(PRUint32 aBytesPerSecond)
{
  NS_ASSERTION(aBytesPerSecond > 0, "Zero playback rate not allowed");
  nsAutoMonitor mon(gMediaCache->Monitor());
  if (aBytesPerSecond == mPlaybackBytesPerSecond)
    return;
  mPlaybackBytesPerSecond = aBytesPerSecond;
  gMediaCache->QueueUpdate();
}

nsresult
nsMediaCacheStream::Seek(PRInt32 aWhence, PRInt64 aOffset)
{
  NS_ASSERTION(!NS_IsMainThread(), "Don't call on main thread");

  nsAutoMonitor mon(gMediaCache->Monitor());
  if (mClosed)
    return NS_ERROR_FAILURE;

  PRInt64 oldOffset = mStreamOffset;
  switch (aWhence) {
  case PR_SEEK_END:
    if (mStreamLength < 0)
      return NS_ERROR_FAILURE;
    mStreamOffset = mStreamLength + aOffset;
    break;
  case PR_SEEK_CUR:
    mStreamOffset += aOffset;
    break;
  case PR_SEEK_SET:
    mStreamOffset = aOffset;
    break;
  default:
    NS_ERROR("Unknown whence");
    return NS_ERROR_FAILURE;
  }

  LOG(PR_LOG_DEBUG, ("Stream %p Seek to %lld", this, (long long)mStreamOffset));
  gMediaCache->NoteSeek(this, oldOffset);

  gMediaCache->QueueUpdate();
  return NS_OK;
}

PRInt64
nsMediaCacheStream::Tell()
{
  NS_ASSERTION(!NS_IsMainThread(), "Don't call on main thread");

  nsAutoMonitor mon(gMediaCache->Monitor());
  return mStreamOffset;
}

nsresult
nsMediaCacheStream::Read(char* aBuffer, PRUint32 aCount, PRUint32* aBytes)
{
  NS_ASSERTION(!NS_IsMainThread(), "Don't call on main thread");

  nsAutoMonitor mon(gMediaCache->Monitor());
  if (mClosed)
    return NS_ERROR_FAILURE;

  PRUint32 count = 0;
  
  while (count < aCount) {
    PRUint32 streamBlock = PRUint32(mStreamOffset/BLOCK_SIZE);
    PRUint32 offsetInStreamBlock =
      PRUint32(mStreamOffset - streamBlock*BLOCK_SIZE);
    PRInt32 size = PR_MIN(aCount - count, BLOCK_SIZE - offsetInStreamBlock);

    if (mStreamLength >= 0) {
      
      PRInt64 bytesRemaining = mStreamLength - mStreamOffset;
      if (bytesRemaining <= 0) {
        
        break;
      }
      size = PR_MIN(size, PRInt32(bytesRemaining));
    }

    PRInt32 bytes;
    PRUint32 channelBlock = PRUint32(mChannelOffset/BLOCK_SIZE);
    PRInt32 cacheBlock = streamBlock < mBlocks.Length() ? mBlocks[streamBlock] : -1;
    if (channelBlock == streamBlock && mStreamOffset < mChannelOffset) {
      
      
      
      bytes = PR_MIN(size, mChannelOffset - mStreamOffset);
      memcpy(aBuffer + count,
        reinterpret_cast<char*>(mPartialBlockBuffer) + offsetInStreamBlock, bytes);
      if (mCurrentMode == MODE_METADATA) {
        mMetadataInPartialBlockBuffer = PR_TRUE;
      }
      gMediaCache->NoteBlockUsage(cacheBlock, mCurrentMode, TimeStamp::Now());
    } else {
      if (cacheBlock < 0) {
        if (count > 0) {
          
          
          break;
        }

        
        mon.Wait();
        if (mClosed) {
          
          
          return NS_ERROR_FAILURE;
        }
        continue;
      }

      gMediaCache->NoteBlockUsage(cacheBlock, mCurrentMode, TimeStamp::Now());

      PRInt64 offset = cacheBlock*BLOCK_SIZE + offsetInStreamBlock;
      nsresult rv = gMediaCache->ReadCacheFile(offset, aBuffer + count, size, &bytes);
      if (NS_FAILED(rv)) {
        if (count == 0)
          return rv;
        
        break;
      }
    }
    mStreamOffset += bytes;
    count += bytes;
  }

  if (count > 0) {
    
    
    gMediaCache->QueueUpdate();
  }
  LOG(PR_LOG_DEBUG,
      ("Stream %p Read at %lld count=%d", this, (long long)(mStreamOffset-count), count));
  *aBytes = count;
  return NS_OK;
}

nsresult
nsMediaCacheStream::ReadFromCache(char* aBuffer,
                                  PRInt64 aOffset,
                                  PRInt64 aCount)
{
  nsAutoMonitor mon(gMediaCache->Monitor());
  if (mClosed)
    return NS_ERROR_FAILURE;

  
  PRUint32 count = 0;
  PRInt64 streamOffset = aOffset;
  while (count < aCount) {
    PRUint32 streamBlock = PRUint32(streamOffset/BLOCK_SIZE);
    PRUint32 offsetInStreamBlock =
      PRUint32(streamOffset - streamBlock*BLOCK_SIZE);
    PRInt32 size = PR_MIN(aCount - count, BLOCK_SIZE - offsetInStreamBlock);

    if (mStreamLength >= 0) {
      
      PRInt64 bytesRemaining = mStreamLength - streamOffset;
      if (bytesRemaining <= 0) {
        return NS_ERROR_FAILURE;
      }
      size = PR_MIN(size, PRInt32(bytesRemaining));
    }

    PRInt32 bytes;
    PRUint32 channelBlock = PRUint32(mChannelOffset/BLOCK_SIZE);
    PRInt32 cacheBlock = streamBlock < mBlocks.Length() ? mBlocks[streamBlock] : -1;
    if (channelBlock == streamBlock && streamOffset < mChannelOffset) {
      
      
      
      bytes = PR_MIN(size, mChannelOffset - streamOffset);
      memcpy(aBuffer + count,
        reinterpret_cast<char*>(mPartialBlockBuffer) + offsetInStreamBlock, bytes);
    } else {
      if (cacheBlock < 0) {
        
        return NS_ERROR_FAILURE;
      }
      PRInt64 offset = cacheBlock*BLOCK_SIZE + offsetInStreamBlock;
      nsresult rv = gMediaCache->ReadCacheFile(offset, aBuffer + count, size, &bytes);
      if (NS_FAILED(rv)) {
        return rv;
      }
    }
    streamOffset += bytes;
    count += bytes;
  }
  
  return NS_OK;
}

nsresult
nsMediaCacheStream::Init()
{
  NS_ASSERTION(NS_IsMainThread(), "Only call on main thread");

  InitMediaCache();
  if (!gMediaCache)
    return NS_ERROR_FAILURE;
  gMediaCache->OpenStream(this);
  return NS_OK;
}
