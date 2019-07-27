





#ifndef MOZILLA_TRACKBUFFER_H_
#define MOZILLA_TRACKBUFFER_H_

#include "SourceBuffer.h"
#include "SourceBufferDecoder.h"
#include "mozilla/Assertions.h"
#include "mozilla/Attributes.h"
#include "mozilla/mozalloc.h"
#include "mozilla/Maybe.h"
#include "nsCOMPtr.h"
#include "nsString.h"
#include "nscore.h"
#include "TimeUnits.h"

namespace mozilla {

class ContainerParser;
class MediaSourceDecoder;
class MediaLargeByteBuffer;

namespace dom {

class TimeRanges;

} 

class TrackBuffer final {
public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(TrackBuffer);

  TrackBuffer(MediaSourceDecoder* aParentDecoder, const nsACString& aType);

  nsRefPtr<ShutdownPromise> Shutdown();

  
  
  
  nsRefPtr<TrackBufferAppendPromise> AppendData(MediaLargeByteBuffer* aData,
                                                int64_t aTimestampOffset );

  
  
  
  
  
  
  bool EvictData(double aPlaybackTime, uint32_t aThreshold, double* aBufferStartTime);

  
  
  void EvictBefore(double aTime);

  
  
  
  double Buffered(dom::TimeRanges* aRanges);

  
  
  void DiscardCurrentDecoder();
  
  void EndCurrentDecoder();

  void Detach();

  
  bool HasInitSegment();

  
  
  bool IsReady();

  bool IsWaitingOnCDMResource();

  
  
  bool ContainsTime(int64_t aTime, int64_t aTolerance);

  void BreakCycles();

  
  
  
  void ResetParserState();

  
  
  
  const nsTArray<nsRefPtr<SourceBufferDecoder>>& Decoders();

  
  
  
  
  
  bool RangeRemoval(mozilla::media::Microseconds aStart,
                    mozilla::media::Microseconds aEnd);

  
  void AbortAppendData();

  
  int64_t GetSize();

  
  
  bool HasOnlyIncompleteMedia();

#ifdef MOZ_EME
  nsresult SetCDMProxy(CDMProxy* aProxy);
#endif

#if defined(DEBUG)
  void Dump(const char* aPath);
#endif

private:
  friend class DecodersToInitialize;
  ~TrackBuffer();

  
  
  
  
  
  already_AddRefed<SourceBufferDecoder> NewDecoder(int64_t aTimestampOffset );

  
  
  bool AppendDataToCurrentResource(MediaLargeByteBuffer* aData,
                                   uint32_t aDuration );

  
  bool QueueInitializeDecoder(SourceBufferDecoder* aDecoder);

  
  
  void InitializeDecoder(SourceBufferDecoder* aDecoder);
  
  
  
  
  void CompleteInitializeDecoder(SourceBufferDecoder* aDecoder);

  
  
  
  
  bool RegisterDecoder(SourceBufferDecoder* aDecoder);

  
  
  bool ValidateTrackFormats(const MediaInfo& aInfo);

  
  
  
  
  void RemoveDecoder(SourceBufferDecoder* aDecoder);

  
  void RemoveEmptyDecoders(nsTArray<SourceBufferDecoder*>& aDecoders);

  nsAutoPtr<ContainerParser> mParser;

  
  
  
  RefPtr<MediaTaskQueue> mTaskQueue;

  
  
  nsTArray<nsRefPtr<SourceBufferDecoder>> mDecoders;

  
  
  
  nsTArray<nsRefPtr<SourceBufferDecoder>> mShutdownDecoders;

  
  
  nsTArray<nsRefPtr<SourceBufferDecoder>> mInitializedDecoders;

  
  
  nsRefPtr<SourceBufferDecoder> mCurrentDecoder;

  nsRefPtr<MediaSourceDecoder> mParentDecoder;
  const nsCString mType;

  
  
  int64_t mLastStartTimestamp;
  Maybe<int64_t> mLastEndTimestamp;
  void AdjustDecodersTimestampOffset(int32_t aOffset);

  
  int64_t mLastTimestampOffset;
  int64_t mAdjustedTimestamp;

  
  bool mIsWaitingOnCDM;

  
  
  MediaInfo mInfo;

  void ContinueShutdown();
  MediaPromiseHolder<ShutdownPromise> mShutdownPromise;
  bool mDecoderPerSegment;
  bool mShutdown;

  MediaPromiseHolder<TrackBufferAppendPromise> mInitializationPromise;

};

} 
#endif 
