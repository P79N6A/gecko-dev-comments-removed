





#ifndef MOZILLA_TRACKBUFFER_H_
#define MOZILLA_TRACKBUFFER_H_

#include "SourceBufferDecoder.h"
#include "mozilla/Assertions.h"
#include "mozilla/Attributes.h"
#include "mozilla/mozalloc.h"
#include "nsCOMPtr.h"
#include "nsString.h"
#include "nscore.h"

namespace mozilla {

class MediaSourceDecoder;

namespace dom {

class TimeRanges;

} 

class TrackBuffer MOZ_FINAL {
public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(TrackBuffer);

  TrackBuffer(MediaSourceDecoder* aParentDecoder, const nsACString& aType);

  void Shutdown();

  
  
  
  void AppendData(const uint8_t* aData, uint32_t aLength);
  bool EvictData(uint32_t aThreshold);
  void EvictBefore(double aTime);

  
  
  
  double Buffered(dom::TimeRanges* aRanges);

  
  
  
  bool NewDecoder();

  
  
  void DiscardDecoder();

  void Detach();

  
  
  bool HasInitSegment();

  
  void LastTimestamp(double& aStart, double& aEnd);
  void SetLastStartTimestamp(double aStart);
  void SetLastEndTimestamp(double aEnd);

  
  
  bool ContainsTime(double aTime);

  
  
  bool HasAudio();
  bool HasVideo();

  void BreakCycles();

  
  void ResetDecode();

  
  
  
  const nsTArray<nsRefPtr<SourceBufferDecoder>>& Decoders();

private:
  ~TrackBuffer();

  
  bool QueueInitializeDecoder(nsRefPtr<SourceBufferDecoder> aDecoder);

  
  
  void InitializeDecoder(nsRefPtr<SourceBufferDecoder> aDecoder);

  
  
  
  void RegisterDecoder(nsRefPtr<SourceBufferDecoder> aDecoder);

  
  
  
  RefPtr<MediaTaskQueue> mTaskQueue;

  
  
  nsTArray<nsRefPtr<SourceBufferDecoder>> mDecoders;

  
  nsRefPtr<SourceBufferDecoder> mCurrentDecoder;

  nsRefPtr<MediaSourceDecoder> mParentDecoder;
  const nsCString mType;

  
  
  double mLastStartTimestamp;
  double mLastEndTimestamp;

  
  
  bool mHasAudio;
  bool mHasVideo;
};

} 
#endif 
