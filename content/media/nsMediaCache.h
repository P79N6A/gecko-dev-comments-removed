





#ifndef nsMediaCache_h_
#define nsMediaCache_h_

#include "nsTArray.h"
#include "nsIPrincipal.h"
#include "nsCOMPtr.h"

namespace mozilla {

class ChannelMediaResource;
class MediaByteRange;
class MediaResource;
class ReentrantMonitorAutoEnter;
}

























































































































































class nsMediaCache;







class nsMediaCacheStream {
public:
  typedef mozilla::ChannelMediaResource ChannelMediaResource;
  typedef mozilla::MediaByteRange MediaByteRange;
  typedef mozilla::MediaResource MediaResource;
  typedef mozilla::ReentrantMonitorAutoEnter ReentrantMonitorAutoEnter;

  enum {
    
    BLOCK_SIZE = 32768
  };
  enum ReadMode {
    MODE_METADATA,
    MODE_PLAYBACK
  };

  
  
  nsMediaCacheStream(ChannelMediaResource* aClient)
    : mClient(aClient), mInitialized(false),
      mHasHadUpdate(false),
      mClosed(false),
      mDidNotifyDataEnded(false), mResourceID(0),
      mIsSeekable(false), mCacheSuspended(false),
      mChannelEnded(false),
      mChannelOffset(0), mStreamLength(-1),  
      mStreamOffset(0), mPlaybackBytesPerSecond(10000),
      mPinCount(0), mCurrentMode(MODE_PLAYBACK),
      mMetadataInPartialBlockBuffer(false) {}
  ~nsMediaCacheStream();

  
  
  
  nsresult Init();

  
  
  
  
  nsresult InitAsClone(nsMediaCacheStream* aOriginal);

  
  
  
  
  
  
  
  void SetSeekable(bool aIsSeekable);
  
  
  void Close();
  
  bool IsClosed() const { return mClosed; }
  
  bool IsAvailableForSharing() const
  {
    return !mClosed &&
      (!mDidNotifyDataEnded || NS_SUCCEEDED(mNotifyDataEndedStatus));
  }
  
  
  nsIPrincipal* GetCurrentPrincipal() { return mPrincipal; }
  
  
  
  
  void EnsureCacheUpdate();

  
  
  
  
  
  
  
  
  
  
  
  
  
  void NotifyDataLength(PRInt64 aLength);
  
  
  
  
  
  
  
  void NotifyDataStarted(PRInt64 aOffset);
  
  
  
  
  
  
  void NotifyDataReceived(PRInt64 aSize, const char* aData,
                          nsIPrincipal* aPrincipal);
  
  void NotifyDataEnded(nsresult aStatus);

  
  
  
  void Pin();
  void Unpin();
  
  
  
  
  
  
  PRInt64 GetLength();
  
  
  PRInt64 GetResourceID() { return mResourceID; }
  
  
  PRInt64 GetCachedDataEnd(PRInt64 aOffset);
  
  
  PRInt64 GetNextCachedData(PRInt64 aOffset);
  
  
  
  
  nsresult GetCachedRanges(nsTArray<MediaByteRange>& aRanges);

  
  
  
  
  nsresult ReadFromCache(char* aBuffer,
                         PRInt64 aOffset,
                         PRInt64 aCount);

  
  
  
  
  bool IsDataCachedToEndOfStream(PRInt64 aOffset);
  
  void SetReadMode(ReadMode aMode);
  
  
  
  
  void SetPlaybackRate(PRUint32 aBytesPerSecond);
  
  bool IsSeekable();

  
  
  
  
  bool AreAllStreamsForResourceSuspended(MediaResource** aActiveResource);

  
  
  
  
  
  nsresult Seek(PRInt32 aWhence, PRInt64 aOffset);
  PRInt64 Tell();
  
  
  
  
  nsresult Read(char* aBuffer, PRUint32 aCount, PRUint32* aBytes);

private:
  friend class nsMediaCache;

  








  class BlockList {
  public:
    BlockList() : mFirstBlock(-1), mCount(0) { mEntries.Init(); }
    ~BlockList() {
      NS_ASSERTION(mFirstBlock == -1 && mCount == 0,
                   "Destroying non-empty block list");
    }
    void AddFirstBlock(PRInt32 aBlock);
    void AddAfter(PRInt32 aBlock, PRInt32 aBefore);
    void RemoveBlock(PRInt32 aBlock);
    
    PRInt32 GetFirstBlock() const { return mFirstBlock; }
    
    PRInt32 GetLastBlock() const;
    
    
    PRInt32 GetNextBlock(PRInt32 aBlock) const;
    
    
    PRInt32 GetPrevBlock(PRInt32 aBlock) const;
    bool IsEmpty() const { return mFirstBlock < 0; }
    PRInt32 GetCount() const { return mCount; }
    
    void NotifyBlockSwapped(PRInt32 aBlockIndex1, PRInt32 aBlockIndex2);
#ifdef DEBUG
    
    void Verify();
#else
    void Verify() {}
#endif

  private:
    struct Entry : public nsUint32HashKey {
      Entry(KeyTypePointer aKey) : nsUint32HashKey(aKey) { }
      Entry(const Entry& toCopy) : nsUint32HashKey(&toCopy.GetKey()),
        mNextBlock(toCopy.mNextBlock), mPrevBlock(toCopy.mPrevBlock) {}

      PRInt32 mNextBlock;
      PRInt32 mPrevBlock;
    };
    nsTHashtable<Entry> mEntries;

    
    PRInt32 mFirstBlock;
    
    PRInt32 mCount;
  };

  
  
  
  
  PRInt64 GetCachedDataEndInternal(PRInt64 aOffset);
  
  
  
  
  PRInt64 GetNextCachedDataInternal(PRInt64 aOffset);
  
  
  
  
  
  void CloseInternal(ReentrantMonitorAutoEnter& aReentrantMonitor);
  
  bool UpdatePrincipal(nsIPrincipal* aPrincipal);

  
  ChannelMediaResource*  mClient;
  nsCOMPtr<nsIPrincipal> mPrincipal;
  
  bool                   mInitialized;
  
  
  bool                   mHasHadUpdate;
  
  
  bool                   mClosed;
  
  bool                   mDidNotifyDataEnded;

  
  
  

  
  
  
  PRInt64 mResourceID;
  
  bool mIsSeekable;
  
  
  
  bool mCacheSuspended;
  
  bool mChannelEnded;
  
  PRInt64      mChannelOffset;
  
  
  PRInt64      mStreamLength;

  
  

  
  PRInt64           mStreamOffset;
  
  
  nsTArray<PRInt32> mBlocks;
  
  
  
  BlockList         mReadaheadBlocks;
  
  BlockList         mMetadataBlocks;
  
  BlockList         mPlayedBlocks;
  
  PRUint32          mPlaybackBytesPerSecond;
  
  
  PRUint32          mPinCount;
  
  
  nsresult          mNotifyDataEndedStatus;
  
  ReadMode          mCurrentMode;
  
  bool              mMetadataInPartialBlockBuffer;

  
  

  
  
  
  
  
  PRInt64           mPartialBlockBuffer[BLOCK_SIZE/sizeof(PRInt64)];
};

#endif
