





#include "mozilla/ReentrantMonitor.h"

#include "MediaCache.h"
#include "prio.h"
#include "nsContentUtils.h"
#include "nsThreadUtils.h"
#include "MediaResource.h"
#include "prlog.h"
#include "mozilla/Preferences.h"
#include "FileBlockCache.h"
#include "nsAnonymousTemporaryFile.h"
#include "nsIObserverService.h"
#include "nsISeekableStream.h"
#include "nsIPrincipal.h"
#include "mozilla/Attributes.h"
#include "mozilla/Services.h"
#include <algorithm>

namespace mozilla {

#ifdef PR_LOGGING
PRLogModuleInfo* gMediaCacheLog;
#define CACHE_LOG(type, msg) PR_LOG(gMediaCacheLog, type, msg)
#else
#define CACHE_LOG(type, msg)
#endif





static const double NONSEEKABLE_READAHEAD_MAX = 0.5;











static const uint32_t REPLAY_PENALTY_FACTOR = 3;






static const uint32_t FREE_BLOCK_SCAN_LIMIT = 16;



static const uint32_t CACHE_POWERSAVE_WAKEUP_LOW_THRESHOLD_MS = 10000;

#ifdef DEBUG


#endif




static MediaCache* gMediaCache;

class MediaCacheFlusher MOZ_FINAL : public nsIObserver,
                                      public nsSupportsWeakReference {
  MediaCacheFlusher() {}
  ~MediaCacheFlusher();
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER

  static void Init();
};

static MediaCacheFlusher* gMediaCacheFlusher;

NS_IMPL_ISUPPORTS(MediaCacheFlusher, nsIObserver, nsISupportsWeakReference)

MediaCacheFlusher::~MediaCacheFlusher()
{
  gMediaCacheFlusher = nullptr;
}

void MediaCacheFlusher::Init()
{
  if (gMediaCacheFlusher) {
    return;
  }

  gMediaCacheFlusher = new MediaCacheFlusher();
  NS_ADDREF(gMediaCacheFlusher);

  nsCOMPtr<nsIObserverService> observerService =
    mozilla::services::GetObserverService();
  if (observerService) {
    observerService->AddObserver(gMediaCacheFlusher, "last-pb-context-exited", true);
    observerService->AddObserver(gMediaCacheFlusher, "network-clear-cache-stored-anywhere", true);
  }
}

class MediaCache {
public:
  friend class MediaCacheStream::BlockList;
  typedef MediaCacheStream::BlockList BlockList;
  enum {
    BLOCK_SIZE = MediaCacheStream::BLOCK_SIZE
  };

  MediaCache() : mNextResourceID(1),
    mReentrantMonitor("MediaCache.mReentrantMonitor"),
    mUpdateQueued(false)
#ifdef DEBUG
    , mInUpdate(false)
#endif
  {
    MOZ_COUNT_CTOR(MediaCache);
  }
  ~MediaCache() {
    NS_ASSERTION(mStreams.IsEmpty(), "Stream(s) still open!");
    Truncate();
    NS_ASSERTION(mIndex.Length() == 0, "Blocks leaked?");
    if (mFileCache) {
      mFileCache->Close();
      mFileCache = nullptr;
    }
    MOZ_COUNT_DTOR(MediaCache);
  }

  
  
  
  nsresult Init();
  
  
  
  
  
  static void MaybeShutdown();

  
  static void Flush();
  void FlushInternal();

  
  
  
  nsresult ReadCacheFile(int64_t aOffset, void* aData, int32_t aLength,
                         int32_t* aBytes);
  
  nsresult ReadCacheFileAllBytes(int64_t aOffset, void* aData, int32_t aLength);

  int64_t AllocateResourceID()
  {
    mReentrantMonitor.AssertCurrentThreadIn();
    return mNextResourceID++;
  }

  
  
  
  
  void OpenStream(MediaCacheStream* aStream);
  
  void ReleaseStream(MediaCacheStream* aStream);
  
  void ReleaseStreamBlocks(MediaCacheStream* aStream);
  
  void AllocateAndWriteBlock(MediaCacheStream* aStream, const void* aData,
                             MediaCacheStream::ReadMode aMode);

  
  
  
  
  
  void NoteSeek(MediaCacheStream* aStream, int64_t aOldOffset);
  
  
  
  
  
  
  void NoteBlockUsage(MediaCacheStream* aStream, int32_t aBlockIndex,
                      MediaCacheStream::ReadMode aMode, TimeStamp aNow);
  
  void AddBlockOwnerAsReadahead(int32_t aBlockIndex, MediaCacheStream* aStream,
                                int32_t aStreamBlockIndex);

  
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
    explicit ResourceStreamIterator(int64_t aResourceID) :
      mResourceID(aResourceID), mNext(0) {}
    MediaCacheStream* Next()
    {
      while (mNext < gMediaCache->mStreams.Length()) {
        MediaCacheStream* stream = gMediaCache->mStreams[mNext];
        ++mNext;
        if (stream->GetResourceID() == mResourceID && !stream->IsClosed())
          return stream;
      }
      return nullptr;
    }
  private:
    int64_t  mResourceID;
    uint32_t mNext;
  };

protected:
  
  
  
  int32_t FindBlockForIncomingData(TimeStamp aNow, MediaCacheStream* aStream);
  
  
  
  
  
  
  
  int32_t FindReusableBlock(TimeStamp aNow,
                            MediaCacheStream* aForStream,
                            int32_t aForStreamBlock,
                            int32_t aMaxSearchBlockIndex);
  bool BlockIsReusable(int32_t aBlockIndex);
  
  
  
  
  void AppendMostReusableBlock(BlockList* aBlockList,
                               nsTArray<uint32_t>* aResult,
                               int32_t aBlockIndexLimit);

  enum BlockClass {
    
    
    
    
    METADATA_BLOCK,
    
    
    PLAYED_BLOCK,
    
    
    
    READAHEAD_BLOCK
  };

  struct BlockOwner {
    BlockOwner() : mStream(nullptr), mClass(READAHEAD_BLOCK) {}

    
    MediaCacheStream* mStream;
    
    uint32_t            mStreamBlock;
    
    
    TimeStamp           mLastUseTime;
    BlockClass          mClass;
  };

  struct Block {
    
    nsTArray<BlockOwner> mOwners;
  };

  
  
  BlockList* GetListForBlock(BlockOwner* aBlock);
  
  
  BlockOwner* GetBlockOwner(int32_t aBlockIndex, MediaCacheStream* aStream);
  
  bool IsBlockFree(int32_t aBlockIndex)
  { return mIndex[aBlockIndex].mOwners.IsEmpty(); }
  
  
  void FreeBlock(int32_t aBlock);
  
  
  void RemoveBlockOwner(int32_t aBlockIndex, MediaCacheStream* aStream);
  
  
  void SwapBlocks(int32_t aBlockIndex1, int32_t aBlockIndex2);
  
  
  void InsertReadaheadBlock(BlockOwner* aBlockOwner, int32_t aBlockIndex);

  
  TimeDuration PredictNextUse(TimeStamp aNow, int32_t aBlock);
  
  TimeDuration PredictNextUseForIncomingData(MediaCacheStream* aStream);

  
  
  void Truncate();

  
  
  int64_t                       mNextResourceID;

  
  
  
  ReentrantMonitor         mReentrantMonitor;
  
  
  nsTArray<MediaCacheStream*> mStreams;
  
  nsTArray<Block> mIndex;
  
  nsRefPtr<FileBlockCache> mFileCache;
  
  BlockList       mFreeBlocks;
  
