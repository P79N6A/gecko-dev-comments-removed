





#include "mozilla/ReentrantMonitor.h"
#include "mozilla/XPCOM.h"

#include "nsMediaCache.h"
#include "nsDirectoryServiceUtils.h"
#include "nsDirectoryServiceDefs.h"
#include "nsXULAppAPI.h"
#include "nsNetUtil.h"
#include "prio.h"
#include "nsContentUtils.h"
#include "nsThreadUtils.h"
#include "MediaResource.h"
#include "nsMathUtils.h"
#include "prlog.h"
#include "mozilla/Preferences.h"
#include "FileBlockCache.h"

using namespace mozilla;

#ifdef PR_LOGGING
PRLogModuleInfo* gMediaCacheLog;
#define LOG(type, msg) PR_LOG(gMediaCacheLog, type, msg)
#else
#define LOG(type, msg)
#endif





static const double NONSEEKABLE_READAHEAD_MAX = 0.5;





static const PRUint32 REPLAY_DELAY = 30;






static const PRUint32 FREE_BLOCK_SCAN_LIMIT = 16;

#ifdef DEBUG


#endif




static nsMediaCache* gMediaCache;

class nsMediaCacheFlusher : public nsIObserver,
                            public nsSupportsWeakReference {
  nsMediaCacheFlusher() {}
  ~nsMediaCacheFlusher();
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER

  static void Init();
};

static nsMediaCacheFlusher* gMediaCacheFlusher;

NS_IMPL_ISUPPORTS2(nsMediaCacheFlusher, nsIObserver, nsISupportsWeakReference)

nsMediaCacheFlusher::~nsMediaCacheFlusher()
{
  gMediaCacheFlusher = nsnull;
}

void nsMediaCacheFlusher::Init()
{
  if (gMediaCacheFlusher) {
    return;
  }

  gMediaCacheFlusher = new nsMediaCacheFlusher();
  NS_ADDREF(gMediaCacheFlusher);

  nsCOMPtr<nsIObserverService> observerService =
    mozilla::services::GetObserverService();
  if (observerService) {
    observerService->AddObserver(gMediaCacheFlusher, "last-pb-context-exited", true);
  }
}

class nsMediaCache {
public:
  friend class nsMediaCacheStream::BlockList;
  typedef nsMediaCacheStream::BlockList BlockList;
  enum {
    BLOCK_SIZE = nsMediaCacheStream::BLOCK_SIZE
  };

  nsMediaCache() : mNextResourceID(1),
    mReentrantMonitor("nsMediaCache.mReentrantMonitor"),
    mUpdateQueued(false)
#ifdef DEBUG
    , mInUpdate(false)
#endif
  {
    MOZ_COUNT_CTOR(nsMediaCache);
  }
  ~nsMediaCache() {
    NS_ASSERTION(mStreams.IsEmpty(), "Stream(s) still open!");
    Truncate();
    NS_ASSERTION(mIndex.Length() == 0, "Blocks leaked?");
    if (mFileCache) {
      mFileCache->Close();
      mFileCache = nsnull;
    }
    MOZ_COUNT_DTOR(nsMediaCache);
  }

  
  
  
  nsresult Init();
  
  
  
  
  
  static void MaybeShutdown();

  
  static void Flush();
  void FlushInternal();

  
  
  
  nsresult ReadCacheFile(PRInt64 aOffset, void* aData, PRInt32 aLength,
                         PRInt32* aBytes);
  
  nsresult ReadCacheFileAllBytes(PRInt64 aOffset, void* aData, PRInt32 aLength);

  PRInt64 AllocateResourceID()
  {
    mReentrantMonitor.AssertCurrentThreadIn();
    return mNextResourceID++;
  }

  
  
  
  
  void OpenStream(nsMediaCacheStream* aStream);
  
  void ReleaseStream(nsMediaCacheStream* aStream);
  
  void ReleaseStreamBlocks(nsMediaCacheStream* aStream);
  
  void AllocateAndWriteBlock(nsMediaCacheStream* aStream, const void* aData,
                             nsMediaCacheStream::ReadMode aMode);

  
  
  
  
  
  void NoteSeek(nsMediaCacheStream* aStream, PRInt64 aOldOffset);
  
  
  
  
  
  
  void NoteBlockUsage(nsMediaCacheStream* aStream, PRInt32 aBlockIndex,
                      nsMediaCacheStream::ReadMode aMode, TimeStamp aNow);
  
  void AddBlockOwnerAsReadahead(PRInt32 aBlockIndex, nsMediaCacheStream* aStream,
                                PRInt32 aStreamBlockIndex);

  
  void QueueUpdate();

  
  
  
  
  
  
  
  void Update();

#ifdef DEBUG_VERIFY_CACHE
  
  void Verify();
#else
  void Verify() {}
#endif

  ReentrantMonitor& GetReentrantMonitor() { return mReentrantMonitor; }

  




  class ResourceStreamIterator {
  public:
    ResourceStreamIterator(PRInt64 aResourceID) :
      mResourceID(aResourceID), mNext(0) {}
    nsMediaCacheStream* Next()
    {
      while (mNext < gMediaCache->mStreams.Length()) {
        nsMediaCacheStream* stream = gMediaCache->mStreams[mNext];
        ++mNext;
        if (stream->GetResourceID() == mResourceID && !stream->IsClosed())
          return stream;
      }
      return nsnull;
    }
  private:
    PRInt64  mResourceID;
    PRUint32 mNext;
  };

protected:
  
  
  
  PRInt32 FindBlockForIncomingData(TimeStamp aNow, nsMediaCacheStream* aStream);
  
  
  
  
  
  
  
  PRInt32 FindReusableBlock(TimeStamp aNow,
                            nsMediaCacheStream* aForStream,
                            PRInt32 aForStreamBlock,
                            PRInt32 aMaxSearchBlockIndex);
  bool BlockIsReusable(PRInt32 aBlockIndex);
  
  
  
  
  void AppendMostReusableBlock(BlockList* aBlockList,
                               nsTArray<PRUint32>* aResult,
                               PRInt32 aBlockIndexLimit);

  enum BlockClass {
    
    
    
    
    METADATA_BLOCK,
    
    
    PLAYED_BLOCK,
    
    
    
    READAHEAD_BLOCK
  };

  struct BlockOwner {
    BlockOwner() : mStream(nsnull), mClass(READAHEAD_BLOCK) {}

    
    nsMediaCacheStream* mStream;
    
    PRUint32            mStreamBlock;
    
    
    TimeStamp           mLastUseTime;
    BlockClass          mClass;
  };

  struct Block {
    
    nsTArray<BlockOwner> mOwners;
  };

  
  
  BlockList* GetListForBlock(BlockOwner* aBlock);
  
  
  BlockOwner* GetBlockOwner(PRInt32 aBlockIndex, nsMediaCacheStream* aStream);
  
  bool IsBlockFree(PRInt32 aBlockIndex)
  { return mIndex[aBlockIndex].mOwners.IsEmpty(); }
  
  
  void FreeBlock(PRInt32 aBlock);
  
  
  void RemoveBlockOwner(PRInt32 aBlockIndex, nsMediaCacheStream* aStream);
  
  
  void SwapBlocks(PRInt32 aBlockIndex1, PRInt32 aBlockIndex2);
  
  
  void InsertReadaheadBlock(BlockOwner* aBlockOwner, PRInt32 aBlockIndex);

  
  TimeDuration PredictNextUse(TimeStamp aNow, PRInt32 aBlock);
  
