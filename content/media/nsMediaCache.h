





































#ifndef nsMediaCache_h_
#define nsMediaCache_h_

#include "nsTArray.h"
#include "nsAutoLock.h"
#include "nsIPrincipal.h"
#include "nsCOMPtr.h"

























































































































































class nsMediaCache;

class nsMediaChannelStream;







class nsMediaCacheStream {
public:
  enum {
    
    BLOCK_SIZE = 4096
  };
  enum ReadMode {
    MODE_METADATA,
    MODE_PLAYBACK
  };

  
  
  nsMediaCacheStream(nsMediaChannelStream* aClient)
    : mClient(aClient), mChannelOffset(0),
      mStreamOffset(0), mStreamLength(-1), mPlaybackBytesPerSecond(10000),
      mPinCount(0), mCurrentMode(MODE_PLAYBACK), mClosed(PR_FALSE),
      mIsSeekable(PR_FALSE), mCacheSuspended(PR_FALSE),
      mMetadataInPartialBlockBuffer(PR_FALSE),
      mUsingNullPrincipal(PR_FALSE) {}
  ~nsMediaCacheStream();

  
  
  
  nsresult Init();

  
  
  
  
  
  
  
  void SetSeekable(PRBool aIsSeekable);
  
  
  void Close();
  
  PRBool IsClosed() const { return mClosed; }
  
  nsIPrincipal* GetCurrentPrincipal() { return mPrincipal; }

  
  
  
  
  
  
  
  
  
  
  
  
  
  void NotifyDataLength(PRInt64 aLength);
  
  
  
  
  
  
  
  void NotifyDataStarted(PRInt64 aOffset);
  
  
  
  
  
  
  void NotifyDataReceived(PRInt64 aSize, const char* aData,
                          nsIPrincipal* aPrincipal);
  
  void NotifyDataEnded(nsresult aStatus);

  
  
  
  void Pin();
  void Unpin();
  
  
  
  
  
  
  PRInt64 GetLength();
  
  
  PRInt64 GetCachedDataEnd(PRInt64 aOffset);
  
  
  PRInt64 GetNextCachedData(PRInt64 aOffset);

  
  
  
  
  nsresult ReadFromCache(char* aBuffer,
                         PRInt64 aOffset,
                         PRInt64 aCount);

  
  
  
  
  PRBool IsDataCachedToEndOfStream(PRInt64 aOffset);
  
  void SetReadMode(ReadMode aMode);
  
  
  
  
  void SetPlaybackRate(PRUint32 aBytesPerSecond);
  
  PRBool IsSeekable();

  
  
  
  
  
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
    PRBool IsEmpty() const { return mFirstBlock < 0; }
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
  
  
  
  
  
  void CloseInternal(nsAutoMonitor* aMonitor);
  
  void UpdatePrincipal(nsIPrincipal* aPrincipal);

  
  nsMediaChannelStream*  mClient;
  nsCOMPtr<nsIPrincipal> mPrincipal;

  
  
  
  PRInt64           mChannelOffset;
  
  PRInt64           mStreamOffset;
  
  
  PRInt64           mStreamLength;
  
  
  nsTArray<PRInt32> mBlocks;
  
  
  
  BlockList         mReadaheadBlocks;
  
  BlockList         mMetadataBlocks;
  
  BlockList         mPlayedBlocks;
  
  PRUint32          mPlaybackBytesPerSecond;
  
  
  PRUint32          mPinCount;
  
  ReadMode          mCurrentMode;
  
  
  PRPackedBool      mClosed;
  
  PRPackedBool      mIsSeekable;
  
  
  
  PRPackedBool      mCacheSuspended;
  
  PRPackedBool      mMetadataInPartialBlockBuffer;
  
  
  PRPackedBool      mUsingNullPrincipal;

  
  
  
  
  
  PRInt64           mPartialBlockBuffer[BLOCK_SIZE/sizeof(PRInt64)];
};

#endif