  bool            mUpdateQueued;
#ifdef DEBUG
  bool            mInUpdate;
#endif
};

NS_IMETHODIMP
MediaCacheFlusher::Observe(nsISupports *aSubject, char const *aTopic, char16_t const *aData)
{
  if (strcmp(aTopic, "last-pb-context-exited") == 0) {
    MediaCache::Flush();
  }
  if (strcmp(aTopic, "network-clear-cache-stored-anywhere") == 0) {
    MediaCache::Flush();
  }
  return NS_OK;
}

MediaCacheStream::MediaCacheStream(ChannelMediaResource* aClient)
  : mClient(aClient),
    mInitialized(false),
    mHasHadUpdate(false),
    mClosed(false),
    mDidNotifyDataEnded(false),
    mResourceID(0),
    mIsTransportSeekable(false),
    mCacheSuspended(false),
    mChannelEnded(false),
    mChannelOffset(0),
    mStreamLength(-1),
    mStreamOffset(0),
    mPlaybackBytesPerSecond(10000),
    mPinCount(0),
    mCurrentMode(MODE_PLAYBACK),
    mMetadataInPartialBlockBuffer(false),
    mPartialBlockBuffer(new int64_t[BLOCK_SIZE/sizeof(int64_t)])
{
}

size_t MediaCacheStream::SizeOfExcludingThis(
                                MallocSizeOf aMallocSizeOf) const
{
  
  
  
  size_t size = mBlocks.SizeOfExcludingThis(aMallocSizeOf);
  size += mReadaheadBlocks.SizeOfExcludingThis(aMallocSizeOf);
  size += mMetadataBlocks.SizeOfExcludingThis(aMallocSizeOf);
  size += mPlayedBlocks.SizeOfExcludingThis(aMallocSizeOf);
  size += mPartialBlockBuffer.SizeOfExcludingThis(aMallocSizeOf);

  return size;
}

size_t MediaCacheStream::BlockList::SizeOfExcludingThis(
                                MallocSizeOf aMallocSizeOf) const
{
  return mEntries.SizeOfExcludingThis( nullptr,
                                      aMallocSizeOf);
}

void MediaCacheStream::BlockList::AddFirstBlock(int32_t aBlock)
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

void MediaCacheStream::BlockList::AddAfter(int32_t aBlock, int32_t aBefore)
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

void MediaCacheStream::BlockList::RemoveBlock(int32_t aBlock)
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

int32_t MediaCacheStream::BlockList::GetLastBlock() const
{
  if (mFirstBlock < 0)
    return -1;
  return mEntries.GetEntry(mFirstBlock)->mPrevBlock;
}

int32_t MediaCacheStream::BlockList::GetNextBlock(int32_t aBlock) const
{
  int32_t block = mEntries.GetEntry(aBlock)->mNextBlock;
  if (block == mFirstBlock)
    return -1;
  return block;
}

int32_t MediaCacheStream::BlockList::GetPrevBlock(int32_t aBlock) const
{
  if (aBlock == mFirstBlock)
    return -1;
  return mEntries.GetEntry(aBlock)->mPrevBlock;
}