  TimeDuration PredictNextUseForIncomingData(nsMediaCacheStream* aStream);

  
  
  void Truncate();

  
  
  PRInt64                       mNextResourceID;

  
  
  
  ReentrantMonitor         mReentrantMonitor;
  
  
  nsTArray<nsMediaCacheStream*> mStreams;
  
  nsTArray<Block> mIndex;
  
  nsRefPtr<FileBlockCache> mFileCache;
  
  BlockList       mFreeBlocks;
  
  bool            mUpdateQueued;
#ifdef DEBUG
  bool            mInUpdate;
#endif
};

NS_IMETHODIMP
nsMediaCacheFlusher::Observe(nsISupports *aSubject, char const *aTopic, PRUnichar const *aData)
{
  if (strcmp(aTopic, "last-pb-context-exited") == 0) {
    nsMediaCache::Flush();
  }
  return NS_OK;
}

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
    
    e2 = mEntries.GetEntry(aBlockIndex2);
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
  NS_ASSERTION(!mFileCache, "Cache file already open?");

  
  
  
  
  nsresult rv;
  nsCOMPtr<nsIFile> tmp;
  const char* dir = (XRE_GetProcessType() == GeckoProcessType_Content) ?
    NS_OS_TEMP_DIR : NS_APP_USER_PROFILE_LOCAL_50_DIR;
  rv = NS_GetSpecialDirectory(dir, getter_AddRefs(tmp));
  NS_ENSURE_SUCCESS(rv,rv);

  nsCOMPtr<nsILocalFile> tmpFile = do_QueryInterface(tmp);
  NS_ENSURE_TRUE(tmpFile != nsnull, NS_ERROR_FAILURE);

  
  
  rv = tmpFile->AppendNative(nsDependentCString("mozilla-media-cache"));
  NS_ENSURE_SUCCESS(rv,rv);

  rv = tmpFile->Create(nsIFile::DIRECTORY_TYPE, 0700);
  if (rv == NS_ERROR_FILE_ALREADY_EXISTS) {
    
    
    
    PRUint32 perms;
    rv = tmpFile->GetPermissions(&perms);
    NS_ENSURE_SUCCESS(rv,rv);
    if (perms != 0700) {
      rv = tmpFile->SetPermissions(0700);
      NS_ENSURE_SUCCESS(rv,rv);
    }
  } else {
    NS_ENSURE_SUCCESS(rv,rv);
  }

  rv = tmpFile->AppendNative(nsDependentCString("media_cache"));
  NS_ENSURE_SUCCESS(rv,rv);

  rv = tmpFile->CreateUnique(nsIFile::NORMAL_FILE_TYPE, 0700);
  NS_ENSURE_SUCCESS(rv,rv);

  PRFileDesc* fileDesc = nsnull;
  rv = tmpFile->OpenNSPRFileDesc(PR_RDWR | nsILocalFile::DELETE_ON_CLOSE,
                                 PR_IRWXU, &fileDesc);
  NS_ENSURE_SUCCESS(rv,rv);

  mFileCache = new FileBlockCache();
  rv = mFileCache->Open(fileDesc);
  NS_ENSURE_SUCCESS(rv,rv);

#ifdef PR_LOGGING
  if (!gMediaCacheLog) {
    gMediaCacheLog = PR_NewLogModule("nsMediaCache");
  }
#endif

  nsMediaCacheFlusher::Init();

  return NS_OK;
}

void
nsMediaCache::Flush()
{
  NS_ASSERTION(NS_IsMainThread(), "Only call on main thread");

  if (!gMediaCache)
    return;

  gMediaCache->FlushInternal();
}

void
nsMediaCache::FlushInternal()
{
  ReentrantMonitorAutoEnter mon(mReentrantMonitor);

  for (PRUint32 blockIndex = 0; blockIndex < mIndex.Length(); ++blockIndex) {
    FreeBlock(blockIndex);
  }

  
  Truncate();
  NS_ASSERTION(mIndex.Length() == 0, "Blocks leaked?");
  if (mFileCache) {
    mFileCache->Close();
    mFileCache = nsnull;
  }
  Init();
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
  NS_IF_RELEASE(gMediaCacheFlusher);
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
  mReentrantMonitor.AssertCurrentThreadIn();

  if (!mFileCache)
    return NS_ERROR_FAILURE;

  return mFileCache->Read(aOffset, reinterpret_cast<PRUint8*>(aData), aLength, aBytes);
}

