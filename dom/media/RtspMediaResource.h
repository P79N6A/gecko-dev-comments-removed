




#if !defined(RtspMediaResource_h_)
#define RtspMediaResource_h_

#include "MediaResource.h"
#include "mozilla/Monitor.h"
#include "nsITimer.h"
#include "VideoUtils.h"

namespace mozilla {

class RtspTrackBuffer;






















































class RtspMediaResource : public BaseMediaResource
{
public:
  RtspMediaResource(MediaDecoder* aDecoder, nsIChannel* aChannel, nsIURI* aURI,
                    const nsACString& aContentType);
  virtual ~RtspMediaResource();

  

  
  
  virtual RtspMediaResource* GetRtspPointer() override final {
    return this;
  }

  
  
  
  
  
  nsIStreamingProtocolController* GetMediaStreamController() {
    return mMediaStreamController;
  }

  
  
  virtual bool IsRealTime() override {
    return !mHasTimestamp;
  }

  
  
  void SetSuspend(bool aIsSuspend);

  

  
  
  
  
  
  
  nsresult ReadFrameFromTrack(uint8_t* aBuffer, uint32_t aBufferSize,
                              uint32_t aTrackIdx, uint32_t& aBytes,
                              uint64_t& aTime, uint32_t& aFrameSize);

  
  nsresult SeekTime(int64_t aOffset);

  
  
  
  void EnablePlayoutDelay();
  void DisablePlayoutDelay();

  
  virtual nsresult ReadAt(int64_t aOffset, char* aBuffer,
                          uint32_t aCount, uint32_t* aBytes)  override{
    return NS_ERROR_FAILURE;
  }
  
  virtual void     SetReadMode(MediaCacheStream::ReadMode aMode) override {}
  
  virtual void     SetPlaybackRate(uint32_t aBytesPerSecond) override {}
  
  virtual nsresult Read(char* aBuffer, uint32_t aCount, uint32_t* aBytes)
  override {
    return NS_OK;
  }
  
  virtual nsresult Seek(int32_t aWhence, int64_t aOffset) override {
    return NS_OK;
  }
  
  virtual int64_t  Tell() override { return 0; }

  
  virtual void    Pin() override {}
  virtual void    Unpin() override {}

  virtual bool    IsSuspendedByCache() override { return mIsSuspend; }

  virtual bool    IsSuspended() override { return false; }
  virtual bool    IsTransportSeekable() override { return true; }
  
  virtual double  GetDownloadRate(bool* aIsReliable) override { *aIsReliable = false; return 0; }

  virtual int64_t GetLength() override {
    if (mIsLiveStream) {
      return -1;
    }
    return 0;
  }

  
  virtual int64_t GetNextCachedData(int64_t aOffset) override { return 0; }
  
  virtual int64_t GetCachedDataEnd(int64_t aOffset) override { return 0; }
  
  virtual bool    IsDataCachedToEndOfResource(int64_t aOffset) override {
    return false;
  }
  
  nsresult GetCachedRanges(nsTArray<MediaByteRange>& aRanges) override {
    return NS_ERROR_FAILURE;
  }

  

  virtual nsresult Open(nsIStreamListener** aStreamListener) override;
  virtual nsresult Close() override;
  virtual void     Suspend(bool aCloseImmediately) override;
  virtual void     Resume() override;
  virtual already_AddRefed<nsIPrincipal> GetCurrentPrincipal() override;
  virtual bool     CanClone() override {
    return false;
  }
  virtual already_AddRefed<MediaResource> CloneData(MediaDecoder* aDecoder)
  override {
    return nullptr;
  }
  
  virtual nsresult ReadFromCache(char* aBuffer, int64_t aOffset,
                                 uint32_t aCount) override {
    return NS_ERROR_FAILURE;
  }

  virtual size_t SizeOfExcludingThis(
                      MallocSizeOf aMallocSizeOf) const override;

  virtual size_t SizeOfIncludingThis(
                      MallocSizeOf aMallocSizeOf) const override {
    return aMallocSizeOf(this) + SizeOfExcludingThis(aMallocSizeOf);
  }

  
  
  
  
  
  class Listener final : public nsIInterfaceRequestor,
                         public nsIStreamingProtocolListener
  {
    ~Listener() {}
  public:
    explicit Listener(RtspMediaResource* aResource) : mResource(aResource) {}

    NS_DECL_ISUPPORTS
    NS_DECL_NSIINTERFACEREQUESTOR
    NS_DECL_NSISTREAMINGPROTOCOLLISTENER

    void Revoke();

  private:
    nsRefPtr<RtspMediaResource> mResource;
  };
  friend class Listener;

protected:
  
  
  nsresult OnMediaDataAvailable(uint8_t aIndex, const nsACString& aData,
                                uint32_t aLength, uint32_t aOffset,
                                nsIStreamingProtocolMetaData* aMeta);
  nsresult OnConnected(uint8_t aIndex, nsIStreamingProtocolMetaData* aMeta);
  nsresult OnDisconnected(uint8_t aIndex, nsresult aReason);

  nsRefPtr<Listener> mListener;

private:
  
  void NotifySuspend(bool aIsSuspend);
  bool IsVideoEnabled();
  bool IsVideo(uint8_t tracks, nsIStreamingProtocolMetaData *meta);
  
  nsCOMPtr<nsIStreamingProtocolController> mMediaStreamController;
  nsTArray<nsAutoPtr<RtspTrackBuffer>> mTrackBuffer;

  
  
  bool mIsConnected;
  
  bool mIsLiveStream;
  
  bool mHasTimestamp;
  
  bool mIsSuspend;
};

} 

#endif

