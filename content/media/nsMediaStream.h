




































#if !defined(nsMediaStream_h_)
#define nsMediaStream_h_

#include "mozilla/XPCOM.h"
#include "nsIChannel.h"
#include "nsIPrincipal.h"
#include "nsIURI.h"
#include "nsIStreamListener.h"
#include "nsIChannelEventSink.h"
#include "nsIInterfaceRequestor.h"
#include "prlock.h"
#include "nsMediaCache.h"




#define SEEK_VS_READ_THRESHOLD (32*1024)

class nsMediaDecoder;














class nsChannelStatistics {
public:
  typedef mozilla::TimeStamp TimeStamp;
  typedef mozilla::TimeDuration TimeDuration;

  nsChannelStatistics() { Reset(); }
  void Reset() {
    mLastStartTime = TimeStamp();
    mAccumulatedTime = TimeDuration(0);
    mAccumulatedBytes = 0;
    mIsStarted = PR_FALSE;
  }
  void Start(TimeStamp aNow) {
    if (mIsStarted)
      return;
    mLastStartTime = aNow;
    mIsStarted = PR_TRUE;
  }
  void Stop(TimeStamp aNow) {
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
    double seconds = mAccumulatedTime.ToSeconds();
    *aReliable = seconds >= 1.0;
    if (seconds <= 0.0)
      return 0.0;
    return double(mAccumulatedBytes)/seconds;
  }
  double GetRate(TimeStamp aNow, PRPackedBool* aReliable) {
    TimeDuration time = mAccumulatedTime;
    if (mIsStarted) {
      time += aNow - mLastStartTime;
    }
    double seconds = time.ToSeconds();
    *aReliable = seconds >= 3.0;
    if (seconds <= 0.0)
      return 0.0;
    return double(mAccumulatedBytes)/seconds;
  }
private:
  PRInt64      mAccumulatedBytes;
  TimeDuration mAccumulatedTime;
  TimeStamp    mLastStartTime;
  PRPackedBool mIsStarted;
};
















class nsMediaStream 
{
public:
  virtual ~nsMediaStream()
  {
    MOZ_COUNT_DTOR(nsMediaStream);
  }

  
  
  nsMediaDecoder* Decoder() { return mDecoder; }
  
  nsIURI* URI() { return mURI; }
  
  
  
  virtual nsresult Close() = 0;
  
  
  
  
  
  virtual void Suspend(PRBool aCloseImmediately) = 0;
  
  virtual void Resume() = 0;
  
  virtual already_AddRefed<nsIPrincipal> GetCurrentPrincipal() = 0;
  
  
  
  virtual nsMediaStream* CloneData(nsMediaDecoder* aDecoder) = 0;

  
  
  virtual void SetReadMode(nsMediaCacheStream::ReadMode aMode) = 0;
  
  
  
  virtual void SetPlaybackRate(PRUint32 aBytesPerSecond) = 0;
  
  
  
  
  
  
  virtual nsresult Read(char* aBuffer, PRUint32 aCount, PRUint32* aBytes) = 0;
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual nsresult Seek(PRInt32 aWhence, PRInt64 aOffset) = 0;
  
  virtual PRInt64 Tell() = 0;
  
  
  
  void MoveLoadsToBackground();

  
  
  
  virtual void Pin() = 0;
  virtual void Unpin() = 0;
  
  
  
  virtual double GetDownloadRate(PRPackedBool* aIsReliable) = 0;
  
  
  
  
  
  
  virtual PRInt64 GetLength() = 0;
  
  
  virtual PRInt64 GetNextCachedData(PRInt64 aOffset) = 0;
  
  
  virtual PRInt64 GetCachedDataEnd(PRInt64 aOffset) = 0;
  
  
  virtual PRBool IsDataCachedToEndOfStream(PRInt64 aOffset) = 0;
  
  
  
  
  
  virtual PRBool IsSuspendedByCache() = 0;

  




  static nsMediaStream* Create(nsMediaDecoder* aDecoder, nsIChannel* aChannel);

  



  virtual nsresult Open(nsIStreamListener** aStreamListener) = 0;

protected:
  nsMediaStream(nsMediaDecoder* aDecoder, nsIChannel* aChannel, nsIURI* aURI) :
    mDecoder(aDecoder),
    mChannel(aChannel),
    mURI(aURI),
    mLoadInBackground(PR_FALSE)
  {
    MOZ_COUNT_CTOR(nsMediaStream);
  }

  
  
  
  nsMediaDecoder* mDecoder;

  
  