#ifdef DEBUG
void MediaCacheStream::BlockList::Verify()
{
  int32_t count = 0;
  if (mFirstBlock >= 0) {
    int32_t block = mFirstBlock;
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

static void UpdateSwappedBlockIndex(int32_t* aBlockIndex,
    int32_t aBlock1Index, int32_t aBlock2Index)
{
  int32_t index = *aBlockIndex;
  if (index == aBlock1Index) {
    *aBlockIndex = aBlock2Index;
  } else if (index == aBlock2Index) {
    *aBlockIndex = aBlock1Index;
  }
}

void
MediaCacheStream::BlockList::NotifyBlockSwapped(int32_t aBlockIndex1,
                                                  int32_t aBlockIndex2)
{
  Entry* e1 = mEntries.GetEntry(aBlockIndex1);
  Entry* e2 = mEntries.GetEntry(aBlockIndex2);
  int32_t e1Prev = -1, e1Next = -1, e2Prev = -1, e2Next = -1;

  
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
MediaCache::Init()
{
  NS_ASSERTION(NS_IsMainThread(), "Only call on main thread");
  NS_ASSERTION(!mFileCache, "Cache file already open?");

  PRFileDesc* fileDesc = nullptr;
  nsresult rv = NS_OpenAnonymousTemporaryFile(&fileDesc);
  NS_ENSURE_SUCCESS(rv,rv);

  mFileCache = new FileBlockCache();
  rv = mFileCache->Open(fileDesc);
  NS_ENSURE_SUCCESS(rv,rv);

#ifdef PR_LOGGING
  if (!gMediaCacheLog) {
    gMediaCacheLog = PR_NewLogModule("MediaCache");
  }
#endif

  MediaCacheFlusher::Init();

  return NS_OK;
}

void
MediaCache::Flush()
{
  NS_ASSERTION(NS_IsMainThread(), "Only call on main thread");

  if (!gMediaCache)
    return;

  gMediaCache->FlushInternal();
}

void
MediaCache::FlushInternal()
{
  ReentrantMonitorAutoEnter mon(mReentrantMonitor);

  for (uint32_t blockIndex = 0; blockIndex < mIndex.Length(); ++blockIndex) {
    FreeBlock(blockIndex);
  }

  
  Truncate();
  NS_ASSERTION(mIndex.Length() == 0, "Blocks leaked?");
  if (mFileCache) {
    mFileCache->Close();
    mFileCache = nullptr;
  }
  Init();
}

void
MediaCache::MaybeShutdown()
{
  NS_ASSERTION(NS_IsMainThread(),
               "MediaCache::MaybeShutdown called on non-main thread");
  if (!gMediaCache->mStreams.IsEmpty()) {
    
    return;
  }

  
  
  
  delete gMediaCache;
  gMediaCache = nullptr;
  NS_IF_RELEASE(gMediaCacheFlusher);
}

static void
InitMediaCache()
{
  if (gMediaCache)
    return;

  gMediaCache = new MediaCache();
  if (!gMediaCache)
    return;

  nsresult rv = gMediaCache->Init();
  if (NS_FAILED(rv)) {
    delete gMediaCache;
    gMediaCache = nullptr;
  }
}

nsresult
MediaCache::ReadCacheFile(int64_t aOffset, void* aData, int32_t aLength,
                            int32_t* aBytes)
{
  mReentrantMonitor.AssertCurrentThreadIn();

  if (!mFileCache)
    return NS_ERROR_FAILURE;

  return mFileCache->Read(aOffset, reinterpret_cast<uint8_t*>(aData), aLength, aBytes);
}

nsresult
MediaCache::ReadCacheFileAllBytes(int64_t aOffset, void* aData, int32_t aLength)
{
  mReentrantMonitor.AssertCurrentThreadIn();

  int64_t offset = aOffset;
  int32_t count = aLength;
  
  char* data = static_cast<char*>(aData);
  while (count > 0) {
    int32_t bytes;
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

static int32_t GetMaxBlocks()
{
  
  
  
  int32_t cacheSize = Preferences::GetInt("media.cache_size", 500*1024);
  int64_t maxBlocks = static_cast<int64_t>(cacheSize)*1024/MediaCache::BLOCK_SIZE;
  maxBlocks = std::max<int64_t>(maxBlocks, 1);
  return int32_t(std::min<int64_t>(maxBlocks, INT32_MAX));
}

int32_t
MediaCache::FindBlockForIncomingData(TimeStamp aNow,
                                       MediaCacheStream* aStream)
{
  mReentrantMonitor.AssertCurrentThreadIn();

  int32_t blockIndex = FindReusableBlock(aNow, aStream,
      aStream->mChannelOffset/BLOCK_SIZE, INT32_MAX);

  if (blockIndex < 0 || !IsBlockFree(blockIndex)) {
    
    
    
    
    
    if ((mIndex.Length() < uint32_t(GetMaxBlocks()) || blockIndex < 0 ||
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
MediaCache::BlockIsReusable(int32_t aBlockIndex)
{
  Block* block = &mIndex[aBlockIndex];
  for (uint32_t i = 0; i < block->mOwners.Length(); ++i) {
    MediaCacheStream* stream = block->mOwners[i].mStream;
    if (stream->mPinCount > 0 ||
        stream->mStreamOffset/BLOCK_SIZE == block->mOwners[i].mStreamBlock) {
      return false;
    }
  }
  return true;
}

void
MediaCache::AppendMostReusableBlock(BlockList* aBlockList,
                                      nsTArray<uint32_t>* aResult,
                                      int32_t aBlockIndexLimit)
{
  mReentrantMonitor.AssertCurrentThreadIn();

  int32_t blockIndex = aBlockList->GetLastBlock();
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

int32_t
MediaCache::FindReusableBlock(TimeStamp aNow,
                                MediaCacheStream* aForStream,
                                int32_t aForStreamBlock,
                                int32_t aMaxSearchBlockIndex)
{
  mReentrantMonitor.AssertCurrentThreadIn();

  uint32_t length = std::min(uint32_t(aMaxSearchBlockIndex), uint32_t(mIndex.Length()));

  if (aForStream && aForStreamBlock > 0 &&
      uint32_t(aForStreamBlock) <= aForStream->mBlocks.Length()) {
    int32_t prevCacheBlock = aForStream->mBlocks[aForStreamBlock - 1];
    if (prevCacheBlock >= 0) {
      uint32_t freeBlockScanEnd =
        std::min(length, prevCacheBlock + FREE_BLOCK_SCAN_LIMIT);
      for (uint32_t i = prevCacheBlock; i < freeBlockScanEnd; ++i) {
        if (IsBlockFree(i))
          return i;
      }
    }
  }

  if (!mFreeBlocks.IsEmpty()) {
    int32_t blockIndex = mFreeBlocks.GetFirstBlock();
    do {
      if (blockIndex < aMaxSearchBlockIndex)
        return blockIndex;
      blockIndex = mFreeBlocks.GetNextBlock(blockIndex);
    } while (blockIndex >= 0);
  }

  
  
  
  
  nsAutoTArray<uint32_t,8> candidates;
  for (uint32_t i = 0; i < mStreams.Length(); ++i) {
    MediaCacheStream* stream = mStreams[i];
    if (stream->mPinCount > 0) {
      
      continue;
    }

    AppendMostReusableBlock(&stream->mMetadataBlocks, &candidates, length);
    AppendMostReusableBlock(&stream->mPlayedBlocks, &candidates, length);

    
    
    if (stream->mIsTransportSeekable) {
      AppendMostReusableBlock(&stream->mReadaheadBlocks, &candidates, length);
    }
  }

  TimeDuration latestUse;
  int32_t latestUseBlock = -1;
  for (uint32_t i = 0; i < candidates.Length(); ++i) {
    TimeDuration nextUse = PredictNextUse(aNow, candidates[i]);
    if (nextUse > latestUse) {
      latestUse = nextUse;
      latestUseBlock = candidates[i];
    }
  }

  return latestUseBlock;
}

MediaCache::BlockList*
MediaCache::GetListForBlock(BlockOwner* aBlock)
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
    return nullptr;
  }
}

MediaCache::BlockOwner*
MediaCache::GetBlockOwner(int32_t aBlockIndex, MediaCacheStream* aStream)
{
  Block* block = &mIndex[aBlockIndex];
  for (uint32_t i = 0; i < block->mOwners.Length(); ++i) {
    if (block->mOwners[i].mStream == aStream)
      return &block->mOwners[i];
  }
  return nullptr;
}

void
MediaCache::SwapBlocks(int32_t aBlockIndex1, int32_t aBlockIndex2)
{
  mReentrantMonitor.AssertCurrentThreadIn();

  Block* block1 = &mIndex[aBlockIndex1];
  Block* block2 = &mIndex[aBlockIndex2];

  block1->mOwners.SwapElements(block2->mOwners);

  
  
  
  const Block* blocks[] = { block1, block2 };
  int32_t blockIndices[] = { aBlockIndex1, aBlockIndex2 };
  for (int32_t i = 0; i < 2; ++i) {
    for (uint32_t j = 0; j < blocks[i]->mOwners.Length(); ++j) {
      const BlockOwner* b = &blocks[i]->mOwners[j];
      b->mStream->mBlocks[b->mStreamBlock] = blockIndices[i];
    }
  }

  
  mFreeBlocks.NotifyBlockSwapped(aBlockIndex1, aBlockIndex2);

  nsTHashtable<nsPtrHashKey<MediaCacheStream> > visitedStreams;

  for (int32_t i = 0; i < 2; ++i) {
    for (uint32_t j = 0; j < blocks[i]->mOwners.Length(); ++j) {
      MediaCacheStream* stream = blocks[i]->mOwners[j].mStream;
      
      
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
MediaCache::RemoveBlockOwner(int32_t aBlockIndex, MediaCacheStream* aStream)
{
  Block* block = &mIndex[aBlockIndex];
  for (uint32_t i = 0; i < block->mOwners.Length(); ++i) {
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
MediaCache::AddBlockOwnerAsReadahead(int32_t aBlockIndex,
                                       MediaCacheStream* aStream,
                                       int32_t aStreamBlockIndex)
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
MediaCache::FreeBlock(int32_t aBlock)
{
  mReentrantMonitor.AssertCurrentThreadIn();

  Block* block = &mIndex[aBlock];
  if (block->mOwners.IsEmpty()) {
    
    return;
  }

  CACHE_LOG(PR_LOG_DEBUG, ("Released block %d", aBlock));

  for (uint32_t i = 0; i < block->mOwners.Length(); ++i) {
    BlockOwner* bo = &block->mOwners[i];
    GetListForBlock(bo)->RemoveBlock(aBlock);
    bo->mStream->mBlocks[bo->mStreamBlock] = -1;
  }
  block->mOwners.Clear();
  mFreeBlocks.AddFirstBlock(aBlock);
  Verify();
}

TimeDuration
MediaCache::PredictNextUse(TimeStamp aNow, int32_t aBlock)
{
  mReentrantMonitor.AssertCurrentThreadIn();
  NS_ASSERTION(!IsBlockFree(aBlock), "aBlock is free");

  Block* block = &mIndex[aBlock];
  
  
  TimeDuration result;
  for (uint32_t i = 0; i < block->mOwners.Length(); ++i) {
    BlockOwner* bo = &block->mOwners[i];
    TimeDuration prediction;
    switch (bo->mClass) {
    case METADATA_BLOCK:
      
      
      prediction = aNow - bo->mLastUseTime;
      break;
    case PLAYED_BLOCK: {
      
      
      NS_ASSERTION(static_cast<int64_t>(bo->mStreamBlock)*BLOCK_SIZE <
                   bo->mStream->mStreamOffset,
                   "Played block after the current stream position?");
      int64_t bytesBehind =
        bo->mStream->mStreamOffset - static_cast<int64_t>(bo->mStreamBlock)*BLOCK_SIZE;
      int64_t millisecondsBehind =
        bytesBehind*1000/bo->mStream->mPlaybackBytesPerSecond;
      prediction = TimeDuration::FromMilliseconds(
          std::min<int64_t>(millisecondsBehind*REPLAY_PENALTY_FACTOR, INT32_MAX));
      break;
    }
    case READAHEAD_BLOCK: {
      int64_t bytesAhead =
        static_cast<int64_t>(bo->mStreamBlock)*BLOCK_SIZE - bo->mStream->mStreamOffset;
      NS_ASSERTION(bytesAhead >= 0,
                   "Readahead block before the current stream position?");
      int64_t millisecondsAhead =
        bytesAhead*1000/bo->mStream->mPlaybackBytesPerSecond;
      prediction = TimeDuration::FromMilliseconds(
          std::min<int64_t>(millisecondsAhead, INT32_MAX));
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
MediaCache::PredictNextUseForIncomingData(MediaCacheStream* aStream)
{
  mReentrantMonitor.AssertCurrentThreadIn();

  int64_t bytesAhead = aStream->mChannelOffset - aStream->mStreamOffset;
  if (bytesAhead <= -BLOCK_SIZE) {
    
    return TimeDuration::FromSeconds(24*60*60);
  }
  if (bytesAhead <= 0)
    return TimeDuration(0);
  int64_t millisecondsAhead = bytesAhead*1000/aStream->mPlaybackBytesPerSecond;
  return TimeDuration::FromMilliseconds(
      std::min<int64_t>(millisecondsAhead, INT32_MAX));
}

enum StreamAction { NONE, SEEK, SEEK_AND_RESUME, RESUME, SUSPEND };

void
MediaCache::Update()
{
  NS_ASSERTION(NS_IsMainThread(), "Only call on main thread");

  
  
  
  
  nsAutoTArray<StreamAction,10> actions;

  {
    ReentrantMonitorAutoEnter mon(mReentrantMonitor);
    mUpdateQueued = false;
#ifdef DEBUG
    mInUpdate = true;
#endif

    int32_t maxBlocks = GetMaxBlocks();
    TimeStamp now = TimeStamp::Now();

    int32_t freeBlockCount = mFreeBlocks.GetCount();
    TimeDuration latestPredictedUseForOverflow = 0;
    if (mIndex.Length() > uint32_t(maxBlocks)) {
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      for (int32_t blockIndex = mIndex.Length() - 1; blockIndex >= maxBlocks;
           --blockIndex) {
        if (IsBlockFree(blockIndex)) {
          
          --freeBlockCount;
          continue;
        }
        TimeDuration predictedUse = PredictNextUse(now, blockIndex);
        latestPredictedUseForOverflow = std::max(latestPredictedUseForOverflow, predictedUse);
      }
    } else {
      freeBlockCount += maxBlocks - mIndex.Length();
    }

    
    for (int32_t blockIndex = mIndex.Length() - 1; blockIndex >= maxBlocks;
         --blockIndex) {
      if (IsBlockFree(blockIndex))
        continue;

      Block* block = &mIndex[blockIndex];
      
      
      
      int32_t destinationBlockIndex =
        FindReusableBlock(now, block->mOwners[0].mStream,
                          block->mOwners[0].mStreamBlock, maxBlocks);
      if (destinationBlockIndex < 0) {
        
        
        break;
      }

      if (IsBlockFree(destinationBlockIndex) ||
          PredictNextUse(now, destinationBlockIndex) > latestPredictedUseForOverflow) {
        
        

        nsresult rv = mFileCache->MoveBlock(blockIndex, destinationBlockIndex);

        if (NS_SUCCEEDED(rv)) {
          
          CACHE_LOG(PR_LOG_DEBUG, ("Swapping blocks %d and %d (trimming cache)",
                    blockIndex, destinationBlockIndex));
          
          
          SwapBlocks(blockIndex, destinationBlockIndex);
          
          CACHE_LOG(PR_LOG_DEBUG, ("Released block %d (trimming cache)", blockIndex));
          FreeBlock(blockIndex);
        }
      } else {
        CACHE_LOG(PR_LOG_DEBUG, ("Could not trim cache block %d (destination %d, predicted next use %f, latest predicted use for overflow %f",
                                 blockIndex, destinationBlockIndex,
                                 PredictNextUse(now, destinationBlockIndex).ToSeconds(),
                                 latestPredictedUseForOverflow.ToSeconds()));
      }
    }
    
    Truncate();

    
    
    
    int32_t nonSeekableReadaheadBlockCount = 0;
    for (uint32_t i = 0; i < mStreams.Length(); ++i) {
      MediaCacheStream* stream = mStreams[i];
      if (!stream->mIsTransportSeekable) {
        nonSeekableReadaheadBlockCount += stream->mReadaheadBlocks.GetCount();
      }
    }

    
    
    TimeDuration latestNextUse;
    if (freeBlockCount == 0) {
      int32_t reusableBlock = FindReusableBlock(now, nullptr, 0, maxBlocks);
      if (reusableBlock >= 0) {
        latestNextUse = PredictNextUse(now, reusableBlock);
      }
    }

    for (uint32_t i = 0; i < mStreams.Length(); ++i) {
      actions.AppendElement(NONE);

      MediaCacheStream* stream = mStreams[i];
      if (stream->mClosed)
        continue;

      
      
      int64_t dataOffset = stream->GetCachedDataEndInternal(stream->mStreamOffset);

      
      int64_t desiredOffset = dataOffset;
      if (stream->mIsTransportSeekable) {
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
        
        
        
        
        
        
        
        
        
        CACHE_LOG(PR_LOG_DEBUG, ("Stream %p at end of stream", stream));
        enableReading = !stream->mCacheSuspended &&
          stream->mStreamLength == stream->mChannelOffset;
      } else if (desiredOffset < stream->mStreamOffset) {
        
        
        CACHE_LOG(PR_LOG_DEBUG, ("Stream %p catching up", stream));
        enableReading = true;
      } else if (desiredOffset < stream->mStreamOffset + BLOCK_SIZE) {
        
        CACHE_LOG(PR_LOG_DEBUG, ("Stream %p feeding reader", stream));
        enableReading = true;
      } else if (!stream->mIsTransportSeekable &&
                 nonSeekableReadaheadBlockCount >= maxBlocks*NONSEEKABLE_READAHEAD_MAX) {
        
        
        
        CACHE_LOG(PR_LOG_DEBUG, ("Stream %p throttling non-seekable readahead", stream));
        enableReading = false;
      } else if (mIndex.Length() > uint32_t(maxBlocks)) {
        
        
        CACHE_LOG(PR_LOG_DEBUG, ("Stream %p throttling to reduce cache size", stream));
        enableReading = false;
      } else {
        TimeDuration predictedNewDataUse = PredictNextUseForIncomingData(stream);

        if (stream->mCacheSuspended &&
            predictedNewDataUse.ToMilliseconds() > CACHE_POWERSAVE_WAKEUP_LOW_THRESHOLD_MS) {
          
          CACHE_LOG(PR_LOG_DEBUG, ("Stream %p avoiding wakeup since more data is not needed", stream));
          enableReading = false;
        } else if (freeBlockCount > 0) {
          
          CACHE_LOG(PR_LOG_DEBUG, ("Stream %p reading since there are free blocks", stream));
          enableReading = true;
        } else if (latestNextUse <= TimeDuration(0)) {
          
          CACHE_LOG(PR_LOG_DEBUG, ("Stream %p throttling due to no reusable blocks", stream));
          enableReading = false;
        } else {
          
          
          CACHE_LOG(PR_LOG_DEBUG, ("Stream %p predict next data in %f, current worst block is %f",
                    stream, predictedNewDataUse.ToSeconds(), latestNextUse.ToSeconds()));
          enableReading = predictedNewDataUse < latestNextUse;
        }
      }

      if (enableReading) {
        for (uint32_t j = 0; j < i; ++j) {
          MediaCacheStream* other = mStreams[j];
          if (other->mResourceID == stream->mResourceID &&
              !other->mClient->IsSuspended() &&
              other->mChannelOffset/BLOCK_SIZE == desiredOffset/BLOCK_SIZE) {
            
            
            enableReading = false;
            CACHE_LOG(PR_LOG_DEBUG, ("Stream %p waiting on same block (%lld) from stream %p",
                                     stream, desiredOffset/BLOCK_SIZE, other));
            break;
          }
        }
      }

      if (stream->mChannelOffset != desiredOffset && enableReading) {
        
        NS_ASSERTION(stream->mIsTransportSeekable || desiredOffset == 0,
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

  
  
  
  
  
  
  

  
  
  
  for (uint32_t i = 0; i < mStreams.Length(); ++i) {
    MediaCacheStream* stream = mStreams[i];
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

  for (uint32_t i = 0; i < mStreams.Length(); ++i) {
    MediaCacheStream* stream = mStreams[i];
    nsresult rv;
    switch (actions[i]) {
    case SEEK:
	case SEEK_AND_RESUME:
      CACHE_LOG(PR_LOG_DEBUG, ("Stream %p CacheSeek to %lld (resume=%d)", stream,
                (long long)stream->mChannelOffset, actions[i] == SEEK_AND_RESUME));
      rv = stream->mClient->CacheClientSeek(stream->mChannelOffset,
                                            actions[i] == SEEK_AND_RESUME);
      break;
    case RESUME:
      CACHE_LOG(PR_LOG_DEBUG, ("Stream %p Resumed", stream));
      rv = stream->mClient->CacheClientResume();
      break;
    case SUSPEND:
      CACHE_LOG(PR_LOG_DEBUG, ("Stream %p Suspended", stream));
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
MediaCache::QueueUpdate()
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
MediaCache::Verify()
{
  mReentrantMonitor.AssertCurrentThreadIn();

  mFreeBlocks.Verify();
  for (uint32_t i = 0; i < mStreams.Length(); ++i) {
    MediaCacheStream* stream = mStreams[i];
    stream->mReadaheadBlocks.Verify();
    stream->mPlayedBlocks.Verify();
    stream->mMetadataBlocks.Verify();

    
    int32_t block = stream->mReadaheadBlocks.GetFirstBlock();
    int32_t lastStreamBlock = -1;
    while (block >= 0) {
      uint32_t j = 0;
      while (mIndex[block].mOwners[j].mStream != stream) {
        ++j;
      }
      int32_t nextStreamBlock =
        int32_t(mIndex[block].mOwners[j].mStreamBlock);
      NS_ASSERTION(lastStreamBlock < nextStreamBlock,
                   "Blocks not increasing in readahead stream");
      lastStreamBlock = nextStreamBlock;
      block = stream->mReadaheadBlocks.GetNextBlock(block);
    }
  }
}
#endif

void
MediaCache::InsertReadaheadBlock(BlockOwner* aBlockOwner,
                                   int32_t aBlockIndex)
{
  mReentrantMonitor.AssertCurrentThreadIn();

  
  
  MediaCacheStream* stream = aBlockOwner->mStream;
  int32_t readaheadIndex = stream->mReadaheadBlocks.GetLastBlock();
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
MediaCache::AllocateAndWriteBlock(MediaCacheStream* aStream, const void* aData,
                                    MediaCacheStream::ReadMode aMode)
{
  mReentrantMonitor.AssertCurrentThreadIn();

  int32_t streamBlockIndex = aStream->mChannelOffset/BLOCK_SIZE;

  
  ResourceStreamIterator iter(aStream->mResourceID);
  while (MediaCacheStream* stream = iter.Next()) {
    while (streamBlockIndex >= int32_t(stream->mBlocks.Length())) {
      stream->mBlocks.AppendElement(-1);
    }
    if (stream->mBlocks[streamBlockIndex] >= 0) {
      
      int32_t globalBlockIndex = stream->mBlocks[streamBlockIndex];
      CACHE_LOG(PR_LOG_DEBUG, ("Released block %d from stream %p block %d(%lld)",
                globalBlockIndex, stream, streamBlockIndex, (long long)streamBlockIndex*BLOCK_SIZE));
      RemoveBlockOwner(globalBlockIndex, stream);
    }
  }

  

  TimeStamp now = TimeStamp::Now();
  int32_t blockIndex = FindBlockForIncomingData(now, aStream);
  if (blockIndex >= 0) {
    FreeBlock(blockIndex);

    Block* block = &mIndex[blockIndex];
    CACHE_LOG(PR_LOG_DEBUG, ("Allocated block %d to stream %p block %d(%lld)",
              blockIndex, aStream, streamBlockIndex, (long long)streamBlockIndex*BLOCK_SIZE));

    mFreeBlocks.RemoveBlock(blockIndex);

    
    ResourceStreamIterator iter(aStream->mResourceID);
    while (MediaCacheStream* stream = iter.Next()) {
      BlockOwner* bo = block->mOwners.AppendElement();
      if (!bo)
        return;

      bo->mStream = stream;
      bo->mStreamBlock = streamBlockIndex;
      bo->mLastUseTime = now;
      stream->mBlocks[streamBlockIndex] = blockIndex;
      if (streamBlockIndex*BLOCK_SIZE < stream->mStreamOffset) {
        bo->mClass = aMode == MediaCacheStream::MODE_PLAYBACK
          ? PLAYED_BLOCK : METADATA_BLOCK;
        
        
        
        GetListForBlock(bo)->AddFirstBlock(blockIndex);
        Verify();
      } else {
        
        
        
        bo->mClass = READAHEAD_BLOCK;
        InsertReadaheadBlock(bo, blockIndex);
      }
    }

    nsresult rv = mFileCache->WriteBlock(blockIndex, reinterpret_cast<const uint8_t*>(aData));
    if (NS_FAILED(rv)) {
      CACHE_LOG(PR_LOG_DEBUG, ("Released block %d from stream %p block %d(%lld)",
                blockIndex, aStream, streamBlockIndex, (long long)streamBlockIndex*BLOCK_SIZE));
      FreeBlock(blockIndex);
    }
  }

  
  
  QueueUpdate();
}

void
MediaCache::OpenStream(MediaCacheStream* aStream)
{
  NS_ASSERTION(NS_IsMainThread(), "Only call on main thread");

  ReentrantMonitorAutoEnter mon(mReentrantMonitor);
  CACHE_LOG(PR_LOG_DEBUG, ("Stream %p opened", aStream));
  mStreams.AppendElement(aStream);
  aStream->mResourceID = AllocateResourceID();

  
  gMediaCache->QueueUpdate();
}

void
MediaCache::ReleaseStream(MediaCacheStream* aStream)
{
  NS_ASSERTION(NS_IsMainThread(), "Only call on main thread");

  ReentrantMonitorAutoEnter mon(mReentrantMonitor);
  CACHE_LOG(PR_LOG_DEBUG, ("Stream %p closed", aStream));
  mStreams.RemoveElement(aStream);

  
  
  
  gMediaCache->QueueUpdate();
}

void
MediaCache::ReleaseStreamBlocks(MediaCacheStream* aStream)
{
  mReentrantMonitor.AssertCurrentThreadIn();

  
  
  
  uint32_t length = aStream->mBlocks.Length();
  for (uint32_t i = 0; i < length; ++i) {
    int32_t blockIndex = aStream->mBlocks[i];
    if (blockIndex >= 0) {
      CACHE_LOG(PR_LOG_DEBUG, ("Released block %d from stream %p block %d(%lld)",
                blockIndex, aStream, i, (long long)i*BLOCK_SIZE));
      RemoveBlockOwner(blockIndex, aStream);
    }
  }
}

void
MediaCache::Truncate()
{
  uint32_t end;
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
MediaCache::NoteBlockUsage(MediaCacheStream* aStream, int32_t aBlockIndex,
                             MediaCacheStream::ReadMode aMode,
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
    (aMode == MediaCacheStream::MODE_METADATA || bo->mClass == METADATA_BLOCK)
    ? METADATA_BLOCK : PLAYED_BLOCK;
  
  
  GetListForBlock(bo)->AddFirstBlock(aBlockIndex);
  bo->mLastUseTime = aNow;
  Verify();
}

void
MediaCache::NoteSeek(MediaCacheStream* aStream, int64_t aOldOffset)
{
  mReentrantMonitor.AssertCurrentThreadIn();

  if (aOldOffset < aStream->mStreamOffset) {
    
    
    
    int32_t blockIndex = aOldOffset/BLOCK_SIZE;
    int32_t endIndex =
      std::min<int64_t>((aStream->mStreamOffset + BLOCK_SIZE - 1)/BLOCK_SIZE,
             aStream->mBlocks.Length());
    TimeStamp now = TimeStamp::Now();
    while (blockIndex < endIndex) {
      int32_t cacheBlockIndex = aStream->mBlocks[blockIndex];
      if (cacheBlockIndex >= 0) {
        
        
        NoteBlockUsage(aStream, cacheBlockIndex, MediaCacheStream::MODE_PLAYBACK,
                       now);
      }
      ++blockIndex;
    }
  } else {
    
    
    
    int32_t blockIndex =
      (aStream->mStreamOffset + BLOCK_SIZE - 1)/BLOCK_SIZE;
    int32_t endIndex =
      std::min<int64_t>((aOldOffset + BLOCK_SIZE - 1)/BLOCK_SIZE,
             aStream->mBlocks.Length());
    while (blockIndex < endIndex) {
      int32_t cacheBlockIndex = aStream->mBlocks[endIndex - 1];
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
MediaCacheStream::NotifyDataLength(int64_t aLength)
{
  NS_ASSERTION(NS_IsMainThread(), "Only call on main thread");

  ReentrantMonitorAutoEnter mon(gMediaCache->GetReentrantMonitor());
  mStreamLength = aLength;
}

void
MediaCacheStream::NotifyDataStarted(int64_t aOffset)
{
  NS_ASSERTION(NS_IsMainThread(), "Only call on main thread");

  ReentrantMonitorAutoEnter mon(gMediaCache->GetReentrantMonitor());
  NS_WARN_IF_FALSE(aOffset == mChannelOffset,
                   "Server is giving us unexpected offset");
  mChannelOffset = aOffset;
  if (mStreamLength >= 0) {
    
    
    mStreamLength = std::max(mStreamLength, mChannelOffset);
  }
}

bool
MediaCacheStream::UpdatePrincipal(nsIPrincipal* aPrincipal)
{
  return nsContentUtils::CombineResourcePrincipals(&mPrincipal, aPrincipal);
}

void
MediaCacheStream::NotifyDataReceived(int64_t aSize, const char* aData,
    nsIPrincipal* aPrincipal)
{
  NS_ASSERTION(NS_IsMainThread(), "Only call on main thread");

  
  
  
  
  
  {
    MediaCache::ResourceStreamIterator iter(mResourceID);
    while (MediaCacheStream* stream = iter.Next()) {
      if (stream->UpdatePrincipal(aPrincipal)) {
        stream->mClient->CacheClientNotifyPrincipalChanged();
      }
    }
  }

  ReentrantMonitorAutoEnter mon(gMediaCache->GetReentrantMonitor());
  int64_t size = aSize;
  const char* data = aData;

  CACHE_LOG(PR_LOG_DEBUG, ("Stream %p DataReceived at %lld count=%lld",
            this, (long long)mChannelOffset, (long long)aSize));

  
  while (size > 0) {
    uint32_t blockIndex = mChannelOffset/BLOCK_SIZE;
    int32_t blockOffset = int32_t(mChannelOffset - blockIndex*BLOCK_SIZE);
    int32_t chunkSize = std::min<int64_t>(BLOCK_SIZE - blockOffset, size);

    
    
    const char* blockDataToStore = nullptr;
    ReadMode mode = MODE_PLAYBACK;
    if (blockOffset == 0 && chunkSize == BLOCK_SIZE) {
      
      
      blockDataToStore = data;
    } else {
      if (blockOffset == 0) {
        
        
        mMetadataInPartialBlockBuffer = false;
      }
      memcpy(reinterpret_cast<char*>(mPartialBlockBuffer.get()) + blockOffset,
             data, chunkSize);

      if (blockOffset + chunkSize == BLOCK_SIZE) {
        
        blockDataToStore = reinterpret_cast<char*>(mPartialBlockBuffer.get());
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

  MediaCache::ResourceStreamIterator iter(mResourceID);
  while (MediaCacheStream* stream = iter.Next()) {
    if (stream->mStreamLength >= 0) {
      
      stream->mStreamLength = std::max(stream->mStreamLength, mChannelOffset);
    }
    stream->mClient->CacheClientNotifyDataReceived();
  }

  
  
  
  mon.NotifyAll();
}

void
MediaCacheStream::FlushPartialBlockInternal(bool aNotifyAll)
{
  NS_ASSERTION(NS_IsMainThread(), "Only call on main thread");

  ReentrantMonitorAutoEnter mon(gMediaCache->GetReentrantMonitor());

  int32_t blockOffset = int32_t(mChannelOffset%BLOCK_SIZE);
  if (blockOffset > 0) {
    CACHE_LOG(PR_LOG_DEBUG,
              ("Stream %p writing partial block: [%d] bytes; "
               "mStreamOffset [%lld] mChannelOffset[%lld] mStreamLength [%lld] "
               "notifying: [%s]",
               this, blockOffset, mStreamOffset, mChannelOffset, mStreamLength,
               aNotifyAll ? "yes" : "no"));

    
    memset(reinterpret_cast<char*>(mPartialBlockBuffer.get()) + blockOffset, 0,
           BLOCK_SIZE - blockOffset);
    gMediaCache->AllocateAndWriteBlock(this, mPartialBlockBuffer,
        mMetadataInPartialBlockBuffer ? MODE_METADATA : MODE_PLAYBACK);
    if (aNotifyAll) {
      
      mon.NotifyAll();
    }
  }
}

void
MediaCacheStream::FlushPartialBlock()
{
  NS_ASSERTION(NS_IsMainThread(), "Only call on main thread");

  ReentrantMonitorAutoEnter mon(gMediaCache->GetReentrantMonitor());

  
  
  
  
  FlushPartialBlockInternal(false);

  gMediaCache->QueueUpdate();
}

void
MediaCacheStream::NotifyDataEnded(nsresult aStatus)
{
  NS_ASSERTION(NS_IsMainThread(), "Only call on main thread");

  ReentrantMonitorAutoEnter mon(gMediaCache->GetReentrantMonitor());

  if (NS_FAILED(aStatus)) {
    
    
    
    mResourceID = gMediaCache->AllocateResourceID();
  }

  FlushPartialBlockInternal(true);

  MediaCache::ResourceStreamIterator iter(mResourceID);
  while (MediaCacheStream* stream = iter.Next()) {
    if (NS_SUCCEEDED(aStatus)) {
      
      stream->mStreamLength = mChannelOffset;
    }
    if (!stream->mDidNotifyDataEnded) {
      stream->mDidNotifyDataEnded = true;
      stream->mNotifyDataEndedStatus = aStatus;
      stream->mClient->CacheClientNotifyDataEnded(aStatus);
    }
  }

  mChannelEnded = true;
  gMediaCache->QueueUpdate();
}

void
MediaCacheStream::NotifyChannelRecreated()
{
  NS_ASSERTION(NS_IsMainThread(), "Only call on main thread");
  ReentrantMonitorAutoEnter mon(gMediaCache->GetReentrantMonitor());
  mChannelEnded = false;
  mDidNotifyDataEnded = false;
}

MediaCacheStream::~MediaCacheStream()
{
  NS_ASSERTION(NS_IsMainThread(), "Only call on main thread");
  NS_ASSERTION(!mPinCount, "Unbalanced Pin");

  if (gMediaCache) {
    NS_ASSERTION(mClosed, "Stream was not closed");
    gMediaCache->ReleaseStream(this);
    MediaCache::MaybeShutdown();
  }
}

void
MediaCacheStream::SetTransportSeekable(bool aIsTransportSeekable)
{
  ReentrantMonitorAutoEnter mon(gMediaCache->GetReentrantMonitor());
  NS_ASSERTION(mIsTransportSeekable || aIsTransportSeekable ||
               mChannelOffset == 0, "channel offset must be zero when we become non-seekable");
  mIsTransportSeekable = aIsTransportSeekable;
  
  
  gMediaCache->QueueUpdate();
}

bool
MediaCacheStream::IsTransportSeekable()
{
  ReentrantMonitorAutoEnter mon(gMediaCache->GetReentrantMonitor());
  return mIsTransportSeekable;
}

bool
MediaCacheStream::AreAllStreamsForResourceSuspended()
{
  ReentrantMonitorAutoEnter mon(gMediaCache->GetReentrantMonitor());
  MediaCache::ResourceStreamIterator iter(mResourceID);
  
  int64_t dataOffset = -1;
  while (MediaCacheStream* stream = iter.Next()) {
    if (stream->mCacheSuspended || stream->mChannelEnded || stream->mClosed) {
      continue;
    }
    if (dataOffset < 0) {
      dataOffset = GetCachedDataEndInternal(mStreamOffset);
    }
    
    if (stream->mChannelOffset > dataOffset) {
      continue;
    }
    return false;
  }

  return true;
}

void
MediaCacheStream::Close()
{
  NS_ASSERTION(NS_IsMainThread(), "Only call on main thread");

  ReentrantMonitorAutoEnter mon(gMediaCache->GetReentrantMonitor());
  CloseInternal(mon);
  
  
  
  gMediaCache->QueueUpdate();
}

void
MediaCacheStream::EnsureCacheUpdate()
{
  if (mHasHadUpdate)
    return;
  gMediaCache->Update();
}

void
MediaCacheStream::CloseInternal(ReentrantMonitorAutoEnter& aReentrantMonitor)
{
  NS_ASSERTION(NS_IsMainThread(), "Only call on main thread");

  if (mClosed)
    return;
  mClosed = true;
  gMediaCache->ReleaseStreamBlocks(this);
  
  aReentrantMonitor.NotifyAll();
}

void
MediaCacheStream::Pin()
{
  ReentrantMonitorAutoEnter mon(gMediaCache->GetReentrantMonitor());
  ++mPinCount;
  
  
  gMediaCache->QueueUpdate();
}

void
MediaCacheStream::Unpin()
{
  ReentrantMonitorAutoEnter mon(gMediaCache->GetReentrantMonitor());
  NS_ASSERTION(mPinCount > 0, "Unbalanced Unpin");
  --mPinCount;
  
  
  gMediaCache->QueueUpdate();
}

int64_t
MediaCacheStream::GetLength()
{
  ReentrantMonitorAutoEnter mon(gMediaCache->GetReentrantMonitor());
  return mStreamLength;
}

int64_t
MediaCacheStream::GetNextCachedData(int64_t aOffset)
{
  ReentrantMonitorAutoEnter mon(gMediaCache->GetReentrantMonitor());
  return GetNextCachedDataInternal(aOffset);
}

int64_t
MediaCacheStream::GetCachedDataEnd(int64_t aOffset)
{
  ReentrantMonitorAutoEnter mon(gMediaCache->GetReentrantMonitor());
  return GetCachedDataEndInternal(aOffset);
}

bool
MediaCacheStream::IsDataCachedToEndOfStream(int64_t aOffset)
{
  ReentrantMonitorAutoEnter mon(gMediaCache->GetReentrantMonitor());
  if (mStreamLength < 0)
    return false;
  return GetCachedDataEndInternal(aOffset) >= mStreamLength;
}

int64_t
MediaCacheStream::GetCachedDataEndInternal(int64_t aOffset)
{
  gMediaCache->GetReentrantMonitor().AssertCurrentThreadIn();
  uint32_t startBlockIndex = aOffset/BLOCK_SIZE;
  uint32_t blockIndex = startBlockIndex;
  while (blockIndex < mBlocks.Length() && mBlocks[blockIndex] != -1) {
    ++blockIndex;
  }
  int64_t result = blockIndex*BLOCK_SIZE;
  if (blockIndex == mChannelOffset/BLOCK_SIZE) {
    
    
    result = mChannelOffset;
  }
  if (mStreamLength >= 0) {
    
    
    result = std::min(result, mStreamLength);
  }
  return std::max(result, aOffset);
}

int64_t
MediaCacheStream::GetNextCachedDataInternal(int64_t aOffset)
{
  gMediaCache->GetReentrantMonitor().AssertCurrentThreadIn();
  if (aOffset == mStreamLength)
    return -1;

  uint32_t startBlockIndex = aOffset/BLOCK_SIZE;
  uint32_t channelBlockIndex = mChannelOffset/BLOCK_SIZE;

  if (startBlockIndex == channelBlockIndex &&
      aOffset < mChannelOffset) {
    
    
    
    return aOffset;
  }

  if (startBlockIndex >= mBlocks.Length())
    return -1;

  
  if (mBlocks[startBlockIndex] != -1)
    return aOffset;

  
  bool hasPartialBlock = (mChannelOffset % BLOCK_SIZE) != 0;
  uint32_t blockIndex = startBlockIndex + 1;
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
MediaCacheStream::SetReadMode(ReadMode aMode)
{
  ReentrantMonitorAutoEnter mon(gMediaCache->GetReentrantMonitor());
  if (aMode == mCurrentMode)
    return;
  mCurrentMode = aMode;
  gMediaCache->QueueUpdate();
}

void
MediaCacheStream::SetPlaybackRate(uint32_t aBytesPerSecond)
{
  NS_ASSERTION(aBytesPerSecond > 0, "Zero playback rate not allowed");
  ReentrantMonitorAutoEnter mon(gMediaCache->GetReentrantMonitor());
  if (aBytesPerSecond == mPlaybackBytesPerSecond)
    return;
  mPlaybackBytesPerSecond = aBytesPerSecond;
  gMediaCache->QueueUpdate();
}

nsresult
MediaCacheStream::Seek(int32_t aWhence, int64_t aOffset)
{
  NS_ASSERTION(!NS_IsMainThread(), "Don't call on main thread");

  ReentrantMonitorAutoEnter mon(gMediaCache->GetReentrantMonitor());
  if (mClosed)
    return NS_ERROR_FAILURE;

  int64_t oldOffset = mStreamOffset;
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

  CACHE_LOG(PR_LOG_DEBUG, ("Stream %p Seek to %lld", this, (long long)mStreamOffset));
  gMediaCache->NoteSeek(this, oldOffset);

  gMediaCache->QueueUpdate();
  return NS_OK;
}

int64_t
MediaCacheStream::Tell()
{
  NS_ASSERTION(!NS_IsMainThread(), "Don't call on main thread");

  ReentrantMonitorAutoEnter mon(gMediaCache->GetReentrantMonitor());
  return mStreamOffset;
}

nsresult
MediaCacheStream::Read(char* aBuffer, uint32_t aCount, uint32_t* aBytes)
{
  NS_ASSERTION(!NS_IsMainThread(), "Don't call on main thread");

  ReentrantMonitorAutoEnter mon(gMediaCache->GetReentrantMonitor());
  if (mClosed)
    return NS_ERROR_FAILURE;

  uint32_t count = 0;
  
  while (count < aCount) {
    uint32_t streamBlock = uint32_t(mStreamOffset/BLOCK_SIZE);
    uint32_t offsetInStreamBlock =
      uint32_t(mStreamOffset - streamBlock*BLOCK_SIZE);
    int64_t size = std::min(aCount - count, BLOCK_SIZE - offsetInStreamBlock);

    if (mStreamLength >= 0) {
      
      int64_t bytesRemaining = mStreamLength - mStreamOffset;
      if (bytesRemaining <= 0) {
        
        break;
      }
      size = std::min(size, bytesRemaining);
      
      size = std::min(size, int64_t(INT32_MAX));
    }

    int32_t bytes;
    int32_t cacheBlock = streamBlock < mBlocks.Length() ? mBlocks[streamBlock] : -1;
    if (cacheBlock < 0) {
      

      if (count > 0) {
        
        
        break;
      }

      
      
      
      
      MediaCacheStream* streamWithPartialBlock = nullptr;
      MediaCache::ResourceStreamIterator iter(mResourceID);
      while (MediaCacheStream* stream = iter.Next()) {
        if (uint32_t(stream->mChannelOffset/BLOCK_SIZE) == streamBlock &&
            mStreamOffset < stream->mChannelOffset) {
          streamWithPartialBlock = stream;
          break;
        }
      }
      if (streamWithPartialBlock) {
        
        
        
        bytes = std::min<int64_t>(size, streamWithPartialBlock->mChannelOffset - mStreamOffset);
        memcpy(aBuffer,
          reinterpret_cast<char*>(streamWithPartialBlock->mPartialBlockBuffer.get()) + offsetInStreamBlock, bytes);
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

    int64_t offset = cacheBlock*BLOCK_SIZE + offsetInStreamBlock;
    NS_ABORT_IF_FALSE(size >= 0 && size <= INT32_MAX, "Size out of range.");
    nsresult rv = gMediaCache->ReadCacheFile(offset, aBuffer + count, int32_t(size), &bytes);
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
  CACHE_LOG(PR_LOG_DEBUG,
            ("Stream %p Read at %lld count=%d", this, (long long)(mStreamOffset-count), count));
  *aBytes = count;
  return NS_OK;
}

nsresult
MediaCacheStream::ReadAt(int64_t aOffset, char* aBuffer,
                         uint32_t aCount, uint32_t* aBytes)
{
  NS_ASSERTION(!NS_IsMainThread(), "Don't call on main thread");

  ReentrantMonitorAutoEnter mon(gMediaCache->GetReentrantMonitor());
  nsresult rv = Seek(nsISeekableStream::NS_SEEK_SET, aOffset);
  if (NS_FAILED(rv)) return rv;
  return Read(aBuffer, aCount, aBytes);
}

nsresult
MediaCacheStream::ReadFromCache(char* aBuffer,
                                  int64_t aOffset,
                                  int64_t aCount)
{
  ReentrantMonitorAutoEnter mon(gMediaCache->GetReentrantMonitor());
  if (mClosed)
    return NS_ERROR_FAILURE;

  
  uint32_t count = 0;
  int64_t streamOffset = aOffset;
  while (count < aCount) {
    uint32_t streamBlock = uint32_t(streamOffset/BLOCK_SIZE);
    uint32_t offsetInStreamBlock =
      uint32_t(streamOffset - streamBlock*BLOCK_SIZE);
    int64_t size = std::min<int64_t>(aCount - count, BLOCK_SIZE - offsetInStreamBlock);

    if (mStreamLength >= 0) {
      
      int64_t bytesRemaining = mStreamLength - streamOffset;
      if (bytesRemaining <= 0) {
        return NS_ERROR_FAILURE;
      }
      size = std::min(size, bytesRemaining);
      
      size = std::min(size, int64_t(INT32_MAX));
    }

    int32_t bytes;
    uint32_t channelBlock = uint32_t(mChannelOffset/BLOCK_SIZE);
    int32_t cacheBlock = streamBlock < mBlocks.Length() ? mBlocks[streamBlock] : -1;
    if (channelBlock == streamBlock && streamOffset < mChannelOffset) {
      
      
      
      bytes = std::min<int64_t>(size, mChannelOffset - streamOffset);
      memcpy(aBuffer + count,
        reinterpret_cast<char*>(mPartialBlockBuffer.get()) + offsetInStreamBlock, bytes);
    } else {
      if (cacheBlock < 0) {
        
        return NS_ERROR_FAILURE;
      }
      int64_t offset = cacheBlock*BLOCK_SIZE + offsetInStreamBlock;
      NS_ABORT_IF_FALSE(size >= 0 && size <= INT32_MAX, "Size out of range.");
      nsresult rv = gMediaCache->ReadCacheFile(offset, aBuffer + count, int32_t(size), &bytes);
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
MediaCacheStream::Init()
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
MediaCacheStream::InitAsClone(MediaCacheStream* aOriginal)
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
  mIsTransportSeekable = aOriginal->mIsTransportSeekable;

  
  
  mCacheSuspended = true;
  mChannelEnded = true;

  if (aOriginal->mDidNotifyDataEnded) {
    mNotifyDataEndedStatus = aOriginal->mNotifyDataEndedStatus;
    mDidNotifyDataEnded = true;
    mClient->CacheClientNotifyDataEnded(mNotifyDataEndedStatus);
  }

  for (uint32_t i = 0; i < aOriginal->mBlocks.Length(); ++i) {
    int32_t cacheBlockIndex = aOriginal->mBlocks[i];
    if (cacheBlockIndex < 0)
      continue;

    while (i >= mBlocks.Length()) {
      mBlocks.AppendElement(-1);
    }
    
    
    gMediaCache->AddBlockOwnerAsReadahead(cacheBlockIndex, this, i);
  }

  return NS_OK;
}

nsresult MediaCacheStream::GetCachedRanges(nsTArray<MediaByteRange>& aRanges)
{
  
  
  ReentrantMonitorAutoEnter mon(gMediaCache->GetReentrantMonitor());

  
  
  NS_ASSERTION(mPinCount > 0, "Must be pinned");

  int64_t startOffset = GetNextCachedData(0);
  while (startOffset >= 0) {
    int64_t endOffset = GetCachedDataEnd(startOffset);
    NS_ASSERTION(startOffset < endOffset, "Buffered range must end after its start");
    
    aRanges.AppendElement(MediaByteRange(startOffset, endOffset));
    startOffset = GetNextCachedData(endOffset);
    NS_ASSERTION(startOffset == -1 || startOffset > endOffset,
      "Must have advanced to start of next range, or hit end of stream");
  }
  return NS_OK;
}

} 

