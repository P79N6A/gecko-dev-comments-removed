





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

class ContainerParser;
class MediaSourceDecoder;

namespace dom {

class TimeRanges;

} 

class TrackBuffer MOZ_FINAL {
public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(TrackBuffer);

  TrackBuffer(MediaSourceDecoder* aParentDecoder, const nsACString& aType);

  void Shutdown();

  
  
  
  bool AppendData(const uint8_t* aData, uint32_t aLength);
  bool EvictData(uint32_t aThreshold);
  void EvictBefore(double aTime);

  
  
  
  double Buffered(dom::TimeRanges* aRanges);

  
  
  void DiscardDecoder();

  void Detach();

  
  bool HasInitSegment();

  
  
  bool IsReady();

  
  
  bool ContainsTime(int64_t aTime);

  void BreakCycles();

  
  void ResetDecode();

  
  
  
  const nsTArray<nsRefPtr<SourceBufferDecoder>>& Decoders();

#if defined(DEBUG)
  void Dump(const char* aPath);
#endif

private:
  ~TrackBuffer();

  
  
  
  bool NewDecoder();

  
  
  bool AppendDataToCurrentResource(const uint8_t* aData, uint32_t aLength);

  
  bool QueueInitializeDecoder(nsRefPtr<SourceBufferDecoder> aDecoder);

  
  
  void InitializeDecoder(nsRefPtr<SourceBufferDecoder> aDecoder);

  
  
  
  
  bool RegisterDecoder(nsRefPtr<SourceBufferDecoder> aDecoder);

  
  
  bool ValidateTrackFormats(const MediaInfo& aInfo);

  
  
  
  
  void RemoveDecoder(nsRefPtr<SourceBufferDecoder> aDecoder);

  nsAutoPtr<ContainerParser> mParser;

  
  
  
  RefPtr<MediaTaskQueue> mTaskQueue;

  
  
  nsTArray<nsRefPtr<SourceBufferDecoder>> mDecoders;

  
  
  nsTArray<nsRefPtr<SourceBufferDecoder>> mInitializedDecoders;

  
  nsRefPtr<SourceBufferDecoder> mCurrentDecoder;

  nsRefPtr<MediaSourceDecoder> mParentDecoder;
  const nsCString mType;

  
  
  int64_t mLastStartTimestamp;
  int64_t mLastEndTimestamp;

  
  
  MediaInfo mInfo;
};

} 
#endif 
