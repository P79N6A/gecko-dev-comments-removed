





#ifndef AbstractMediaDecoder_h_
#define AbstractMediaDecoder_h_

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

typedef nsDataHashtable<nsCStringHashKey, nsCString> MetadataTags;





class AbstractMediaDecoder : public nsISupports
{
public:
  
  
  virtual ReentrantMonitor& GetReentrantMonitor() = 0;

  
  virtual bool IsShutdown() const = 0;

  virtual bool OnStateMachineThread() const = 0;

  virtual bool OnDecodeThread() const = 0;

  
  
  virtual MediaResource* GetResource() const = 0;

  
  
  virtual void NotifyBytesConsumed(int64_t aBytes) = 0;

  
  
  virtual void NotifyDecodedFrames(uint32_t aParsed, uint32_t aDecoded) = 0;

  
  
  
  virtual int64_t GetEndMediaTime() const = 0;

  
  virtual int64_t GetMediaDuration() = 0;

  
  virtual void SetMediaDuration(int64_t aDuration) = 0;

  
  virtual void SetMediaSeekable(bool aMediaSeekable) = 0;

  
  virtual void SetTransportSeekable(bool aTransportSeekable) = 0;

  virtual VideoFrameContainer* GetVideoFrameContainer() = 0;
  virtual mozilla::layers::ImageContainer* GetImageContainer() = 0;

  
  virtual bool IsTransportSeekable() = 0;

  
  virtual bool IsMediaSeekable() = 0;

  virtual void MetadataLoaded(int aChannels, int aRate, bool aHasAudio, MetadataTags* aTags) = 0;
  virtual void QueueMetadata(int64_t aTime, int aChannels, int aRate, bool aHasAudio, MetadataTags* aTags) = 0;

  
  virtual void SetMediaEndTime(int64_t aTime) = 0;

  
  
  
  
  virtual void UpdatePlaybackPosition(int64_t aTime) = 0;

  
  
  virtual void OnReadMetadataCompleted() = 0;

  
  
  
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
};

class AudioMetadataEventRunner : public nsRunnable
{
  private:
    nsRefPtr<AbstractMediaDecoder> mDecoder;
  public:
    AudioMetadataEventRunner(AbstractMediaDecoder* aDecoder, int aChannels, int aRate, bool aHasAudio, MetadataTags* aTags)
      : mDecoder(aDecoder),
        mChannels(aChannels),
        mRate(aRate),
        mHasAudio(aHasAudio),
        mTags(aTags)
  {}

  NS_IMETHOD Run()
  {
    mDecoder->MetadataLoaded(mChannels, mRate, mHasAudio, mTags);
    return NS_OK;
  }

  int mChannels;
  int mRate;
  bool mHasAudio;
  MetadataTags* mTags;
};


}

#endif