  nsCOMPtr<nsIChannel> mChannel;

  
  
  nsCOMPtr<nsIURI> mURI;

  
  
  PRPackedBool mLoadInBackground;
};









class nsMediaChannelStream : public nsMediaStream
{
public:
  nsMediaChannelStream(nsMediaDecoder* aDecoder, nsIChannel* aChannel, nsIURI* aURI);
  ~nsMediaChannelStream();

  
  
  
  
  
  void CacheClientNotifyDataReceived();
  
  
  
  void CacheClientNotifyDataEnded(nsresult aStatus);
  
  
  
  
  nsresult CacheClientSeek(PRInt64 aOffset, PRBool aResume);
  
  nsresult CacheClientSuspend();
  
  nsresult CacheClientResume();

  
  virtual nsresult Open(nsIStreamListener** aStreamListener);
  virtual nsresult Close();
  virtual void     Suspend(PRBool aCloseImmediately);
  virtual void     Resume();
  virtual already_AddRefed<nsIPrincipal> GetCurrentPrincipal();
  
  PRBool IsClosed() const { return mCacheStream.IsClosed(); }
  virtual nsMediaStream* CloneData(nsMediaDecoder* aDecoder);

  
  virtual void     SetReadMode(nsMediaCacheStream::ReadMode aMode);
  virtual void     SetPlaybackRate(PRUint32 aBytesPerSecond);
  virtual nsresult Read(char* aBuffer, PRUint32 aCount, PRUint32* aBytes);
  virtual nsresult Seek(PRInt32 aWhence, PRInt64 aOffset);
  virtual PRInt64  Tell();

  
  virtual void    Pin();
  virtual void    Unpin();
  virtual double  GetDownloadRate(PRPackedBool* aIsReliable);
  virtual PRInt64 GetLength();
  virtual PRInt64 GetNextCachedData(PRInt64 aOffset);
  virtual PRInt64 GetCachedDataEnd(PRInt64 aOffset);
  virtual PRBool  IsDataCachedToEndOfStream(PRInt64 aOffset);
  virtual PRBool  IsSuspendedByCache();

  class Listener : public nsIStreamListener,
                   public nsIInterfaceRequestor,
                   public nsIChannelEventSink
  {
  public:
    Listener(nsMediaChannelStream* aStream) : mStream(aStream) {}

    NS_DECL_ISUPPORTS
    NS_DECL_NSIREQUESTOBSERVER
    NS_DECL_NSISTREAMLISTENER
    NS_DECL_NSICHANNELEVENTSINK
    NS_DECL_NSIINTERFACEREQUESTOR

    void Revoke() { mStream = nsnull; }

  private:
    nsMediaChannelStream* mStream;
  };
  friend class Listener;

protected:
  
  nsresult OnStartRequest(nsIRequest* aRequest);
  nsresult OnStopRequest(nsIRequest* aRequest, nsresult aStatus);
  nsresult OnDataAvailable(nsIRequest* aRequest,
                           nsIInputStream* aStream,
                           PRUint32 aCount);
  nsresult OnChannelRedirect(nsIChannel* aOld, nsIChannel* aNew, PRUint32 aFlags);

  
  
  nsresult OpenChannel(nsIStreamListener** aStreamListener);
  nsresult RecreateChannel();
  void SetupChannelHeaders();
  
  void CloseChannel();

  void DoNotifyDataReceived();

  static NS_METHOD CopySegmentToCache(nsIInputStream *aInStream,
                                      void *aClosure,
                                      const char *aFromSegment,
                                      PRUint32 aToOffset,
                                      PRUint32 aCount,
                                      PRUint32 *aWriteCount);

  
  PRInt64            mOffset;
  nsRefPtr<Listener> mListener;
  
  
  nsRevocableEventPtr<nsNonOwningRunnableMethod<nsMediaChannelStream> > mDataReceivedEvent;
  PRUint32           mSuspendCount;
  
  
  PRPackedBool       mReopenOnError;
  
  
  PRPackedBool       mIgnoreClose;

  
  nsMediaCacheStream mCacheStream;

  
  PRLock* mLock;
  nsChannelStatistics mChannelStatistics;
  PRUint32            mCacheSuspendCount;
};

#endif
