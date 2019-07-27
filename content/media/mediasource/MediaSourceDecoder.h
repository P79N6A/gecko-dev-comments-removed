





#ifndef MOZILLA_MEDIASOURCEDECODER_H_
#define MOZILLA_MEDIASOURCEDECODER_H_

#include "mozilla/Attributes.h"
#include "nsCOMPtr.h"
#include "nsError.h"
#include "MediaDecoder.h"

class nsIStreamListener;

namespace mozilla {

class MediaResource;
class MediaDecoderStateMachine;
class MediaSourceReader;
class SourceBufferDecoder;

namespace dom {

class HTMLMediaElement;
class MediaSource;

} 

class MediaSourceDecoder : public MediaDecoder
{
public:
  MediaSourceDecoder(dom::HTMLMediaElement* aElement);

  virtual MediaDecoder* Clone() MOZ_OVERRIDE;
  virtual MediaDecoderStateMachine* CreateStateMachine() MOZ_OVERRIDE;
  virtual nsresult Load(nsIStreamListener**, MediaDecoder*) MOZ_OVERRIDE;
  virtual nsresult GetSeekable(dom::TimeRanges* aSeekable) MOZ_OVERRIDE;

  virtual void Shutdown() MOZ_OVERRIDE;

  static already_AddRefed<MediaResource> CreateResource();

  void AttachMediaSource(dom::MediaSource* aMediaSource);
  void DetachMediaSource();

  already_AddRefed<SourceBufferDecoder> CreateSubDecoder(const nsACString& aType);

  void SetMediaSourceDuration(double aDuration);

  
  void WaitForData();

  
  void NotifyGotData();

private:
  
  
  
  dom::MediaSource* mMediaSource;
  nsRefPtr<MediaSourceReader> mReader;
};

} 

#endif 
