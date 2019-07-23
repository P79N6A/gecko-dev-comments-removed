





































#ifndef nsMediaCache_h_
#define nsMediaCache_h_

#include "nsTArray.h"
#include "prinrval.h"
#include "nsAutoLock.h"


















































































































































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
      mMetadataInPartialBlockBuffer(PR_FALSE) {}
  ~nsMediaCacheStream();

  
  
  
  nsresult Init();

  
  
  
  
  
  
  
  void SetSeekable(PRBool aIsSeekable);
  
  
  void Close();
  
  PRBool IsClosed() const { return mClosed; }

  
  
  
  
  
  
  
  
  
  
  
  
  
  void NotifyDataLength(PRInt64 aLength);
  
  
  
  
  
  
  void NotifyDataStarted(PRInt64 aOffset);
  
  
  
  
  
  void NotifyDataReceived(PRInt64 aSize, const char* aData);
  
  void NotifyDataEnded(nsresult aStatus);

  
  
  
  void Pin();
  void Unpin();
  
  
  
  
  
  
  PRInt64 GetLength();
  
  
  PRInt64 GetCachedDataEnd(PRInt64 aOffset);
  
  
  
  
  
  PRBool IsDataCachedToEndOfStream(PRInt64 aOffset);
  
  void SetReadMode(ReadMode aMode);
  
  
  
  
  void SetPlaybackRate(PRUint32 aBytesPerSecond);

  
  
  
  
  
  nsresult Seek(PRInt32 aWhence, PRInt64 aOffset);
  PRInt64 Tell();
  
  
  
  
  nsresult Read(char* aBuffer, PRUint32 aCount, PRUint32* aBytes);

private:
  friend class nsMediaCache;

  






  class BlockList {
  public:
    BlockList() : mFirstBlock(-1), mCount(0) {}
    ~BlockList() {
      NS_ASSERTION(mFirstBlock == -1 && mCount == 0,
                   "Destroying non-empty block list");
    }
    void AddFirstBlock(PRInt32 aBlock);
    void AddAfter(PRInt32 aBlock, PRInt32 aBefore);
    void RemoveBlock(PRInt32 aBlock);
    
    PRInt32 GetFirstBlock() const { return mFirstBlock; }
    
    PRInt32 GetLastBlock() const;
    PRBool IsEmpty() const { return mFirstBlock < 0; }
    PRInt32 GetCount() const { return mCount; }
    
    
    void NotifyBlockSwapped(PRInt32 aBlockIndex1, PRInt32 aBlockIndex2);
#ifdef DEBUG
    
    void Verify();
#else
    void Verify() {}
#endif

  private:
    
    PRInt32 mFirstBlock;
    
    PRInt32 mCount;
  };

  
  
  
  
  PRInt64 GetCachedDataEndInternal(PRInt64 aOffset);
  
  
  
  
  
  void CloseInternal(nsAutoMonitor* aMonitor);

  
  nsMediaChannelStream* mClient;

  
  
  
  PRInt64           mChannelOffset;
  
  PRInt64           mStreamOffset;
  
  
  PRInt64           mStreamLength;
  
  
  nsTArray<PRInt32> mBlocks;
  
  
  
  BlockList         mReadaheadBlocks;
  
  PRUint32          mPlaybackBytesPerSecond;
  
  
  PRUint32          mPinCount;
  
  ReadMode          mCurrentMode;
  
  
  PRPackedBool      mClosed;
  
  PRPackedBool      mIsSeekable;
  
  
  
  PRPackedBool      mCacheSuspended;
  
  PRPackedBool      mMetadataInPartialBlockBuffer;

  
  
  
  
  
  PRInt64           mPartialBlockBuffer[BLOCK_SIZE/sizeof(PRInt64)];
};

#endif
