




































#if !defined(MediaResource_h_)
#define MediaResource_h_

#include "mozilla/Mutex.h"
#include "mozilla/XPCOM.h"
#include "nsIChannel.h"
#include "nsIPrincipal.h"
#include "nsIURI.h"
#include "nsIStreamListener.h"
#include "nsIChannelEventSink.h"
#include "nsIInterfaceRequestor.h"
#include "nsMediaCache.h"




static const PRInt64 SEEK_VS_READ_THRESHOLD = 32*1024;

static const PRUint32 HTTP_REQUESTED_RANGE_NOT_SATISFIABLE_CODE = 416;

class nsMediaDecoder;

namespace mozilla {














class MediaChannelStatistics {
public:
  MediaChannelStatistics() { Reset(); }
  void Reset() {
    mLastStartTime = TimeStamp();
    mAccumulatedTime = TimeDuration(0);
    mAccumulatedBytes = 0;
    mIsStarted = false;
  }
  void Start(TimeStamp aNow) {
    if (mIsStarted)
      return;
    mLastStartTime = aNow;
    mIsStarted = true;
  }
  void Stop(TimeStamp aNow) {
    if (!mIsStarted)
      return;
    mAccumulatedTime += aNow - mLastStartTime;
    mIsStarted = false;
  }
  void AddBytes(PRInt64 aBytes) {
    if (!mIsStarted) {
      
      
      return;
    }
    mAccumulatedBytes += aBytes;
  }
  double GetRateAtLastStop(bool* aReliable) {
    double seconds = mAccumulatedTime.ToSeconds();
    *aReliable = seconds >= 1.0;
    if (seconds <= 0.0)
      return 0.0;
    return static_cast<double>(mAccumulatedBytes)/seconds;
  }
  double GetRate(TimeStamp aNow, bool* aReliable) {
    TimeDuration time = mAccumulatedTime;
    if (mIsStarted) {
      time += aNow - mLastStartTime;
    }
    double seconds = time.ToSeconds();
    *aReliable = seconds >= 3.0;
    if (seconds <= 0.0)
      return 0.0;
    return static_cast<double>(mAccumulatedBytes)/seconds;
  }
private:
  PRInt64      mAccumulatedBytes;
  TimeDuration mAccumulatedTime;
  TimeStamp    mLastStartTime;
  bool         mIsStarted;
};



class MediaByteRange {
public:
  MediaByteRange() : mStart(0), mEnd(0) {}

  MediaByteRange(PRInt64 aStart, PRInt64 aEnd)
    : mStart(aStart), mEnd(aEnd)
  {
    NS_ASSERTION(mStart < mEnd, "Range should end after start!");
  }

  bool IsNull() const {
    return mStart == 0 && mEnd == 0;
  }

  PRInt64 mStart, mEnd;
};






















class MediaResource
{
public:
  virtual ~MediaResource()
  {
    MOZ_COUNT_DTOR(MediaResource);
  }

  
  
  nsIURI* URI() const { return mURI; }
  
  
  
  virtual nsresult Close() = 0;
  
  
  
  
  
  virtual void Suspend(bool aCloseImmediately) = 0;
  
  virtual void Resume() = 0;
  
  virtual already_AddRefed<nsIPrincipal> GetCurrentPrincipal() = 0;
  
  
  
  
  
  virtual bool CanClone() { return false; }
  
  
  
  virtual MediaResource* CloneData(nsMediaDecoder* aDecoder) = 0;

  
  
  virtual void SetReadMode(nsMediaCacheStream::ReadMode aMode) = 0;
  
  
  
  virtual void SetPlaybackRate(PRUint32 aBytesPerSecond) = 0;
  
  
  
  
  
  
  virtual nsresult Read(char* aBuffer, PRUint32 aCount, PRUint32* aBytes) = 0;
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual nsresult Seek(PRInt32 aWhence, PRInt64 aOffset) = 0;
  
  virtual PRInt64 Tell() = 0;
  
  
  
  void MoveLoadsToBackground();
  
  
  virtual void EnsureCacheUpToDate() {}

  
  
  
  virtual void Pin() = 0;
  virtual void Unpin() = 0;
  
  
  
  virtual double GetDownloadRate(bool* aIsReliable) = 0;
  
  
  
  
  
  
  virtual PRInt64 GetLength() = 0;
  
  
  virtual PRInt64 GetNextCachedData(PRInt64 aOffset) = 0;
  
  
  virtual PRInt64 GetCachedDataEnd(PRInt64 aOffset) = 0;
  
  
  virtual bool IsDataCachedToEndOfResource(PRInt64 aOffset) = 0;
  
  
  
  
  
  
  
  
  
