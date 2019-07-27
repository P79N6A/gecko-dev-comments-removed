





#ifndef MOZILLA_MEDIASOURCEDECODER_H_
#define MOZILLA_MEDIASOURCEDECODER_H_

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

namespace dom {

class HTMLMediaElement;
class MediaSource;

} 

class MediaSourceDecoder : public MediaDecoder
{
public:
  explicit MediaSourceDecoder(dom::HTMLMediaElement* aElement);

  virtual MediaDecoder* Clone() MOZ_OVERRIDE;
  virtual MediaDecoderStateMachine* CreateStateMachine() MOZ_OVERRIDE;
  virtual nsresult Load(nsIStreamListener**, MediaDecoder*) MOZ_OVERRIDE;
  virtual nsresult GetSeekable(dom::TimeRanges* aSeekable) MOZ_OVERRIDE;

  virtual void Shutdown() MOZ_OVERRIDE;

  static already_AddRefed<MediaResource> CreateResource(nsIPrincipal* aPrincipal = nullptr);

  void AttachMediaSource(dom::MediaSource* aMediaSource);
  void DetachMediaSource();

  already_AddRefed<SourceBufferDecoder> CreateSubDecoder(const nsACString& aType,
                                                         int64_t aTimestampOffset );
  void AddTrackBuffer(TrackBuffer* aTrackBuffer);
  void RemoveTrackBuffer(TrackBuffer* aTrackBuffer);
  void OnTrackBufferConfigured(TrackBuffer* aTrackBuffer, const MediaInfo& aInfo);

  void Ended();
  bool IsExpectingMoreData() MOZ_OVERRIDE;

  
  virtual double GetDuration() MOZ_OVERRIDE;

  void SetInitialDuration(int64_t aDuration);
  void SetMediaSourceDuration(double aDuration, MSRangeRemovalAction aAction);
  double GetMediaSourceDuration();
  void DurationChanged(double aOldDuration, double aNewDuration);

  
  
  void NotifyTimeRangesChanged();

  
  
  void PrepareReaderInitialization();

#ifdef MOZ_EME
  virtual nsresult SetCDMProxy(CDMProxy* aProxy) MOZ_OVERRIDE;
#endif

  MediaSourceReader* GetReader() { return mReader; }

  
  
  bool IsActiveReader(MediaDecoderReader* aReader);

  
  
  void GetMozDebugReaderData(nsAString& aString);

private:
  void DoSetMediaSourceDuration(double aDuration);
  void ScheduleDurationChange(double aOldDuration,
                              double aNewDuration,
                              MSRangeRemovalAction aAction);

  
  
  
  dom::MediaSource* mMediaSource;
  nsRefPtr<MediaSourceReader> mReader;

  
  double mMediaSourceDuration;
};

} 

#endif 
