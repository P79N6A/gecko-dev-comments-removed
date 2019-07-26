





#ifndef MOZILLA_MEDIASOURCEDECODER_H_
#define MOZILLA_MEDIASOURCEDECODER_H_

#include "MediaCache.h"
#include "MediaDecoder.h"
#include "mozilla/Attributes.h"
#include "mozilla/ReentrantMonitor.h"
#include "nsCOMPtr.h"
#include "nsError.h"
#include "nsStringGlue.h"
#include "nsTArray.h"

class nsIStreamListener;

namespace mozilla {

class MediaResource;
class MediaDecoderReader;
class MediaDecoderStateMachine;
class SubBufferDecoder;
class MediaSourceReader;

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

  static already_AddRefed<MediaResource> CreateResource();

  void AttachMediaSource(dom::MediaSource* aMediaSource);
  void DetachMediaSource();

  already_AddRefed<SubBufferDecoder> CreateSubDecoder(const nsACString& aType);

  nsresult EnqueueDecoderInitialization();

private:
  
  
  
  dom::MediaSource* mMediaSource;
};

} 

#endif 
