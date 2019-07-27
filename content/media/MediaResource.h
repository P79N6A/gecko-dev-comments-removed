




#if !defined(MediaResource_h_)
#define MediaResource_h_

#include "mozilla/Mutex.h"
#include "nsIChannel.h"
#include "nsIURI.h"
#include "nsIStreamingProtocolController.h"
#include "nsIStreamListener.h"
#include "nsIChannelEventSink.h"
#include "nsIInterfaceRequestor.h"
#include "MediaCache.h"
#include "mozilla/Attributes.h"
#include "mozilla/TimeStamp.h"
#include "nsThreadUtils.h"
#include <algorithm>




static const int64_t SEEK_VS_READ_THRESHOLD = 32*1024;

static const uint32_t HTTP_REQUESTED_RANGE_NOT_SATISFIABLE_CODE = 416;




static const int64_t RELIABLE_DATA_THRESHOLD = 57 * 1460;

class nsIHttpChannel;
class nsIPrincipal;

namespace mozilla {

class MediaDecoder;
class MediaChannelStatistics;














class MediaChannelStatistics {
public:
  MediaChannelStatistics() { Reset(); }

  explicit MediaChannelStatistics(MediaChannelStatistics * aCopyFrom)
  {
    MOZ_ASSERT(aCopyFrom);
    mAccumulatedBytes = aCopyFrom->mAccumulatedBytes;
    mAccumulatedTime = aCopyFrom->mAccumulatedTime;
    mLastStartTime = aCopyFrom->mLastStartTime;
    mIsStarted = aCopyFrom->mIsStarted;
  }

  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(MediaChannelStatistics)

  void Reset() {
    mLastStartTime = TimeStamp();
    mAccumulatedTime = TimeDuration(0);
    mAccumulatedBytes = 0;
    mIsStarted = false;
  }
  void Start() {
    if (mIsStarted)
      return;
    mLastStartTime = TimeStamp::Now();
    mIsStarted = true;
  }
  void Stop() {
    if (!mIsStarted)
      return;
    mAccumulatedTime += TimeStamp::Now() - mLastStartTime;
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
    *aReliable = (seconds >= 1.0) ||
                 (mAccumulatedBytes >= RELIABLE_DATA_THRESHOLD);
    if (seconds <= 0.0)
      return 0.0;
    return static_cast<double>(mAccumulatedBytes)/seconds;
  }
  double GetRate(bool* aReliable) {
    TimeDuration time = mAccumulatedTime;
    if (mIsStarted) {
      time += TimeStamp::Now() - mLastStartTime;
    }
    double seconds = time.ToSeconds();
    *aReliable = (seconds >= 3.0) ||
                 (mAccumulatedBytes >= RELIABLE_DATA_THRESHOLD);
    if (seconds <= 0.0)
      return 0.0;
    return static_cast<double>(mAccumulatedBytes)/seconds;
  }
private:
  ~MediaChannelStatistics() {}
  int64_t      mAccumulatedBytes;
  TimeDuration mAccumulatedTime;
  TimeStamp    mLastStartTime;
  bool         mIsStarted;
};


class TimestampedMediaByteRange;



class MediaByteRange {
public:
  MediaByteRange() : mStart(0), mEnd(0) {}

  MediaByteRange(int64_t aStart, int64_t aEnd)
    : mStart(aStart), mEnd(aEnd)
  {
    NS_ASSERTION(mStart < mEnd, "Range should end after start!");
  }

  explicit MediaByteRange(TimestampedMediaByteRange& aByteRange);

  bool IsNull() const {
    return mStart == 0 && mEnd == 0;
  }

  bool operator==(const MediaByteRange& aRange) const {
    return mStart == aRange.mStart && mEnd == aRange.mEnd;
  }

  
  void Clear() {
    mStart = 0;
    mEnd = 0;
  }

  bool Contains(const MediaByteRange& aByteRange) const {
    return aByteRange.mStart >= mStart && aByteRange.mEnd <= mEnd;
  }

  MediaByteRange Extents(const MediaByteRange& aByteRange) const
  {
    if (IsNull()) {
      return aByteRange;
    }
    return MediaByteRange(std::min(mStart, aByteRange.mStart),
                          std::max(mEnd, aByteRange.mEnd));
  }

  int64_t mStart, mEnd;
};



class TimestampedMediaByteRange : public MediaByteRange {
public:
  TimestampedMediaByteRange() : MediaByteRange(), mStartTime(-1) {}

  TimestampedMediaByteRange(int64_t aStart, int64_t aEnd, int64_t aStartTime)
    : MediaByteRange(aStart, aEnd), mStartTime(aStartTime)
  {
    NS_ASSERTION(aStartTime >= 0, "Start time should not be negative!");
  }

