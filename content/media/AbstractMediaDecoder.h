





#ifndef AbstractMediaDecoder_h_
#define AbstractMediaDecoder_h_

#include "mozilla/Attributes.h"
#include "MediaInfo.h"
#include "nsISupports.h"
#include "nsDataHashtable.h"
#include "nsThreadUtils.h"

namespace mozilla
{

namespace layers
{
  class ImageContainer;
}
class MediaResource;
class ReentrantMonitor;
class VideoFrameContainer;
class TimedMetadata;
class MediaDecoderOwner;
class CDMProxy;

typedef nsDataHashtable<nsCStringHashKey, nsCString> MetadataTags;

static inline bool IsCurrentThread(nsIThread* aThread) {
  return NS_GetCurrentThread() == aThread;
}





class AbstractMediaDecoder : public nsISupports
{
public:
  
  
  virtual ReentrantMonitor& GetReentrantMonitor() = 0;

  
  virtual bool IsShutdown() const = 0;

  virtual bool OnStateMachineThread() const = 0;

  virtual bool OnDecodeThread() const = 0;

  
  
  virtual MediaResource* GetResource() const = 0;

  
  
  virtual void NotifyBytesConsumed(int64_t aBytes, int64_t aOffset) = 0;

  
  
  virtual void NotifyDecodedFrames(uint32_t aParsed, uint32_t aDecoded) = 0;

  
  
  
  virtual int64_t GetEndMediaTime() const = 0;

  
  virtual int64_t GetMediaDuration() = 0;

  
  virtual void SetMediaDuration(int64_t aDuration) = 0;

  
  
  
  virtual void UpdateEstimatedMediaDuration(int64_t aDuration) = 0;

  
  virtual void SetMediaSeekable(bool aMediaSeekable) = 0;

  virtual VideoFrameContainer* GetVideoFrameContainer() = 0;
  virtual mozilla::layers::ImageContainer* GetImageContainer() = 0;

  
  virtual bool IsTransportSeekable() = 0;

  
  virtual bool IsMediaSeekable() = 0;

  virtual void MetadataLoaded(MediaInfo* aInfo, MetadataTags* aTags) = 0;
  virtual void QueueMetadata(int64_t aTime, MediaInfo* aInfo, MetadataTags* aTags) = 0;

  virtual void RemoveMediaTracks() = 0;

  
  virtual void SetMediaEndTime(int64_t aTime) = 0;

  
  
  
  
  virtual void UpdatePlaybackPosition(int64_t aTime) = 0;

  
  
  virtual void OnReadMetadataCompleted() = 0;

  
  
  virtual MediaDecoderOwner* GetOwner() = 0;

  
  
  virtual void NotifyWaitingForResourcesStatusChanged() = 0;

  
  virtual void SetCanOffloadAudio(bool aCanOffloadAudio) {}

  
  virtual void SetElementVisibility(bool aIsVisible) {}

  
  
  
  class AutoNotifyDecoded {
  public:
    AutoNotifyDecoded(AbstractMediaDecoder* aDecoder, uint32_t& aParsed, uint32_t& aDecoded)
      : mDecoder(aDecoder), mParsed(aParsed), mDecoded(aDecoded) {}
    ~AutoNotifyDecoded() {
      mDecoder->NotifyDecodedFrames(mParsed, mDecoded);
    }
  private:
    AbstractMediaDecoder* mDecoder;
    uint32_t& mParsed;
    uint32_t& mDecoded;
  };

  virtual nsresult SetCDMProxy(CDMProxy* aProxy) { return NS_ERROR_NOT_IMPLEMENTED; }
  virtual CDMProxy* GetCDMProxy() { return nullptr; }
};

class MetadataEventRunner : public nsRunnable
{
  private:
    nsRefPtr<AbstractMediaDecoder> mDecoder;
  public:
    MetadataEventRunner(AbstractMediaDecoder* aDecoder, MediaInfo* aInfo, MetadataTags* aTags)
          : mDecoder(aDecoder),
            mInfo(aInfo),
            mTags(aTags)
  {}

  NS_IMETHOD Run() MOZ_OVERRIDE
  {
    mDecoder->MetadataLoaded(mInfo, mTags);
    return NS_OK;
  }

  
  MediaInfo* mInfo;

  
  MetadataTags* mTags;
};

class RemoveMediaTracksEventRunner : public nsRunnable
{
public:
  RemoveMediaTracksEventRunner(AbstractMediaDecoder* aDecoder)
    : mDecoder(aDecoder)
  {}

  NS_IMETHOD Run() MOZ_OVERRIDE
  {
    MOZ_ASSERT(NS_IsMainThread());

    mDecoder->RemoveMediaTracks();
    return NS_OK;
  }

private:
  nsRefPtr<AbstractMediaDecoder> mDecoder;
};

}

#endif