nsresult
nsMediaCache::ReadCacheFileAllBytes(PRInt64 aOffset, void* aData, PRInt32 aLength)
{
  mReentrantMonitor.AssertCurrentThreadIn();

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

static PRInt32 GetMaxBlocks()
{
  
  
  
  PRInt32 cacheSize = Preferences::GetInt("media.cache_size", 500*1024);
  PRInt64 maxBlocks = static_cast<PRInt64>(cacheSize)*1024/nsMediaCache::BLOCK_SIZE;
  maxBlocks = NS_MAX<PRInt64>(maxBlocks, 1);
  return PRInt32(NS_MIN<PRInt64>(maxBlocks, PR_INT32_MAX));
}

PRInt32
nsMediaCache::FindBlockForIncomingData(TimeStamp aNow,
                                       nsMediaCacheStream* aStream)
{
  mReentrantMonitor.AssertCurrentThreadIn();

  PRInt32 blockIndex = FindReusableBlock(aNow, aStream,
      aStream->mChannelOffset/BLOCK_SIZE, PR_INT32_MAX);

  if (blockIndex < 0 || !IsBlockFree(blockIndex)) {
    
    
    
    
    
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

bool
nsMediaCache::BlockIsReusable(PRInt32 aBlockIndex)
{
  Block* block = &mIndex[aBlockIndex];
  for (PRUint32 i = 0; i < block->mOwners.Length(); ++i) {
    nsMediaCacheStream* stream = block->mOwners[i].mStream;
    if (stream->mPinCount > 0 ||
        stream->mStreamOffset/BLOCK_SIZE == block->mOwners[i].mStreamBlock) {
      return false;
    }
  }
  return true;
}

void
nsMediaCache::AppendMostReusableBlock(BlockList* aBlockList,
                                      nsTArray<PRUint32>* aResult,
                                      PRInt32 aBlockIndexLimit)
{
  mReentrantMonitor.AssertCurrentThreadIn();

  PRInt32 blockIndex = aBlockList->GetLastBlock();
  if (blockIndex < 0)
    return;
  do {
    
    
    
    
    if (blockIndex < aBlockIndexLimit && BlockIsReusable(blockIndex)) {
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
  mReentrantMonitor.AssertCurrentThreadIn();

  PRUint32 length = NS_MIN(PRUint32(aMaxSearchBlockIndex), mIndex.Length());

  if (aForStream && aForStreamBlock > 0 &&
      PRUint32(aForStreamBlock) <= aForStream->mBlocks.Length()) {
    PRInt32 prevCacheBlock = aForStream->mBlocks[aForStreamBlock - 1];
    if (prevCacheBlock >= 0) {
      PRUint32 freeBlockScanEnd =
        NS_MIN(length, prevCacheBlock + FREE_BLOCK_SCAN_LIMIT);
      for (PRUint32 i = prevCacheBlock; i < freeBlockScanEnd; ++i) {
        if (IsBlockFree(i))
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
  for (PRUint32 i = 0; i < mStreams.Length(); ++i) {
    nsMediaCacheStream* stream = mStreams[i];
    if (stream->mPinCount > 0) {
      
      continue;
    }

    AppendMostReusableBlock(&stream->mMetadataBlocks, &candidates, length);
    AppendMostReusableBlock(&stream->mPlayedBlocks, &candidates, length);

    
    
    if (stream->mIsSeekable) {
      AppendMostReusableBlock(&stream->mReadaheadBlocks, &candidates, length);
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

  return latestUseBlock;
}

nsMediaCache::BlockList*
nsMediaCache::GetListForBlock(BlockOwner* aBlock)
{
  switch (aBlock->mClass) {
  case METADATA_BLOCK:
    NS_ASSERTION(aBlock->mStream, "Metadata block has no stream?");
    return &aBlock->mStream->mMetadataBlocks;
  case PLAYED_BLOCK:
    NS_ASSERTION(aBlock->mStream, "Metadata block has no stream?");
    return &aBlock->mStream->mPlayedBlocks;
  case READAHEAD_BLOCK:
    NS_ASSERTION(aBlock->mStream, "Readahead block has no stream?");
    return &aBlock->mStream->mReadaheadBlocks;
  default:
    NS_ERROR("Invalid block class");
    return nsnull;
  }
}

nsMediaCache::BlockOwner*
nsMediaCache::GetBlockOwner(PRInt32 aBlockIndex, nsMediaCacheStream* aStream)
{
  Block* block = &mIndex[aBlockIndex];
  for (PRUint32 i = 0; i < block->mOwners.Length(); ++i) {
    if (block->mOwners[i].mStream == aStream)
      return &block->mOwners[i];
  }
  return nsnull;
}

void
nsMediaCache::SwapBlocks(PRInt32 aBlockIndex1, PRInt32 aBlockIndex2)
{
  mReentrantMonitor.AssertCurrentThreadIn();

  Block* block1 = &mIndex[aBlockIndex1];
  Block* block2 = &mIndex[aBlockIndex2];

  block1->mOwners.SwapElements(block2->mOwners);

  
  
  
  const Block* blocks[] = { block1, block2 };
  PRInt32 blockIndices[] = { aBlockIndex1, aBlockIndex2 };
  for (PRInt32 i = 0; i < 2; ++i) {
    for (PRUint32 j = 0; j < blocks[i]->mOwners.Length(); ++j) {
      const BlockOwner* b = &blocks[i]->mOwners[j];
      b->mStream->mBlocks[b->mStreamBlock] = blockIndices[i];
    }
  }

  
  mFreeBlocks.NotifyBlockSwapped(aBlockIndex1, aBlockIndex2);

  nsTHashtable<nsPtrHashKey<nsMediaCacheStream> > visitedStreams;
  visitedStreams.Init();

  for (PRInt32 i = 0; i < 2; ++i) {
    for (PRUint32 j = 0; j < blocks[i]->mOwners.Length(); ++j) {
      nsMediaCacheStream* stream = blocks[i]->mOwners[j].mStream;
      
      
      if (visitedStreams.GetEntry(stream))
        continue;
      visitedStreams.PutEntry(stream);
      stream->mReadaheadBlocks.NotifyBlockSwapped(aBlockIndex1, aBlockIndex2);
      stream->mPlayedBlocks.NotifyBlockSwapped(aBlockIndex1, aBlockIndex2);
      stream->mMetadataBlocks.NotifyBlockSwapped(aBlockIndex1, aBlockIndex2);
    }
  }

  Verify();
}

void
nsMediaCache::RemoveBlockOwner(PRInt32 aBlockIndex, nsMediaCacheStream* aStream)
{
  Block* block = &mIndex[aBlockIndex];
  for (PRUint32 i = 0; i < block->mOwners.Length(); ++i) {
    BlockOwner* bo = &block->mOwners[i];
    if (bo->mStream == aStream) {
      GetListForBlock(bo)->RemoveBlock(aBlockIndex);
      bo->mStream->mBlocks[bo->mStreamBlock] = -1;
      block->mOwners.RemoveElementAt(i);
      if (block->mOwners.IsEmpty()) {
        mFreeBlocks.AddFirstBlock(aBlockIndex);
      }
      return;
    }
  }
}

void
nsMediaCache::AddBlockOwnerAsReadahead(PRInt32 aBlockIndex,
                                       nsMediaCacheStream* aStream,
                                       PRInt32 aStreamBlockIndex)
{
  Block* block = &mIndex[aBlockIndex];
  if (block->mOwners.IsEmpty()) {
    mFreeBlocks.RemoveBlock(aBlockIndex);
  }
  BlockOwner* bo = block->mOwners.AppendElement();
  bo->mStream = aStream;
  bo->mStreamBlock = aStreamBlockIndex;
  aStream->mBlocks[aStreamBlockIndex] = aBlockIndex;
  bo->mClass = READAHEAD_BLOCK;
  InsertReadaheadBlock(bo, aBlockIndex);
}

void
nsMediaCache::FreeBlock(PRInt32 aBlock)
{
  mReentrantMonitor.AssertCurrentThreadIn();

  Block* block = &mIndex[aBlock];
  if (block->mOwners.IsEmpty()) {
    
    return;
  }

  LOG(PR_LOG_DEBUG, ("Released block %d", aBlock));

  for (PRUint32 i = 0; i < block->mOwners.Length(); ++i) {
    BlockOwner* bo = &block->mOwners[i];
    GetListForBlock(bo)->RemoveBlock(aBlock);
    bo->mStream->mBlocks[bo->mStreamBlock] = -1;
  }
  block->mOwners.Clear();
  mFreeBlocks.AddFirstBlock(aBlock);
  Verify();
}

TimeDuration
nsMediaCache::PredictNextUse(TimeStamp aNow, PRInt32 aBlock)
{
  mReentrantMonitor.AssertCurrentThreadIn();
  NS_ASSERTION(!IsBlockFree(aBlock), "aBlock is free");

  Block* block = &mIndex[aBlock];
  
  
  TimeDuration result;
  for (PRUint32 i = 0; i < block->mOwners.Length(); ++i) {
    BlockOwner* bo = &block->mOwners[i];
    TimeDuration prediction;
    switch (bo->mClass) {
    case METADATA_BLOCK:
      
      
      prediction = aNow - bo->mLastUseTime;
      break;
    case PLAYED_BLOCK:
      
      
      NS_ASSERTION(static_cast<PRInt64>(bo->mStreamBlock)*BLOCK_SIZE <
                   bo->mStream->mStreamOffset,
                   "Played block after the current stream position?");
      prediction = aNow - bo->mLastUseTime +
        TimeDuration::FromSeconds(REPLAY_DELAY);
      break;
    case READAHEAD_BLOCK: {
      PRInt64 bytesAhead =
        static_cast<PRInt64>(bo->mStreamBlock)*BLOCK_SIZE - bo->mStream->mStreamOffset;
      NS_ASSERTION(bytesAhead >= 0,
                   "Readahead block before the current stream position?");
      PRInt64 millisecondsAhead =
        bytesAhead*1000/bo->mStream->mPlaybackBytesPerSecond;
      prediction = TimeDuration::FromMilliseconds(
          NS_MIN<PRInt64>(millisecondsAhead, PR_INT32_MAX));
      break;
    }
    default:
      NS_ERROR("Invalid class for predicting next use");
      return TimeDuration(0);
    }
    if (i == 0 || prediction < result) {
      result = prediction;
    }
  }
  return result;
}

TimeDuration
nsMediaCache::PredictNextUseForIncomingData(nsMediaCacheStream* aStream)
{
  mReentrantMonitor.AssertCurrentThreadIn();

  PRInt64 bytesAhead = aStream->mChannelOffset - aStream->mStreamOffset;
  if (bytesAhead <= -BLOCK_SIZE) {
    
    return TimeDuration::FromSeconds(24*60*60);
  }
  if (bytesAhead <= 0)
    return TimeDuration(0);
  PRInt64 millisecondsAhead = bytesAhead*1000/aStream->mPlaybackBytesPerSecond;
  return TimeDuration::FromMilliseconds(
      NS_MIN<PRInt64>(millisecondsAhead, PR_INT32_MAX));
}

enum StreamAction { NONE, SEEK, SEEK_AND_RESUME, RESUME, SUSPEND };

void
nsMediaCache::Update()
{
  NS_ASSERTION(NS_IsMainThread(), "Only call on main thread");

  
  
  
  
  nsAutoTArray<StreamAction,10> actions;

  {
    ReentrantMonitorAutoEnter mon(mReentrantMonitor);
    mUpdateQueued = false;
#ifdef DEBUG
    mInUpdate = true;
#endif

    PRInt32 maxBlocks = GetMaxBlocks();
    TimeStamp now = TimeStamp::Now();

    PRInt32 freeBlockCount = mFreeBlocks.GetCount();
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    TimeDuration latestPredictedUseForOverflow = 0;
    for (PRInt32 blockIndex = mIndex.Length() - 1; blockIndex >= maxBlocks;
         --blockIndex) {
      if (IsBlockFree(blockIndex)) {
        
        --freeBlockCount;
        continue;
      }
      TimeDuration predictedUse = PredictNextUse(now, blockIndex);
      latestPredictedUseForOverflow = NS_MAX(latestPredictedUseForOverflow, predictedUse);
    }

    
    for (PRInt32 blockIndex = mIndex.Length() - 1; blockIndex >= maxBlocks;
         --blockIndex) {
      if (IsBlockFree(blockIndex))
        continue;

      Block* block = &mIndex[blockIndex];
      
      
      
      PRInt32 destinationBlockIndex =
        FindReusableBlock(now, block->mOwners[0].mStream,
                          block->mOwners[0].mStreamBlock, maxBlocks);
      if (destinationBlockIndex < 0) {
        
        
        break;
      }

      if (IsBlockFree(destinationBlockIndex) ||
          PredictNextUse(now, destinationBlockIndex) > latestPredictedUseForOverflow) {
        
        

        nsresult rv = mFileCache->MoveBlock(blockIndex, destinationBlockIndex);

        if (NS_SUCCEEDED(rv)) {
          
          LOG(PR_LOG_DEBUG, ("Swapping blocks %d and %d (trimming cache)",
              blockIndex, destinationBlockIndex));
          
          
          SwapBlocks(blockIndex, destinationBlockIndex);
          
          LOG(PR_LOG_DEBUG, ("Released block %d (trimming cache)", blockIndex));
          FreeBlock(blockIndex);
        }
      } else {
        LOG(PR_LOG_DEBUG, ("Could not trim cache block %d (destination %d, predicted next use %f, latest predicted use for overflow %f",
                           blockIndex, destinationBlockIndex,
                           PredictNextUse(now, destinationBlockIndex).ToSeconds(),
                           latestPredictedUseForOverflow.ToSeconds()));
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

    for (PRUint32 i = 0; i < mStreams.Length(); ++i) {
      actions.AppendElement(NONE);

      nsMediaCacheStream* stream = mStreams[i];
      if (stream->mClosed)
        continue;

      
      
      PRInt64 dataOffset = stream->GetCachedDataEndInternal(stream->mStreamOffset);

      
      PRInt64 desiredOffset = dataOffset;
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

      
      
      bool enableReading;
      if (stream->mStreamLength >= 0 && dataOffset >= stream->mStreamLength) {
        
        
        
        
        
        
        
        
        
        LOG(PR_LOG_DEBUG, ("Stream %p at end of stream", stream));
        enableReading = !stream->mCacheSuspended &&
          stream->mStreamLength == stream->mChannelOffset;
      } else if (desiredOffset < stream->mStreamOffset) {
        
        
        LOG(PR_LOG_DEBUG, ("Stream %p catching up", stream));
        enableReading = true;
      } else if (desiredOffset < stream->mStreamOffset + BLOCK_SIZE) {
        
        LOG(PR_LOG_DEBUG, ("Stream %p feeding reader", stream));
        enableReading = true;
      } else if (!stream->mIsSeekable &&
                 nonSeekableReadaheadBlockCount >= maxBlocks*NONSEEKABLE_READAHEAD_MAX) {
        
        
        
        LOG(PR_LOG_DEBUG, ("Stream %p throttling non-seekable readahead", stream));
        enableReading = false;
      } else if (mIndex.Length() > PRUint32(maxBlocks)) {
        
        
        LOG(PR_LOG_DEBUG, ("Stream %p throttling to reduce cache size", stream));
        enableReading = false;
      } else if (freeBlockCount > 0 || mIndex.Length() < PRUint32(maxBlocks)) {
        
        LOG(PR_LOG_DEBUG, ("Stream %p reading since there are free blocks", stream));
        enableReading = true;
      } else if (latestNextUse <= TimeDuration(0)) {
        
        LOG(PR_LOG_DEBUG, ("Stream %p throttling due to no reusable blocks", stream));
        enableReading = false;
      } else {
        
        
        TimeDuration predictedNewDataUse = PredictNextUseForIncomingData(stream);
        LOG(PR_LOG_DEBUG, ("Stream %p predict next data in %f, current worst block is %f",
            stream, predictedNewDataUse.ToSeconds(), latestNextUse.ToSeconds()));
        enableReading = predictedNewDataUse < latestNextUse;
      }

      if (enableReading) {
        for (PRUint32 j = 0; j < i; ++j) {
          nsMediaCacheStream* other = mStreams[j];
          if (other->mResourceID == stream->mResourceID &&
              !other->mClient->IsSuspended() &&
              other->mChannelOffset/BLOCK_SIZE == desiredOffset/BLOCK_SIZE) {
            
            
            enableReading = false;
            LOG(PR_LOG_DEBUG, ("Stream %p waiting on same block (%lld) from stream %p",
                               stream, desiredOffset/BLOCK_SIZE, other));
            break;
          }
        }
      }

      if (stream->mChannelOffset != desiredOffset && enableReading) {
        
        NS_ASSERTION(stream->mIsSeekable || desiredOffset == 0,
                     "Trying to seek in a non-seekable stream!");
        
        
        
        stream->mChannelOffset = (desiredOffset/BLOCK_SIZE)*BLOCK_SIZE;
        actions[i] = stream->mCacheSuspended ? SEEK_AND_RESUME : SEEK;
      } else if (enableReading && stream->mCacheSuspended) {
        actions[i] = RESUME;
      } else if (!enableReading && !stream->mCacheSuspended) {
        actions[i] = SUSPEND;
      }
    }
#ifdef DEBUG
    mInUpdate = false;
#endif
  }

  
  
  
  
  
  
  

  
  
  
  for (PRUint32 i = 0; i < mStreams.Length(); ++i) {
    nsMediaCacheStream* stream = mStreams[i];
    switch (actions[i]) {
    case SEEK:
	case SEEK_AND_RESUME:
      stream->mCacheSuspended = false;
      stream->mChannelEnded = false;
      break;
    case RESUME:
      stream->mCacheSuspended = false;
      break;
    case SUSPEND:
      stream->mCacheSuspended = true;
      break;
    default:
      break;
    }
    stream->mHasHadUpdate = true;
  }

  for (PRUint32 i = 0; i < mStreams.Length(); ++i) {
    nsMediaCacheStream* stream = mStreams[i];
    nsresult rv;
    switch (actions[i]) {
    case SEEK:
	case SEEK_AND_RESUME:
      LOG(PR_LOG_DEBUG, ("Stream %p CacheSeek to %lld (resume=%d)", stream,
           (long long)stream->mChannelOffset, actions[i] == SEEK_AND_RESUME));
      rv = stream->mClient->CacheClientSeek(stream->mChannelOffset,
                                            actions[i] == SEEK_AND_RESUME);
      break;
    case RESUME:
      LOG(PR_LOG_DEBUG, ("Stream %p Resumed", stream));
      rv = stream->mClient->CacheClientResume();
      break;
    case SUSPEND:
      LOG(PR_LOG_DEBUG, ("Stream %p Suspended", stream));
      rv = stream->mClient->CacheClientSuspend();
      break;
    default:
      rv = NS_OK;
      break;
    }

    if (NS_FAILED(rv)) {
      
      
      
      ReentrantMonitorAutoEnter mon(mReentrantMonitor);
      stream->CloseInternal(mon);
    }
  }
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
  mReentrantMonitor.AssertCurrentThreadIn();

  
  
  NS_ASSERTION(!mInUpdate,
               "Queuing an update while we're in an update");
  if (mUpdateQueued)
    return;
  mUpdateQueued = true;
  nsCOMPtr<nsIRunnable> event = new UpdateEvent();
  NS_DispatchToMainThread(event);
}

#ifdef DEBUG_VERIFY_CACHE
void
nsMediaCache::Verify()
{
  mReentrantMonitor.AssertCurrentThreadIn();

  mFreeBlocks.Verify();
  for (PRUint32 i = 0; i < mStreams.Length(); ++i) {
    nsMediaCacheStream* stream = mStreams[i];
    stream->mReadaheadBlocks.Verify();
    stream->mPlayedBlocks.Verify();
    stream->mMetadataBlocks.Verify();

    
    PRInt32 block = stream->mReadaheadBlocks.GetFirstBlock();
    PRInt32 lastStreamBlock = -1;
    while (block >= 0) {
      PRUint32 j = 0;
      while (mIndex[block].mOwners[j].mStream != stream) {
        ++j;
      }
      PRInt32 nextStreamBlock =
        PRInt32(mIndex[block].mOwners[j].mStreamBlock);
      NS_ASSERTION(lastStreamBlock < nextStreamBlock,
                   "Blocks not increasing in readahead stream");
      lastStreamBlock = nextStreamBlock;
      block = stream->mReadaheadBlocks.GetNextBlock(block);
    }
  }
}
#endif

void
nsMediaCache::InsertReadaheadBlock(BlockOwner* aBlockOwner,
                                   PRInt32 aBlockIndex)
{
  mReentrantMonitor.AssertCurrentThreadIn();

  
  
  nsMediaCacheStream* stream = aBlockOwner->mStream;
  PRInt32 readaheadIndex = stream->mReadaheadBlocks.GetLastBlock();
  while (readaheadIndex >= 0) {
    BlockOwner* bo = GetBlockOwner(readaheadIndex, stream);
    NS_ASSERTION(bo, "stream must own its blocks");
    if (bo->mStreamBlock < aBlockOwner->mStreamBlock) {
      stream->mReadaheadBlocks.AddAfter(aBlockIndex, readaheadIndex);
      return;
    }
    NS_ASSERTION(bo->mStreamBlock > aBlockOwner->mStreamBlock,
                 "Duplicated blocks??");
    readaheadIndex = stream->mReadaheadBlocks.GetPrevBlock(readaheadIndex);
  }

  stream->mReadaheadBlocks.AddFirstBlock(aBlockIndex);
  Verify();
}

void
nsMediaCache::AllocateAndWriteBlock(nsMediaCacheStream* aStream, const void* aData,
                                    nsMediaCacheStream::ReadMode aMode)
{
  mReentrantMonitor.AssertCurrentThreadIn();

  PRInt32 streamBlockIndex = aStream->mChannelOffset/BLOCK_SIZE;

  
  ResourceStreamIterator iter(aStream->mResourceID);
  while (nsMediaCacheStream* stream = iter.Next()) {
    while (streamBlockIndex >= PRInt32(stream->mBlocks.Length())) {
      stream->mBlocks.AppendElement(-1);
    }
    if (stream->mBlocks[streamBlockIndex] >= 0) {
      
      PRInt32 globalBlockIndex = stream->mBlocks[streamBlockIndex];
      LOG(PR_LOG_DEBUG, ("Released block %d from stream %p block %d(%lld)",
          globalBlockIndex, stream, streamBlockIndex, (long long)streamBlockIndex*BLOCK_SIZE));
      RemoveBlockOwner(globalBlockIndex, stream);
    }
  }

  

  TimeStamp now = TimeStamp::Now();
  PRInt32 blockIndex = FindBlockForIncomingData(now, aStream);
  if (blockIndex >= 0) {
    FreeBlock(blockIndex);

    Block* block = &mIndex[blockIndex];
    LOG(PR_LOG_DEBUG, ("Allocated block %d to stream %p block %d(%lld)",
        blockIndex, aStream, streamBlockIndex, (long long)streamBlockIndex*BLOCK_SIZE));

    mFreeBlocks.RemoveBlock(blockIndex);

    
    ResourceStreamIterator iter(aStream->mResourceID);
    while (nsMediaCacheStream* stream = iter.Next()) {
      BlockOwner* bo = block->mOwners.AppendElement();
      if (!bo)
        return;

      bo->mStream = stream;
      bo->mStreamBlock = streamBlockIndex;
      bo->mLastUseTime = now;
      stream->mBlocks[streamBlockIndex] = blockIndex;
      if (streamBlockIndex*BLOCK_SIZE < stream->mStreamOffset) {
        bo->mClass = aMode == nsMediaCacheStream::MODE_PLAYBACK
          ? PLAYED_BLOCK : METADATA_BLOCK;
        
        
        
        GetListForBlock(bo)->AddFirstBlock(blockIndex);
        Verify();
      } else {
        
        
        
        bo->mClass = READAHEAD_BLOCK;
        InsertReadaheadBlock(bo, blockIndex);
      }
    }

    nsresult rv = mFileCache->WriteBlock(blockIndex, reinterpret_cast<const PRUint8*>(aData));
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

  ReentrantMonitorAutoEnter mon(mReentrantMonitor);
  LOG(PR_LOG_DEBUG, ("Stream %p opened", aStream));
  mStreams.AppendElement(aStream);
  aStream->mResourceID = AllocateResourceID();

  
  gMediaCache->QueueUpdate();
}

void
nsMediaCache::ReleaseStream(nsMediaCacheStream* aStream)
{
  NS_ASSERTION(NS_IsMainThread(), "Only call on main thread");

  ReentrantMonitorAutoEnter mon(mReentrantMonitor);
  LOG(PR_LOG_DEBUG, ("Stream %p closed", aStream));
  mStreams.RemoveElement(aStream);
}

void
nsMediaCache::ReleaseStreamBlocks(nsMediaCacheStream* aStream)
{
  mReentrantMonitor.AssertCurrentThreadIn();

  
  
  
  PRUint32 length = aStream->mBlocks.Length();
  for (PRUint32 i = 0; i < length; ++i) {
    PRInt32 blockIndex = aStream->mBlocks[i];
    if (blockIndex >= 0) {
      LOG(PR_LOG_DEBUG, ("Released block %d from stream %p block %d(%lld)",
          blockIndex, aStream, i, (long long)i*BLOCK_SIZE));
      RemoveBlockOwner(blockIndex, aStream);
    }
  }
}

void
nsMediaCache::Truncate()
{
  PRUint32 end;
  for (end = mIndex.Length(); end > 0; --end) {
    if (!IsBlockFree(end - 1))
      break;
    mFreeBlocks.RemoveBlock(end - 1);
  }

  if (end < mIndex.Length()) {
    mIndex.TruncateLength(end);
    
    
    
    
  }
}

void
nsMediaCache::NoteBlockUsage(nsMediaCacheStream* aStream, PRInt32 aBlockIndex,
                             nsMediaCacheStream::ReadMode aMode,
                             TimeStamp aNow)
{
  mReentrantMonitor.AssertCurrentThreadIn();

  if (aBlockIndex < 0) {
    
    return;
  }

  BlockOwner* bo = GetBlockOwner(aBlockIndex, aStream);
  if (!bo) {
    
    return;
  }

  
  
  NS_ASSERTION(bo->mStreamBlock*BLOCK_SIZE <= bo->mStream->mStreamOffset,
               "Using a block that's behind the read position?");

  GetListForBlock(bo)->RemoveBlock(aBlockIndex);
  bo->mClass =
    (aMode == nsMediaCacheStream::MODE_METADATA || bo->mClass == METADATA_BLOCK)
    ? METADATA_BLOCK : PLAYED_BLOCK;
  
  
  GetListForBlock(bo)->AddFirstBlock(aBlockIndex);
  bo->mLastUseTime = aNow;
  Verify();
}

void
nsMediaCache::NoteSeek(nsMediaCacheStream* aStream, PRInt64 aOldOffset)
{
  mReentrantMonitor.AssertCurrentThreadIn();

  if (aOldOffset < aStream->mStreamOffset) {
    
    
    
    PRInt32 blockIndex = aOldOffset/BLOCK_SIZE;
    PRInt32 endIndex =
      NS_MIN<PRInt64>((aStream->mStreamOffset + BLOCK_SIZE - 1)/BLOCK_SIZE,
             aStream->mBlocks.Length());
    TimeStamp now = TimeStamp::Now();
    while (blockIndex < endIndex) {
      PRInt32 cacheBlockIndex = aStream->mBlocks[blockIndex];
      if (cacheBlockIndex >= 0) {
        
        
        NoteBlockUsage(aStream, cacheBlockIndex, nsMediaCacheStream::MODE_PLAYBACK,
                       now);
      }
      ++blockIndex;
    }
  } else {
    
    
    
    PRInt32 blockIndex =
      (aStream->mStreamOffset + BLOCK_SIZE - 1)/BLOCK_SIZE;
    PRInt32 endIndex =
      NS_MIN<PRInt64>((aOldOffset + BLOCK_SIZE - 1)/BLOCK_SIZE,
             aStream->mBlocks.Length());
    while (blockIndex < endIndex) {
      PRInt32 cacheBlockIndex = aStream->mBlocks[endIndex - 1];
      if (cacheBlockIndex >= 0) {
        BlockOwner* bo = GetBlockOwner(cacheBlockIndex, aStream);
        NS_ASSERTION(bo, "Stream doesn't own its blocks?");
        if (bo->mClass == PLAYED_BLOCK) {
          aStream->mPlayedBlocks.RemoveBlock(cacheBlockIndex);
          bo->mClass = READAHEAD_BLOCK;
          
          
          
          
          aStream->mReadaheadBlocks.AddFirstBlock(cacheBlockIndex);
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

  ReentrantMonitorAutoEnter mon(gMediaCache->GetReentrantMonitor());
  mStreamLength = aLength;
}

void
nsMediaCacheStream::NotifyDataStarted(PRInt64 aOffset)
{
  NS_ASSERTION(NS_IsMainThread(), "Only call on main thread");

  ReentrantMonitorAutoEnter mon(gMediaCache->GetReentrantMonitor());
  NS_WARN_IF_FALSE(aOffset == mChannelOffset,
                   "Server is giving us unexpected offset");
  mChannelOffset = aOffset;
  if (mStreamLength >= 0) {
    
    
    mStreamLength = NS_MAX(mStreamLength, mChannelOffset);
  }
}

bool
nsMediaCacheStream::UpdatePrincipal(nsIPrincipal* aPrincipal)
{
  return nsContentUtils::CombineResourcePrincipals(&mPrincipal, aPrincipal);
}

void
nsMediaCacheStream::NotifyDataReceived(PRInt64 aSize, const char* aData,
    nsIPrincipal* aPrincipal)
{
  NS_ASSERTION(NS_IsMainThread(), "Only call on main thread");

  
  
  
  
  
  {
    nsMediaCache::ResourceStreamIterator iter(mResourceID);
    while (nsMediaCacheStream* stream = iter.Next()) {
      if (stream->UpdatePrincipal(aPrincipal)) {
        stream->mClient->CacheClientNotifyPrincipalChanged();
      }
    }
  }

  ReentrantMonitorAutoEnter mon(gMediaCache->GetReentrantMonitor());
  PRInt64 size = aSize;
  const char* data = aData;

  LOG(PR_LOG_DEBUG, ("Stream %p DataReceived at %lld count=%lld",
      this, (long long)mChannelOffset, (long long)aSize));

  
  while (size > 0) {
    PRUint32 blockIndex = mChannelOffset/BLOCK_SIZE;
    PRInt32 blockOffset = PRInt32(mChannelOffset - blockIndex*BLOCK_SIZE);
    PRInt32 chunkSize = NS_MIN<PRInt64>(BLOCK_SIZE - blockOffset, size);

    
    
    const char* blockDataToStore = nsnull;
    ReadMode mode = MODE_PLAYBACK;
    if (blockOffset == 0 && chunkSize == BLOCK_SIZE) {
      
      
      blockDataToStore = data;
    } else {
      if (blockOffset == 0) {
        
        
        mMetadataInPartialBlockBuffer = false;
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
    size -= chunkSize;
    data += chunkSize;
  }

  nsMediaCache::ResourceStreamIterator iter(mResourceID);
  while (nsMediaCacheStream* stream = iter.Next()) {
    if (stream->mStreamLength >= 0) {
      
      stream->mStreamLength = NS_MAX(stream->mStreamLength, mChannelOffset);
    }
    stream->mClient->CacheClientNotifyDataReceived();
  }

  
  
  
  mon.NotifyAll();
}

void
nsMediaCacheStream::NotifyDataEnded(nsresult aStatus)
{
  NS_ASSERTION(NS_IsMainThread(), "Only call on main thread");

  ReentrantMonitorAutoEnter mon(gMediaCache->GetReentrantMonitor());

  if (NS_FAILED(aStatus)) {
    
    
    
    mResourceID = gMediaCache->AllocateResourceID();
  }

  PRInt32 blockOffset = PRInt32(mChannelOffset%BLOCK_SIZE);
  if (blockOffset > 0) {
    
    memset(reinterpret_cast<char*>(mPartialBlockBuffer) + blockOffset, 0,
           BLOCK_SIZE - blockOffset);
    gMediaCache->AllocateAndWriteBlock(this, mPartialBlockBuffer,
        mMetadataInPartialBlockBuffer ? MODE_METADATA : MODE_PLAYBACK);
    
    mon.NotifyAll();
  }

  if (!mDidNotifyDataEnded) {
    nsMediaCache::ResourceStreamIterator iter(mResourceID);
    while (nsMediaCacheStream* stream = iter.Next()) {
      if (NS_SUCCEEDED(aStatus)) {
        
        stream->mStreamLength = mChannelOffset;
      }
      NS_ASSERTION(!stream->mDidNotifyDataEnded, "Stream already ended!");
      stream->mDidNotifyDataEnded = true;
      stream->mNotifyDataEndedStatus = aStatus;
      stream->mClient->CacheClientNotifyDataEnded(aStatus);
    }
  }

  mChannelEnded = true;
  gMediaCache->QueueUpdate();
}

nsMediaCacheStream::~nsMediaCacheStream()
{
  NS_ASSERTION(NS_IsMainThread(), "Only call on main thread");
  NS_ASSERTION(!mPinCount, "Unbalanced Pin");

  if (gMediaCache) {
    NS_ASSERTION(mClosed, "Stream was not closed");
    gMediaCache->ReleaseStream(this);
    nsMediaCache::MaybeShutdown();
  }
}

void
nsMediaCacheStream::SetSeekable(bool aIsSeekable)
{
  ReentrantMonitorAutoEnter mon(gMediaCache->GetReentrantMonitor());
  NS_ASSERTION(mIsSeekable || aIsSeekable ||
               mChannelOffset == 0, "channel offset must be zero when we become non-seekable");
  mIsSeekable = aIsSeekable;
  
  
  gMediaCache->QueueUpdate();
}

bool
nsMediaCacheStream::IsSeekable()
{
  ReentrantMonitorAutoEnter mon(gMediaCache->GetReentrantMonitor());
  return mIsSeekable;
}

bool
nsMediaCacheStream::AreAllStreamsForResourceSuspended(MediaResource** aActiveStream)
{
  ReentrantMonitorAutoEnter mon(gMediaCache->GetReentrantMonitor());
  nsMediaCache::ResourceStreamIterator iter(mResourceID);
  while (nsMediaCacheStream* stream = iter.Next()) {
    if (!stream->mCacheSuspended && !stream->mChannelEnded && !stream->mClosed) {
      if (aActiveStream) {
        *aActiveStream = stream->mClient;
      }
      return false;
    }
  }
  if (aActiveStream) {
    *aActiveStream = nsnull;
  }
  return true;
}

void
nsMediaCacheStream::Close()
{
  NS_ASSERTION(NS_IsMainThread(), "Only call on main thread");

  ReentrantMonitorAutoEnter mon(gMediaCache->GetReentrantMonitor());
  CloseInternal(mon);
  
  
  
  gMediaCache->QueueUpdate();
}

void
nsMediaCacheStream::EnsureCacheUpdate()
{
  if (mHasHadUpdate)
    return;
  gMediaCache->Update();
}

void
nsMediaCacheStream::CloseInternal(ReentrantMonitorAutoEnter& aReentrantMonitor)
{
  NS_ASSERTION(NS_IsMainThread(), "Only call on main thread");

  if (mClosed)
    return;
  mClosed = true;
  gMediaCache->ReleaseStreamBlocks(this);
  
  aReentrantMonitor.NotifyAll();
}

void
nsMediaCacheStream::Pin()
{
  ReentrantMonitorAutoEnter mon(gMediaCache->GetReentrantMonitor());
  ++mPinCount;
  
  
  gMediaCache->QueueUpdate();
}

void
nsMediaCacheStream::Unpin()
{
  ReentrantMonitorAutoEnter mon(gMediaCache->GetReentrantMonitor());
  NS_ASSERTION(mPinCount > 0, "Unbalanced Unpin");
  --mPinCount;
  
  
  gMediaCache->QueueUpdate();
}

PRInt64
nsMediaCacheStream::GetLength()
{
  ReentrantMonitorAutoEnter mon(gMediaCache->GetReentrantMonitor());
  return mStreamLength;
}

PRInt64
nsMediaCacheStream::GetNextCachedData(PRInt64 aOffset)
{
  ReentrantMonitorAutoEnter mon(gMediaCache->GetReentrantMonitor());
  return GetNextCachedDataInternal(aOffset);
}

PRInt64
nsMediaCacheStream::GetCachedDataEnd(PRInt64 aOffset)
{
  ReentrantMonitorAutoEnter mon(gMediaCache->GetReentrantMonitor());
  return GetCachedDataEndInternal(aOffset);
}

bool
nsMediaCacheStream::IsDataCachedToEndOfStream(PRInt64 aOffset)
{
  ReentrantMonitorAutoEnter mon(gMediaCache->GetReentrantMonitor());
  if (mStreamLength < 0)
    return false;
  return GetCachedDataEndInternal(aOffset) >= mStreamLength;
}

PRInt64
nsMediaCacheStream::GetCachedDataEndInternal(PRInt64 aOffset)
{
  gMediaCache->GetReentrantMonitor().AssertCurrentThreadIn();
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
    
    
    result = NS_MIN(result, mStreamLength);
  }
  return NS_MAX(result, aOffset);
}

PRInt64
nsMediaCacheStream::GetNextCachedDataInternal(PRInt64 aOffset)
{
  gMediaCache->GetReentrantMonitor().AssertCurrentThreadIn();
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

  
  bool hasPartialBlock = (mChannelOffset % BLOCK_SIZE) != 0;
  PRUint32 blockIndex = startBlockIndex + 1;
  while (true) {
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
  ReentrantMonitorAutoEnter mon(gMediaCache->GetReentrantMonitor());
  if (aMode == mCurrentMode)
    return;
  mCurrentMode = aMode;
  gMediaCache->QueueUpdate();
}

void
nsMediaCacheStream::SetPlaybackRate(PRUint32 aBytesPerSecond)
{
  NS_ASSERTION(aBytesPerSecond > 0, "Zero playback rate not allowed");
  ReentrantMonitorAutoEnter mon(gMediaCache->GetReentrantMonitor());
  if (aBytesPerSecond == mPlaybackBytesPerSecond)
    return;
  mPlaybackBytesPerSecond = aBytesPerSecond;
  gMediaCache->QueueUpdate();
}

nsresult
nsMediaCacheStream::Seek(PRInt32 aWhence, PRInt64 aOffset)
{
  NS_ASSERTION(!NS_IsMainThread(), "Don't call on main thread");

  ReentrantMonitorAutoEnter mon(gMediaCache->GetReentrantMonitor());
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

  ReentrantMonitorAutoEnter mon(gMediaCache->GetReentrantMonitor());
  return mStreamOffset;
}

nsresult
nsMediaCacheStream::Read(char* aBuffer, PRUint32 aCount, PRUint32* aBytes)
{
  NS_ASSERTION(!NS_IsMainThread(), "Don't call on main thread");

  ReentrantMonitorAutoEnter mon(gMediaCache->GetReentrantMonitor());
  if (mClosed)
    return NS_ERROR_FAILURE;

  PRUint32 count = 0;
  
  while (count < aCount) {
    PRUint32 streamBlock = PRUint32(mStreamOffset/BLOCK_SIZE);
    PRUint32 offsetInStreamBlock =
      PRUint32(mStreamOffset - streamBlock*BLOCK_SIZE);
    PRInt64 size = NS_MIN(aCount - count, BLOCK_SIZE - offsetInStreamBlock);

    if (mStreamLength >= 0) {
      
      PRInt64 bytesRemaining = mStreamLength - mStreamOffset;
      if (bytesRemaining <= 0) {
        
        break;
      }
      size = NS_MIN(size, bytesRemaining);
      
      size = NS_MIN(size, PRInt64(PR_INT32_MAX));
    }

    PRInt32 bytes;
    PRInt32 cacheBlock = streamBlock < mBlocks.Length() ? mBlocks[streamBlock] : -1;
    if (cacheBlock < 0) {
      

      if (count > 0) {
        
        
        break;
      }

      
      
      
      
      nsMediaCacheStream* streamWithPartialBlock = nsnull;
      nsMediaCache::ResourceStreamIterator iter(mResourceID);
      while (nsMediaCacheStream* stream = iter.Next()) {
        if (PRUint32(stream->mChannelOffset/BLOCK_SIZE) == streamBlock &&
            mStreamOffset < stream->mChannelOffset) {
          streamWithPartialBlock = stream;
          break;
        }
      }
      if (streamWithPartialBlock) {
        
        
        
        bytes = NS_MIN<PRInt64>(size, streamWithPartialBlock->mChannelOffset - mStreamOffset);
        memcpy(aBuffer,
          reinterpret_cast<char*>(streamWithPartialBlock->mPartialBlockBuffer) + offsetInStreamBlock, bytes);
        if (mCurrentMode == MODE_METADATA) {
          streamWithPartialBlock->mMetadataInPartialBlockBuffer = true;
        }
        mStreamOffset += bytes;
        count = bytes;
        break;
      }

      
      mon.Wait();
      if (mClosed) {
        
        
        return NS_ERROR_FAILURE;
      }
      continue;
    }

    gMediaCache->NoteBlockUsage(this, cacheBlock, mCurrentMode, TimeStamp::Now());

    PRInt64 offset = cacheBlock*BLOCK_SIZE + offsetInStreamBlock;
    NS_ABORT_IF_FALSE(size >= 0 && size <= PR_INT32_MAX, "Size out of range.");
    nsresult rv = gMediaCache->ReadCacheFile(offset, aBuffer + count, PRInt32(size), &bytes);
    if (NS_FAILED(rv)) {
      if (count == 0)
        return rv;
      
      break;
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
  ReentrantMonitorAutoEnter mon(gMediaCache->GetReentrantMonitor());
  if (mClosed)
    return NS_ERROR_FAILURE;

  
  PRUint32 count = 0;
  PRInt64 streamOffset = aOffset;
  while (count < aCount) {
    PRUint32 streamBlock = PRUint32(streamOffset/BLOCK_SIZE);
    PRUint32 offsetInStreamBlock =
      PRUint32(streamOffset - streamBlock*BLOCK_SIZE);
    PRInt64 size = NS_MIN<PRInt64>(aCount - count, BLOCK_SIZE - offsetInStreamBlock);

    if (mStreamLength >= 0) {
      
      PRInt64 bytesRemaining = mStreamLength - streamOffset;
      if (bytesRemaining <= 0) {
        return NS_ERROR_FAILURE;
      }
      size = NS_MIN(size, bytesRemaining);
      
      size = NS_MIN(size, PRInt64(PR_INT32_MAX));
    }

    PRInt32 bytes;
    PRUint32 channelBlock = PRUint32(mChannelOffset/BLOCK_SIZE);
    PRInt32 cacheBlock = streamBlock < mBlocks.Length() ? mBlocks[streamBlock] : -1;
    if (channelBlock == streamBlock && streamOffset < mChannelOffset) {
      
      
      
      bytes = NS_MIN<PRInt64>(size, mChannelOffset - streamOffset);
      memcpy(aBuffer + count,
        reinterpret_cast<char*>(mPartialBlockBuffer) + offsetInStreamBlock, bytes);
    } else {
      if (cacheBlock < 0) {
        
        return NS_ERROR_FAILURE;
      }
      PRInt64 offset = cacheBlock*BLOCK_SIZE + offsetInStreamBlock;
      NS_ABORT_IF_FALSE(size >= 0 && size <= PR_INT32_MAX, "Size out of range.");
      nsresult rv = gMediaCache->ReadCacheFile(offset, aBuffer + count, PRInt32(size), &bytes);
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

  if (mInitialized)
    return NS_OK;

  InitMediaCache();
  if (!gMediaCache)
    return NS_ERROR_FAILURE;
  gMediaCache->OpenStream(this);
  mInitialized = true;
  return NS_OK;
}

nsresult
nsMediaCacheStream::InitAsClone(nsMediaCacheStream* aOriginal)
{
  if (!aOriginal->IsAvailableForSharing())
    return NS_ERROR_FAILURE;

  if (mInitialized)
    return NS_OK;

  nsresult rv = Init();
  if (NS_FAILED(rv))
    return rv;
  mResourceID = aOriginal->mResourceID;

  
  ReentrantMonitorAutoEnter mon(gMediaCache->GetReentrantMonitor());

  mPrincipal = aOriginal->mPrincipal;
  mStreamLength = aOriginal->mStreamLength;
  mIsSeekable = aOriginal->mIsSeekable;

  
  
  mCacheSuspended = true;
  mChannelEnded = true;

  if (aOriginal->mDidNotifyDataEnded) {
    mNotifyDataEndedStatus = aOriginal->mNotifyDataEndedStatus;
    mDidNotifyDataEnded = true;
    mClient->CacheClientNotifyDataEnded(mNotifyDataEndedStatus);
  }

  for (PRUint32 i = 0; i < aOriginal->mBlocks.Length(); ++i) {
    PRInt32 cacheBlockIndex = aOriginal->mBlocks[i];
    if (cacheBlockIndex < 0)
      continue;

    while (i >= mBlocks.Length()) {
      mBlocks.AppendElement(-1);
    }
    
    
    gMediaCache->AddBlockOwnerAsReadahead(cacheBlockIndex, this, i);
  }

  return NS_OK;
}

nsresult nsMediaCacheStream::GetCachedRanges(nsTArray<MediaByteRange>& aRanges)
{
  
  
  ReentrantMonitorAutoEnter mon(gMediaCache->GetReentrantMonitor());

  
  
  NS_ASSERTION(mPinCount > 0, "Must be pinned");

  PRInt64 startOffset = GetNextCachedData(0);
  while (startOffset >= 0) {
    PRInt64 endOffset = GetCachedDataEnd(startOffset);
    NS_ASSERTION(startOffset < endOffset, "Buffered range must end after its start");
    
    aRanges.AppendElement(MediaByteRange(startOffset, endOffset));
    startOffset = GetNextCachedData(endOffset);
    NS_ASSERTION(startOffset == -1 || startOffset > endOffset,
      "Must have advanced to start of next range, or hit end of stream");
  }
  return NS_OK;
}
