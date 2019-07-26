




#if !defined(MediaResource_h_)
#define MediaResource_h_

#include "mozilla/Mutex.h"
#include "mozilla/XPCOM.h"
#include "mozilla/ReentrantMonitor.h"
#include "nsIChannel.h"
#include "nsIHttpChannel.h"
#include "nsIPrincipal.h"
#include "nsIURI.h"
#include "nsIStreamListener.h"
#include "nsIChannelEventSink.h"
#include "nsIInterfaceRequestor.h"
#include "MediaCache.h"
#include "mozilla/Attributes.h"




static const int64_t SEEK_VS_READ_THRESHOLD = 32*1024;

static const uint32_t HTTP_REQUESTED_RANGE_NOT_SATISFIABLE_CODE = 416;

namespace mozilla {

class MediaDecoder;














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
  void AddBytes(int64_t aBytes) {
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
  int64_t      mAccumulatedBytes;
  TimeDuration mAccumulatedTime;
  TimeStamp    mLastStartTime;
  bool         mIsStarted;
};



class MediaByteRange {
public:
  MediaByteRange() : mStart(0), mEnd(0) {}

  MediaByteRange(int64_t aStart, int64_t aEnd)
    : mStart(aStart), mEnd(aEnd)
  {
    NS_ASSERTION(mStart < mEnd, "Range should end after start!");
  }

  bool IsNull() const {
    return mStart == 0 && mEnd == 0;
  }

  
  void Clear() {
    mStart = 0;
    mEnd = 0;
  }

  int64_t mStart, mEnd;
};






















class MediaResource
{
public:
  virtual ~MediaResource() {}

  
  
  virtual nsIURI* URI() const { return nullptr; }
  
  
  
  virtual nsresult Close() = 0;
  
  
  
  
  
  virtual void Suspend(bool aCloseImmediately) = 0;
  
  virtual void Resume() = 0;
  
  virtual already_AddRefed<nsIPrincipal> GetCurrentPrincipal() = 0;
  
  
  
  
  
  virtual bool CanClone() { return false; }
  
  
  
  virtual MediaResource* CloneData(MediaDecoder* aDecoder) = 0;

  
  
  virtual void SetReadMode(MediaCacheStream::ReadMode aMode) = 0;
  
  
  
  virtual void SetPlaybackRate(uint32_t aBytesPerSecond) = 0;
  
  
  
  
  
  
  virtual nsresult Read(char* aBuffer, uint32_t aCount, uint32_t* aBytes) = 0;
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual nsresult Seek(int32_t aWhence, int64_t aOffset) = 0;
  virtual void StartSeekingForMetadata() = 0;
  virtual void EndSeekingForMetadata() = 0;
  
  virtual int64_t Tell() = 0;
  
  
  
  virtual void MoveLoadsToBackground() {}
  
  
  virtual void EnsureCacheUpToDate() {}

  
  
  
  virtual void Pin() = 0;
  virtual void Unpin() = 0;
  
  
  
  virtual double GetDownloadRate(bool* aIsReliable) = 0;
  
  
  
  
  
  
  virtual int64_t GetLength() = 0;
  
  
  virtual int64_t GetNextCachedData(int64_t aOffset) = 0;
  
  
  virtual int64_t GetCachedDataEnd(int64_t aOffset) = 0;
  
  
  virtual bool IsDataCachedToEndOfResource(int64_t aOffset) = 0;
  
  
  
  
  
  
  
  
  
  virtual bool IsSuspendedByCache(MediaResource** aActiveResource) = 0;
  
  virtual bool IsSuspended() = 0;
  
  
  
  
  
  virtual nsresult ReadFromCache(char* aBuffer,
                                 int64_t aOffset,
                                 uint32_t aCount) = 0;

  



  static MediaResource* Create(MediaDecoder* aDecoder, nsIChannel* aChannel);

  



  virtual nsresult Open(nsIStreamListener** aStreamListener) = 0;

  




  virtual nsresult OpenByteRange(nsIStreamListener** aStreamListener,
                                 MediaByteRange const &aByteRange)
  {
    return Open(aStreamListener);
  }

  




  virtual nsresult GetCachedRanges(nsTArray<MediaByteRange>& aRanges) = 0;
};

class BaseMediaResource : public MediaResource {
public:
  virtual nsIURI* URI() const { return mURI; }
  virtual void MoveLoadsToBackground();

protected:
  BaseMediaResource(MediaDecoder* aDecoder, nsIChannel* aChannel, nsIURI* aURI) :
    mDecoder(aDecoder),
    mChannel(aChannel),
    mURI(aURI),
    mLoadInBackground(false)
  {
    MOZ_COUNT_CTOR(BaseMediaResource);
  }
  virtual ~BaseMediaResource()
  {
    MOZ_COUNT_DTOR(BaseMediaResource);
  }

  
  
  
  void ModifyLoadFlags(nsLoadFlags aFlags);

  
  
  
  MediaDecoder* mDecoder;

  
  
