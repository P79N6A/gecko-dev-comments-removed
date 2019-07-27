




#if !defined(RtspMediaResource_h_)
#define RtspMediaResource_h_

#include "MediaResource.h"

namespace mozilla {

class RtspTrackBuffer;






















































class RtspMediaResource : public BaseMediaResource
{
public:
  RtspMediaResource(MediaDecoder* aDecoder, nsIChannel* aChannel, nsIURI* aURI,
                    const nsACString& aContentType);
  virtual ~RtspMediaResource();

  

  
  
  virtual RtspMediaResource* GetRtspPointer() MOZ_OVERRIDE MOZ_FINAL {
    return this;
  }

  
  
  
  
  
  nsIStreamingProtocolController* GetMediaStreamController() {
    return mMediaStreamController;
  }

  virtual bool IsRealTime() MOZ_OVERRIDE {
    return mRealTime;
  }

  
  
  void SetSuspend(bool aIsSuspend);

  

  
  
  
  
  
  
  nsresult ReadFrameFromTrack(uint8_t* aBuffer, uint32_t aBufferSize,
                              uint32_t aTrackIdx, uint32_t& aBytes,
                              uint64_t& aTime, uint32_t& aFrameSize);

  
  nsresult SeekTime(int64_t aOffset);

  
  virtual nsresult ReadAt(int64_t aOffset, char* aBuffer,
                          uint32_t aCount, uint32_t* aBytes)  MOZ_OVERRIDE{
    return NS_ERROR_FAILURE;
  }
  
  virtual void     SetReadMode(MediaCacheStream::ReadMode aMode) MOZ_OVERRIDE {}
  
  virtual void     SetPlaybackRate(uint32_t aBytesPerSecond) MOZ_OVERRIDE {}
  
  virtual nsresult Read(char* aBuffer, uint32_t aCount, uint32_t* aBytes)
  MOZ_OVERRIDE {
    return NS_OK;
  }
  
  virtual nsresult Seek(int32_t aWhence, int64_t aOffset) MOZ_OVERRIDE {
    return NS_OK;
  }
  
  virtual void     StartSeekingForMetadata() MOZ_OVERRIDE {}
  
  virtual void     EndSeekingForMetadata() MOZ_OVERRIDE {}
  
  virtual int64_t  Tell() MOZ_OVERRIDE { return 0; }

  
  virtual void    Pin() MOZ_OVERRIDE {}
  virtual void    Unpin() MOZ_OVERRIDE {}

  virtual bool    IsSuspendedByCache() MOZ_OVERRIDE { return mIsSuspend; }

  virtual bool    IsSuspended() MOZ_OVERRIDE { return false; }
  virtual bool    IsTransportSeekable() MOZ_OVERRIDE { return true; }
  
  virtual double  GetDownloadRate(bool* aIsReliable) MOZ_OVERRIDE { *aIsReliable = false; return 0; }

  virtual int64_t GetLength() MOZ_OVERRIDE {
    if (mRealTime) {
      return -1;
    }
    return 0;
  }

  
  virtual int64_t GetNextCachedData(int64_t aOffset) MOZ_OVERRIDE { return 0; }
  
  virtual int64_t GetCachedDataEnd(int64_t aOffset) MOZ_OVERRIDE { return 0; }
  
  virtual bool    IsDataCachedToEndOfResource(int64_t aOffset) MOZ_OVERRIDE {
    return false;
  }
  
  nsresult GetCachedRanges(nsTArray<MediaByteRange>& aRanges) MOZ_OVERRIDE {
    return NS_ERROR_FAILURE;
  }

  

  virtual nsresult Open(nsIStreamListener** aStreamListener) MOZ_OVERRIDE;
  virtual nsresult Close() MOZ_OVERRIDE;
  virtual void     Suspend(bool aCloseImmediately) MOZ_OVERRIDE;
  virtual void     Resume() MOZ_OVERRIDE;
  virtual already_AddRefed<nsIPrincipal> GetCurrentPrincipal() MOZ_OVERRIDE;
  virtual bool     CanClone() MOZ_OVERRIDE {
    return false;
  }
  virtual already_AddRefed<MediaResource> CloneData(MediaDecoder* aDecoder)
  MOZ_OVERRIDE {
    return nullptr;
  }
  
  virtual nsresult ReadFromCache(char* aBuffer, int64_t aOffset,
                                 uint32_t aCount) MOZ_OVERRIDE {
    return NS_ERROR_FAILURE;
  }

  virtual size_t SizeOfExcludingThis(
                      MallocSizeOf aMallocSizeOf) const MOZ_OVERRIDE;

  virtual size_t SizeOfIncludingThis(
                      MallocSizeOf aMallocSizeOf) const MOZ_OVERRIDE {
    return aMallocSizeOf(this) + SizeOfExcludingThis(aMallocSizeOf);
  }

  
  
  
  
  
  class Listener MOZ_FINAL : public nsIInterfaceRequestor,
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
  
  bool mRealTime;
  
  bool mIsSuspend;
};

} 

#endif

