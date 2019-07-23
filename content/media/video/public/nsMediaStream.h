




































#if !defined(nsMediaStream_h_)
#define nsMediaStream_h_

#include "nsCOMPtr.h"
#include "nsAutoPtr.h"
#include "nsIChannel.h"
#include "nsIPrincipal.h"
#include "nsIURI.h"
#include "nsIStreamListener.h"
#include "prlock.h"
#include "nsMediaCache.h"




#define SEEK_VS_READ_THRESHOLD (32*1024)

class nsMediaDecoder;















class nsChannelStatistics {
public:
  nsChannelStatistics() { Reset(); }
  void Reset() {
    mLastStartTime = mAccumulatedTime = 0;
    mAccumulatedBytes = 0;
    mIsStarted = PR_FALSE;
  }
  void Start(PRIntervalTime aNow) {
    if (mIsStarted)
      return;
    mLastStartTime = aNow;
    mIsStarted = PR_TRUE;
  }
  void Stop(PRIntervalTime aNow) {
    if (!mIsStarted)
      return;
    mAccumulatedTime += aNow - mLastStartTime;
    mIsStarted = PR_FALSE;
  }
  void AddBytes(PRInt64 aBytes) {
    if (!mIsStarted) {
      
      
      return;
    }
    mAccumulatedBytes += aBytes;
  }
  double GetRateAtLastStop(PRPackedBool* aReliable) {
    *aReliable = mAccumulatedTime >= PR_TicksPerSecond();
    return double(mAccumulatedBytes)*PR_TicksPerSecond()/mAccumulatedTime;
  }
  double GetRate(PRIntervalTime aNow, PRPackedBool* aReliable) {
    PRIntervalTime time = mAccumulatedTime;
    if (mIsStarted) {
      time += aNow - mLastStartTime;
    }
    *aReliable = time >= PR_TicksPerSecond();
    NS_ASSERTION(time >= 0, "Time wraparound?");
    if (time <= 0)
      return 0.0;
    return double(mAccumulatedBytes)*PR_TicksPerSecond()/time;
  }
private:
  PRInt64        mAccumulatedBytes;
  PRIntervalTime mAccumulatedTime;
  PRIntervalTime mLastStartTime;
  PRPackedBool   mIsStarted;
};
















class nsMediaStream 
{
public:
  virtual ~nsMediaStream()
  {
    MOZ_COUNT_DTOR(nsMediaStream);
  }

  
  
  already_AddRefed<nsIPrincipal> GetCurrentPrincipal();
  
  nsMediaDecoder* Decoder() { return mDecoder; }
  
  
  
  virtual nsresult Close() = 0;
  
  virtual void Suspend() = 0;
  
  virtual void Resume() = 0;

  
  
  virtual void SetReadMode(nsMediaCacheStream::ReadMode aMode) = 0;
  
  
  
  virtual void SetPlaybackRate(PRUint32 aBytesPerSecond) = 0;
  
  
  
  
  
  
  virtual nsresult Read(char* aBuffer, PRUint32 aCount, PRUint32* aBytes) = 0;
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual nsresult Seek(PRInt32 aWhence, PRInt64 aOffset) = 0;
  
  virtual PRInt64 Tell() = 0;

  
  
  
  virtual void Pin() = 0;
  virtual void Unpin() = 0;
  
  
  
  virtual double GetDownloadRate(PRPackedBool* aIsReliable) = 0;
  
  
  
  
  
  
  virtual PRInt64 GetLength() = 0;
  
  
  virtual PRInt64 GetCachedDataEnd(PRInt64 aOffset) = 0;
  
  
  virtual PRBool IsDataCachedToEndOfStream(PRInt64 aOffset) = 0;
  
  
  
  
  
  virtual PRBool IsSuspendedByCache() = 0;

  






  static nsresult Open(nsMediaDecoder* aDecoder, nsIURI* aURI,
                       nsIChannel* aChannel, nsMediaStream** aStream,
                       nsIStreamListener** aListener);

protected:
  nsMediaStream(nsMediaDecoder* aDecoder, nsIChannel* aChannel, nsIURI* aURI) :
    mDecoder(aDecoder),
    mChannel(aChannel),
    mURI(aURI)
  {
    MOZ_COUNT_CTOR(nsMediaStream);
  }

  





  virtual nsresult Open(nsIStreamListener** aStreamListener) = 0;

  
  
  
  nsMediaDecoder* mDecoder;

  
  
  nsCOMPtr<nsIChannel> mChannel;

  
  
  nsCOMPtr<nsIURI> mURI;
};









class nsMediaChannelStream : public nsMediaStream
{
public:
  nsMediaChannelStream(nsMediaDecoder* aDecoder, nsIChannel* aChannel, nsIURI* aURI);
  ~nsMediaChannelStream();

  
  
  
  
  
  
  nsresult CacheClientSeek(PRInt64 aOffset);
  
  nsresult CacheClientSuspend();
  
  nsresult CacheClientResume();

  
  virtual nsresult Open(nsIStreamListener** aStreamListener);
  virtual nsresult Close();
  virtual void     Suspend();
  virtual void     Resume();
  
  PRBool IsClosed() const { return mCacheStream.IsClosed(); }

  
  virtual void     SetReadMode(nsMediaCacheStream::ReadMode aMode);
  virtual void     SetPlaybackRate(PRUint32 aBytesPerSecond);
  virtual nsresult Read(char* aBuffer, PRUint32 aCount, PRUint32* aBytes);
  virtual nsresult Seek(PRInt32 aWhence, PRInt64 aOffset);
  virtual PRInt64  Tell();

  
  virtual void    Pin();
  virtual void    Unpin();
  virtual double  GetDownloadRate(PRPackedBool* aIsReliable);
  virtual PRInt64 GetLength();
  virtual PRInt64 GetCachedDataEnd(PRInt64 aOffset);
  virtual PRBool  IsDataCachedToEndOfStream(PRInt64 aOffset);
  virtual PRBool  IsSuspendedByCache();

protected:
  class Listener : public nsIStreamListener {
  public:
    Listener(nsMediaChannelStream* aStream) : mStream(aStream) {}

    NS_DECL_ISUPPORTS
    NS_DECL_NSIREQUESTOBSERVER
    NS_DECL_NSISTREAMLISTENER

    void Revoke() { mStream = nsnull; }

  private:
    nsMediaChannelStream* mStream;
  };
  friend class Listener;

  
  nsresult OnStartRequest(nsIRequest* aRequest);
  nsresult OnStopRequest(nsIRequest* aRequest, nsresult aStatus);
  nsresult OnDataAvailable(nsIRequest* aRequest,
                           nsIInputStream* aStream,
                           PRUint32 aCount);

  
  
  nsresult OpenChannel(nsIStreamListener** aStreamListener, PRInt64 aOffset);
  
  void CloseChannel();

  static NS_METHOD CopySegmentToCache(nsIInputStream *aInStream,
                                      void *aClosure,
                                      const char *aFromSegment,
                                      PRUint32 aToOffset,
                                      PRUint32 aCount,
                                      PRUint32 *aWriteCount);

  
  nsRefPtr<Listener> mListener;
  PRUint32           mSuspendCount;
  PRPackedBool       mSeeking;

  
  nsMediaCacheStream mCacheStream;

  
  PRLock* mLock;
  nsChannelStatistics mChannelStatistics;
  PRUint32            mCacheSuspendCount;
};

#endif