  bool IsNull() const {
    return MediaByteRange::IsNull() && mStartTime == -1;
  }

  
  void Clear() {
    MediaByteRange::Clear();
    mStartTime = -1;
  }

  
  int64_t mStartTime;
};

inline MediaByteRange::MediaByteRange(TimestampedMediaByteRange& aByteRange)
  : mStart(aByteRange.mStart), mEnd(aByteRange.mEnd)
{
  NS_ASSERTION(mStart < mEnd, "Range should end after start!");
}

class RtspMediaResource;























class MediaResource : public nsISupports
{
public:
  
  
  
  
  
  NS_DECL_THREADSAFE_ISUPPORTS

  
  
  virtual nsIURI* URI() const { return nullptr; }
  
  
  
  virtual nsresult Close() = 0;
  
  
  
  
  
  virtual void Suspend(bool aCloseImmediately) = 0;
  
  virtual void Resume() = 0;
  
  virtual already_AddRefed<nsIPrincipal> GetCurrentPrincipal() = 0;
  
  
  
  
  
  virtual bool CanClone() { return false; }
  
  
  
  virtual already_AddRefed<MediaResource> CloneData(MediaDecoder* aDecoder) = 0;
  
  virtual void RecordStatisticsTo(MediaChannelStatistics *aStatistics) { }

  
  
  virtual void SetReadMode(MediaCacheStream::ReadMode aMode) = 0;
  
  
  
  virtual void SetPlaybackRate(uint32_t aBytesPerSecond) = 0;
  
  
  
  
  
  
  virtual nsresult Read(char* aBuffer, uint32_t aCount, uint32_t* aBytes) = 0;
  
  
  
  
  virtual nsresult ReadAt(int64_t aOffset, char* aBuffer,
                          uint32_t aCount, uint32_t* aBytes) = 0;
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
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
  
  
  
  
  
  
  
  virtual bool IsSuspendedByCache() = 0;
  
  virtual bool IsSuspended() = 0;
  
  
  
  
  
  virtual nsresult ReadFromCache(char* aBuffer,
                                 int64_t aOffset,
                                 uint32_t aCount) = 0;
  
  
  
  virtual bool IsTransportSeekable() = 0;

  



  static already_AddRefed<MediaResource> Create(MediaDecoder* aDecoder, nsIChannel* aChannel);

  



  virtual nsresult Open(nsIStreamListener** aStreamListener) = 0;

  




  virtual nsresult GetCachedRanges(nsTArray<MediaByteRange>& aRanges) = 0;

  
  
  virtual void FlushCache() { }

  
  virtual void NotifyLastByteRange() { }

  
  
  
  virtual const nsCString& GetContentType() const = 0;

  
  
  virtual RtspMediaResource* GetRtspPointer() {
    return nullptr;
  }

  
  virtual bool IsRealTime() {
    return false;
  }

  virtual size_t SizeOfExcludingThis(MallocSizeOf aMallocSizeOf) const {
    return 0;
  }

  virtual size_t SizeOfIncludingThis(MallocSizeOf aMallocSizeOf) const {
    return aMallocSizeOf(this) + SizeOfExcludingThis(aMallocSizeOf);
  }

protected:
  virtual ~MediaResource() {};

private:
  void Destroy();
};

class BaseMediaResource : public MediaResource {
public:
  virtual nsIURI* URI() const { return mURI; }
  virtual void MoveLoadsToBackground();

  virtual size_t SizeOfExcludingThis(
                  MallocSizeOf aMallocSizeOf) const MOZ_OVERRIDE
  {
    
    
    
    
    
    size_t size = MediaResource::SizeOfExcludingThis(aMallocSizeOf);
    size += mContentType.SizeOfExcludingThisIfUnshared(aMallocSizeOf);

    return size;
  }

  virtual size_t SizeOfIncludingThis(
                  MallocSizeOf aMallocSizeOf) const MOZ_OVERRIDE
  {
    return aMallocSizeOf(this) + SizeOfExcludingThis(aMallocSizeOf);
  }

protected:
  BaseMediaResource(MediaDecoder* aDecoder,
                    nsIChannel* aChannel,
                    nsIURI* aURI,
                    const nsACString& aContentType) :
    mDecoder(aDecoder),
    mChannel(aChannel),
    mURI(aURI),
    mContentType(aContentType),
    mLoadInBackground(false)
  {
    MOZ_COUNT_CTOR(BaseMediaResource);
    NS_ASSERTION(!mContentType.IsEmpty(), "Must know content type");
  }
  virtual ~BaseMediaResource()
  {
    MOZ_COUNT_DTOR(BaseMediaResource);
  }

