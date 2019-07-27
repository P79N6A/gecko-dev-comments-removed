





#ifndef MediaCache_h_
#define MediaCache_h_

#include "nsTArray.h"
#include "nsCOMPtr.h"
#include "nsHashKeys.h"
#include "nsTHashtable.h"

class nsIPrincipal;

namespace mozilla {

class ChannelMediaResource;
class MediaByteRange;
class MediaResource;
class ReentrantMonitorAutoEnter;

























































































































































class MediaCache;







class MediaCacheStream {
public:
  enum {
    
    BLOCK_SIZE = 32768
  };
  enum ReadMode {
    MODE_METADATA,
    MODE_PLAYBACK
  };

  
  
  explicit MediaCacheStream(ChannelMediaResource* aClient);
  ~MediaCacheStream();

  
  
  
  nsresult Init();

  
  
  
  
  nsresult InitAsClone(MediaCacheStream* aOriginal);

  
  
  
  
  
  
  
  void SetTransportSeekable(bool aIsTransportSeekable);
  
  
  void Close();
  
  bool IsClosed() const { return mClosed; }
  
  bool IsAvailableForSharing() const
  {
    return !mClosed &&
      (!mDidNotifyDataEnded || NS_SUCCEEDED(mNotifyDataEndedStatus));
  }
  
  
  nsIPrincipal* GetCurrentPrincipal() { return mPrincipal; }
  
  
  
  
  void EnsureCacheUpdate();

  
  
  
  
  
  
  
  
  
  
  
  
  
  void NotifyDataLength(int64_t aLength);
  
  
  
  
  
  
  
  void NotifyDataStarted(int64_t aOffset);
  
  
  
  
  
  
  void NotifyDataReceived(int64_t aSize, const char* aData,
                          nsIPrincipal* aPrincipal);
  
  
  void FlushPartialBlock();
  
  void NotifyDataEnded(nsresult aStatus);

  
  
  void NotifyChannelRecreated();

  
  
  
  void Pin();
  void Unpin();
  
  
  
  
  
  
  int64_t GetLength();
  
  
  int64_t GetResourceID() { return mResourceID; }
  
  
  int64_t GetCachedDataEnd(int64_t aOffset);
  
  
  int64_t GetNextCachedData(int64_t aOffset);
  
  
  
  
  nsresult GetCachedRanges(nsTArray<MediaByteRange>& aRanges);

  
  
  
  
  nsresult ReadFromCache(char* aBuffer,
                         int64_t aOffset,
                         int64_t aCount);

  
  
  
  
  bool IsDataCachedToEndOfStream(int64_t aOffset);
  
  void SetReadMode(ReadMode aMode);
  
  
  
  
  void SetPlaybackRate(uint32_t aBytesPerSecond);
  
  bool IsTransportSeekable();

  
  
  bool AreAllStreamsForResourceSuspended();

  
  
  
  
  
  nsresult Seek(int32_t aWhence, int64_t aOffset);
  int64_t Tell();
  
  
  
  
  nsresult Read(char* aBuffer, uint32_t aCount, uint32_t* aBytes);
  
  
  nsresult ReadAt(int64_t aOffset, char* aBuffer,
                  uint32_t aCount, uint32_t* aBytes);

  size_t SizeOfExcludingThis(MallocSizeOf aMallocSizeOf) const;

private:
  friend class MediaCache;

  








  class BlockList {
  public:
    BlockList() : mFirstBlock(-1), mCount(0) {}
    ~BlockList() {
      NS_ASSERTION(mFirstBlock == -1 && mCount == 0,
                   "Destroying non-empty block list");
    }
    void AddFirstBlock(int32_t aBlock);
    void AddAfter(int32_t aBlock, int32_t aBefore);
    void RemoveBlock(int32_t aBlock);
    
    int32_t GetFirstBlock() const { return mFirstBlock; }
    
    int32_t GetLastBlock() const;
    
    
    int32_t GetNextBlock(int32_t aBlock) const;
    
    
    int32_t GetPrevBlock(int32_t aBlock) const;
    bool IsEmpty() const { return mFirstBlock < 0; }
    int32_t GetCount() const { return mCount; }
    
    void NotifyBlockSwapped(int32_t aBlockIndex1, int32_t aBlockIndex2);
#ifdef DEBUG
    
    void Verify();
#else
    void Verify() {}
#endif

    size_t SizeOfExcludingThis(mozilla::MallocSizeOf aMallocSizeOf) const;

  private:
    struct Entry : public nsUint32HashKey {
      explicit Entry(KeyTypePointer aKey) : nsUint32HashKey(aKey) { }
      Entry(const Entry& toCopy) : nsUint32HashKey(&toCopy.GetKey()),
        mNextBlock(toCopy.mNextBlock), mPrevBlock(toCopy.mPrevBlock) {}

      int32_t mNextBlock;
      int32_t mPrevBlock;
    };
    nsTHashtable<Entry> mEntries;

    
    int32_t mFirstBlock;
    
    int32_t mCount;
  };

  
  
  
  
  int64_t GetCachedDataEndInternal(int64_t aOffset);
  
  
  
  
  int64_t GetNextCachedDataInternal(int64_t aOffset);
  
  
  
  
  void FlushPartialBlockInternal(bool aNotify);
  
  
  
  
  
  void CloseInternal(ReentrantMonitorAutoEnter& aReentrantMonitor);
  
  bool UpdatePrincipal(nsIPrincipal* aPrincipal);

  
  ChannelMediaResource*  mClient;
  nsCOMPtr<nsIPrincipal> mPrincipal;
  
  bool                   mInitialized;
  
  
  bool                   mHasHadUpdate;
  
  
  bool                   mClosed;
  
  bool                   mDidNotifyDataEnded;

  
  
  

  
  
  
  int64_t mResourceID;
  
  bool mIsTransportSeekable;
  
  
  
  bool mCacheSuspended;
  
  bool mChannelEnded;
  
  int64_t      mChannelOffset;
  
  
  int64_t      mStreamLength;

  
  

  
  int64_t           mStreamOffset;
  
  
  nsTArray<int32_t> mBlocks;
  
  
  
  BlockList         mReadaheadBlocks;
  
  BlockList         mMetadataBlocks;
  
  BlockList         mPlayedBlocks;
  
  uint32_t          mPlaybackBytesPerSecond;
  
  
  uint32_t          mPinCount;
  
  
  nsresult          mNotifyDataEndedStatus;
  
  ReadMode          mCurrentMode;
  
  bool              mMetadataInPartialBlockBuffer;

  
  

  
  
  
  
  
  
  
  nsAutoArrayPtr<int64_t> mPartialBlockBuffer;
};

} 

#endif
