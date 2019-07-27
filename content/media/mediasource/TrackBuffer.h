





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

  
  
  
  bool AppendData(const uint8_t* aData, uint32_t aLength);
  bool EvictData(uint32_t aThreshold);
  void EvictBefore(double aTime);

  
  
  
  double Buffered(dom::TimeRanges* aRanges);

  
  
  
  bool NewDecoder();

  
  
  void DiscardDecoder();

  void Detach();

  
  bool HasInitSegment();

  
  
  bool IsReady();

  
  void LastTimestamp(int64_t& aStart, int64_t& aEnd);
  void SetLastStartTimestamp(int64_t aStart);
  void SetLastEndTimestamp(int64_t aEnd);

  
  
  bool ContainsTime(double aTime);

  void BreakCycles();

  
  void ResetDecode();

  
  
  
  const nsTArray<nsRefPtr<SourceBufferDecoder>>& Decoders();

#if defined(DEBUG)
  void Dump(const char* aPath);
#endif

private:
  ~TrackBuffer();

  
  bool QueueInitializeDecoder(nsRefPtr<SourceBufferDecoder> aDecoder);

  
  
  void InitializeDecoder(nsRefPtr<SourceBufferDecoder> aDecoder);

  
  
  
  void RegisterDecoder(nsRefPtr<SourceBufferDecoder> aDecoder);

  
  
  
  RefPtr<MediaTaskQueue> mTaskQueue;

  
  
  nsTArray<nsRefPtr<SourceBufferDecoder>> mDecoders;

  
  
  nsTArray<nsRefPtr<SourceBufferDecoder>> mInitializedDecoders;

  
  nsRefPtr<SourceBufferDecoder> mCurrentDecoder;

  nsRefPtr<MediaSourceDecoder> mParentDecoder;
  const nsCString mType;

  
  
  int64_t mLastStartTimestamp;
  int64_t mLastEndTimestamp;

  
  
  bool mHasInit;

  
  
  bool mHasAudio;
  bool mHasVideo;
};

} 
#endif 