  virtual const nsCString& GetContentType() const MOZ_OVERRIDE
  {
    return mContentType;
  }

  
  
  
  void ModifyLoadFlags(nsLoadFlags aFlags);

  
  
  void DispatchBytesConsumed(int64_t aNumBytes, int64_t aOffset);

  
  
  
  MediaDecoder* mDecoder;

  
  
  nsCOMPtr<nsIChannel> mChannel;

  
  
  nsCOMPtr<nsIURI> mURI;

  
  
  
  const nsAutoCString mContentType;

  
  
  bool mLoadInBackground;
};









class ChannelMediaResource : public BaseMediaResource
{
public:
  ChannelMediaResource(MediaDecoder* aDecoder,
                       nsIChannel* aChannel,
                       nsIURI* aURI,
                       const nsACString& aContentType);
  ~ChannelMediaResource();

  
  
  
  
  
  void CacheClientNotifyDataReceived();
  
  
  
  void CacheClientNotifyDataEnded(nsresult aStatus);
  
  void CacheClientNotifyPrincipalChanged();

  
  
  
  
  
  
  
  nsresult CacheClientSeek(int64_t aOffset, bool aResume);
  
  nsresult CacheClientSuspend();
  
  nsresult CacheClientResume();

  
  
  virtual void FlushCache() MOZ_OVERRIDE;

  
  virtual void NotifyLastByteRange() MOZ_OVERRIDE;

  
  virtual nsresult Open(nsIStreamListener** aStreamListener);
  virtual nsresult Close();
  virtual void     Suspend(bool aCloseImmediately);
  virtual void     Resume();
  virtual already_AddRefed<nsIPrincipal> GetCurrentPrincipal();
  
  bool IsClosed() const { return mCacheStream.IsClosed(); }
  virtual bool     CanClone();
  virtual already_AddRefed<MediaResource> CloneData(MediaDecoder* aDecoder);
  
  
  void RecordStatisticsTo(MediaChannelStatistics *aStatistics) MOZ_OVERRIDE {
    NS_ASSERTION(aStatistics, "Statistics param cannot be null!");
    MutexAutoLock lock(mLock);
    if (!mChannelStatistics) {
      mChannelStatistics = aStatistics;
    }
  }
  virtual nsresult ReadFromCache(char* aBuffer, int64_t aOffset, uint32_t aCount);
  virtual void     EnsureCacheUpToDate();

  
  virtual void     SetReadMode(MediaCacheStream::ReadMode aMode);
  virtual void     SetPlaybackRate(uint32_t aBytesPerSecond);
  virtual nsresult Read(char* aBuffer, uint32_t aCount, uint32_t* aBytes);
  virtual nsresult ReadAt(int64_t offset, char* aBuffer,
                          uint32_t aCount, uint32_t* aBytes);
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
  virtual bool    IsSuspendedByCache();
  virtual bool    IsSuspended();
  virtual bool    IsTransportSeekable() MOZ_OVERRIDE;

  virtual size_t SizeOfExcludingThis(
                      MallocSizeOf aMallocSizeOf) const MOZ_OVERRIDE {
    
    
    
    
    
    size_t size = BaseMediaResource::SizeOfExcludingThis(aMallocSizeOf);
    size += mCacheStream.SizeOfExcludingThis(aMallocSizeOf);

    return size;
  }

  virtual size_t SizeOfIncludingThis(
                      MallocSizeOf aMallocSizeOf) const MOZ_OVERRIDE {
    return aMallocSizeOf(this) + SizeOfExcludingThis(aMallocSizeOf);
  }

  class Listener MOZ_FINAL : public nsIStreamListener,
                             public nsIInterfaceRequestor,
                             public nsIChannelEventSink
  {
    ~Listener() {}
  public:
    explicit Listener(ChannelMediaResource* aResource) : mResource(aResource) {}

    NS_DECL_ISUPPORTS
    NS_DECL_NSIREQUESTOBSERVER
    NS_DECL_NSISTREAMLISTENER
    NS_DECL_NSICHANNELEVENTSINK
    NS_DECL_NSIINTERFACEREQUESTOR

    void Revoke() { mResource = nullptr; }

  private:
    nsRefPtr<ChannelMediaResource> mResource;
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
  
  nsresult SetupChannelHeaders();
  
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
  nsRefPtr<MediaChannelStatistics> mChannelStatistics;

  
  
  
  bool mIgnoreResume;

  
  bool mSeekingForMetadata;

  
  MediaByteRange mByteRange;

  
  
  bool mIsTransportSeekable;
};

} 

#endif
