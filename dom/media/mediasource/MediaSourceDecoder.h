





#ifndef MOZILLA_MEDIASOURCEDECODER_H_
#define MOZILLA_MEDIASOURCEDECODER_H_

#include "mozilla/Atomics.h"
#include "mozilla/Attributes.h"
#include "nsCOMPtr.h"
#include "nsError.h"
#include "MediaDecoder.h"
#include "MediaSourceReader.h"

class nsIStreamListener;

namespace mozilla {

class MediaResource;
class MediaDecoderStateMachine;
class SourceBufferDecoder;
class TrackBuffer;
enum MSRangeRemovalAction : uint8_t;
class MediaSourceDemuxer;

namespace dom {

class HTMLMediaElement;
class MediaSource;

} 

class MediaSourceDecoder : public MediaDecoder
{
public:
  explicit MediaSourceDecoder(dom::HTMLMediaElement* aElement);

  virtual MediaDecoder* Clone() override;
  virtual MediaDecoderStateMachine* CreateStateMachine() override;
  virtual nsresult Load(nsIStreamListener**, MediaDecoder*) override;
  virtual media::TimeIntervals GetSeekable() override;
  media::TimeIntervals GetBuffered() override;

  virtual void Shutdown() override;

  static already_AddRefed<MediaResource> CreateResource(nsIPrincipal* aPrincipal = nullptr);

  void AttachMediaSource(dom::MediaSource* aMediaSource);
  void DetachMediaSource();

  already_AddRefed<SourceBufferDecoder> CreateSubDecoder(const nsACString& aType,
                                                         int64_t aTimestampOffset );
  void AddTrackBuffer(TrackBuffer* aTrackBuffer);
  void RemoveTrackBuffer(TrackBuffer* aTrackBuffer);
  void OnTrackBufferConfigured(TrackBuffer* aTrackBuffer, const MediaInfo& aInfo);

  void Ended(bool aEnded);
  bool IsExpectingMoreData() override;

  
  virtual double GetDuration() override;

  void SetInitialDuration(int64_t aDuration);
  void SetMediaSourceDuration(double aDuration, MSRangeRemovalAction aAction);
  double GetMediaSourceDuration();

  
  
  void NotifyTimeRangesChanged();

  
  
  void PrepareReaderInitialization();

#ifdef MOZ_EME
  virtual nsresult SetCDMProxy(CDMProxy* aProxy) override;
#endif

  MediaSourceReader* GetReader()
  {
    MOZ_ASSERT(!mIsUsingFormatReader);
    return static_cast<MediaSourceReader*>(mReader.get());
  }
  MediaSourceDemuxer* GetDemuxer()
  {
    return mDemuxer;
  }

  
  
  bool IsActiveReader(MediaDecoderReader* aReader);

  
  
  void GetMozDebugReaderData(nsAString& aString);

private:
  void DoSetMediaSourceDuration(double aDuration);

  
  
  
  dom::MediaSource* mMediaSource;
  nsRefPtr<MediaDecoderReader> mReader;
  bool mIsUsingFormatReader;
  nsRefPtr<MediaSourceDemuxer> mDemuxer;

  Atomic<bool> mEnded;
};

} 

#endif 