  nsCOMPtr<nsIChannel> mChannel;

  
  
  nsCOMPtr<nsIURI> mURI;

  
  
  bool mLoadInBackground;
};









class ChannelMediaResource : public BaseMediaResource
{
public:
  ChannelMediaResource(MediaDecoder* aDecoder, nsIChannel* aChannel, nsIURI* aURI);
  ~ChannelMediaResource();

  
  
  
  
  
  void CacheClientNotifyDataReceived();
  
  
  
  void CacheClientNotifyDataEnded(nsresult aStatus);
  
  void CacheClientNotifyPrincipalChanged();

  
  
  
  
  
  
  
  nsresult CacheClientSeek(int64_t aOffset, bool aResume);
  
  nsresult CacheClientSuspend();
  
  nsresult CacheClientResume();

  
  virtual nsresult Open(nsIStreamListener** aStreamListener);
  virtual nsresult OpenByteRange(nsIStreamListener** aStreamListener,
                                 MediaByteRange const & aByteRange);
  virtual nsresult Close();
  virtual void     Suspend(bool aCloseImmediately);
  virtual void     Resume();
  virtual already_AddRefed<nsIPrincipal> GetCurrentPrincipal();
  
  bool IsClosed() const { return mCacheStream.IsClosed(); }
  virtual bool     CanClone();
  virtual MediaResource* CloneData(MediaDecoder* aDecoder);
  virtual nsresult ReadFromCache(char* aBuffer, int64_t aOffset, uint32_t aCount);
  virtual void     EnsureCacheUpToDate();

  
  virtual void     SetReadMode(MediaCacheStream::ReadMode aMode);
  virtual void     SetPlaybackRate(uint32_t aBytesPerSecond);
  virtual nsresult Read(char* aBuffer, uint32_t aCount, uint32_t* aBytes);
  virtual nsresult Seek(int32_t aWhence, int64_t aOffset);
  virtual void     StartSeekingForMetadata();
  virtual void     EndSeekingForMetadata();
  virtual int64_t  Tell();

  
  virtual void    Pin();
  virtual void    Unpin();
  virtual double  GetDownloadRate(bool* aIsReliable);
  virtual int64_t GetLength();
  virtual int64_t GetNextCachedData(int64_t aOffset);
  virtual int64_t GetCachedDataEnd(int64_t aOffset);
  virtual bool    IsDataCachedToEndOfResource(int64_t aOffset);
  virtual bool    IsSuspendedByCache(MediaResource** aActiveResource);
  virtual bool    IsSuspended();

  class Listener MOZ_FINAL : public nsIStreamListener,
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

    void Revoke() { mResource = nullptr; }

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
                           uint32_t aCount);
  nsresult OnChannelRedirect(nsIChannel* aOld, nsIChannel* aNew, uint32_t aFlags);

  
  
  nsresult OpenChannel(nsIStreamListener** aStreamListener);
  nsresult RecreateChannel();
  
  void SetupChannelHeaders();
  
  void CloseChannel();

  
  
  
  nsresult ParseContentRangeHeader(nsIHttpChannel * aHttpChan,
                                   int64_t& aRangeStart,
                                   int64_t& aRangeEnd,
                                   int64_t& aRangeTotal);

  void DoNotifyDataReceived();

  static NS_METHOD CopySegmentToCache(nsIInputStream *aInStream,
                                      void *aClosure,
                                      const char *aFromSegment,
                                      uint32_t aToOffset,
                                      uint32_t aCount,
                                      uint32_t *aWriteCount);

  
  
  
  void PossiblySuspend();

  
  void PossiblyResume();

  
  int64_t            mOffset;
  nsRefPtr<Listener> mListener;
  
  
  nsRevocableEventPtr<nsRunnableMethod<ChannelMediaResource, void, false> > mDataReceivedEvent;
  uint32_t           mSuspendCount;
  
  
  bool               mReopenOnError;
  
  
  bool               mIgnoreClose;

  
  MediaCacheStream mCacheStream;

  
  Mutex               mLock;
  MediaChannelStatistics mChannelStatistics;

  
  
  
  bool mIgnoreResume;

  
  bool mSeekingForMetadata;

  
  MediaByteRange mByteRange;

  
  bool mByteRangeDownloads;

  
  bool mByteRangeFirstOpen;

  
  
  
  ReentrantMonitor mSeekOffsetMonitor;
  int64_t mSeekOffset;
};

} 

#endif