  virtual bool IsSuspendedByCache(MediaResource** aActiveResource) = 0;
  
  virtual bool IsSuspended() = 0;
  
  
  
  
  
  virtual nsresult ReadFromCache(char* aBuffer,
                                 PRInt64 aOffset,
                                 PRUint32 aCount) = 0;

  



  static MediaResource* Create(nsMediaDecoder* aDecoder, nsIChannel* aChannel);

  



  virtual nsresult Open(nsIStreamListener** aStreamListener) = 0;

  




  virtual nsresult GetCachedRanges(nsTArray<MediaByteRange>& aRanges) = 0;

protected:
  MediaResource(nsMediaDecoder* aDecoder, nsIChannel* aChannel, nsIURI* aURI) :
    mDecoder(aDecoder),
    mChannel(aChannel),
    mURI(aURI),
    mLoadInBackground(false)
  {
    MOZ_COUNT_CTOR(MediaResource);
  }

  
  
  
  void ModifyLoadFlags(nsLoadFlags aFlags);

  
  
  
  nsMediaDecoder* mDecoder;

  
  
  nsCOMPtr<nsIChannel> mChannel;

  
  
  nsCOMPtr<nsIURI> mURI;

  
  
  bool mLoadInBackground;
};









class ChannelMediaResource : public MediaResource
{
public:
  ChannelMediaResource(nsMediaDecoder* aDecoder, nsIChannel* aChannel, nsIURI* aURI);
  ~ChannelMediaResource();

  
  
  
  
  
  void CacheClientNotifyDataReceived();
  
  
  
  void CacheClientNotifyDataEnded(nsresult aStatus);

  
  
  
  
  
  
  
  nsresult CacheClientSeek(PRInt64 aOffset, bool aResume);
  
  nsresult CacheClientSuspend();
  
  nsresult CacheClientResume();

  
  virtual nsresult Open(nsIStreamListener** aStreamListener);
  virtual nsresult Close();
  virtual void     Suspend(bool aCloseImmediately);
  virtual void     Resume();
  virtual already_AddRefed<nsIPrincipal> GetCurrentPrincipal();
  
  bool IsClosed() const { return mCacheStream.IsClosed(); }
  virtual bool     CanClone();
  virtual MediaResource* CloneData(nsMediaDecoder* aDecoder);
  virtual nsresult ReadFromCache(char* aBuffer, PRInt64 aOffset, PRUint32 aCount);
  virtual void     EnsureCacheUpToDate();

  
  virtual void     SetReadMode(nsMediaCacheStream::ReadMode aMode);
  virtual void     SetPlaybackRate(PRUint32 aBytesPerSecond);
  virtual nsresult Read(char* aBuffer, PRUint32 aCount, PRUint32* aBytes);
  virtual nsresult Seek(PRInt32 aWhence, PRInt64 aOffset);
  virtual PRInt64  Tell();

  
  virtual void    Pin();
  virtual void    Unpin();
  virtual double  GetDownloadRate(bool* aIsReliable);
  virtual PRInt64 GetLength();
  virtual PRInt64 GetNextCachedData(PRInt64 aOffset);
  virtual PRInt64 GetCachedDataEnd(PRInt64 aOffset);
  virtual bool    IsDataCachedToEndOfResource(PRInt64 aOffset);
  virtual bool    IsSuspendedByCache(MediaResource** aActiveResource);
  virtual bool    IsSuspended();

  class Listener : public nsIStreamListener,
                   public nsIInterfaceRequestor,
                   public nsIChannelEventSink
  {
  public:
    Listener(ChannelMediaResource* aResource) : mResource(aResource) {}

    NS_DECL_ISUPPORTS
    NS_DECL_NSIREQUESTOBSERVER
    NS_DECL_NSISTREAMLISTENER
    NS_DECL_NSICHANNELEVENTSINK
    NS_DECL_NSIINTERFACEREQUESTOR

    void Revoke() { mResource = nsnull; }

  private:
    ChannelMediaResource* mResource;
  };
  friend class Listener;

  nsresult GetCachedRanges(nsTArray<MediaByteRange>& aRanges);

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

  
  
  
  void PossiblySuspend();

  
  void PossiblyResume();

  
  PRInt64            mOffset;
  nsRefPtr<Listener> mListener;
  
  
  nsRevocableEventPtr<nsRunnableMethod<ChannelMediaResource, void, false> > mDataReceivedEvent;
  PRUint32           mSuspendCount;
  
  
  bool               mReopenOnError;
  
  
  bool               mIgnoreClose;

  
  nsMediaCacheStream mCacheStream;

  
  Mutex               mLock;
  MediaChannelStatistics mChannelStatistics;

  
  
  
  bool mIgnoreResume;
};

}

#endif
