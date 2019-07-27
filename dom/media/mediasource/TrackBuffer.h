





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

class TrackBuffer final : public SourceBufferContentManager {
public:
  TrackBuffer(MediaSourceDecoder* aParentDecoder, const nsACString& aType);

  nsRefPtr<ShutdownPromise> Shutdown();

  bool AppendData(MediaLargeByteBuffer* aData, TimeUnit aTimestampOffset) override;

  
  
  
  nsRefPtr<AppendPromise> BufferAppend() override;

  
  
  
  
  
  EvictDataResult EvictData(TimeUnit aPlaybackTime, uint32_t aThreshold, TimeUnit* aBufferStartTime) override;

  
  
  void EvictBefore(TimeUnit aTime) override;

  nsRefPtr<RangeRemovalPromise> RangeRemoval(TimeUnit aStart, TimeUnit aEnd) override;

  void AbortAppendData() override;

  int64_t GetSize() override;

  void ResetParserState() override;

  
  
  media::TimeIntervals Buffered() override;

  void Ended() override
  {
    EndCurrentDecoder();
  }

  void Detach() override;

  
  
  void DiscardCurrentDecoder();
  
  void EndCurrentDecoder();

  
  bool HasInitSegment();

  
  
  bool IsReady();

  bool IsWaitingOnCDMResource();

  
  
  bool ContainsTime(int64_t aTime, int64_t aTolerance);

  void BreakCycles();

  
  
  
  const nsTArray<nsRefPtr<SourceBufferDecoder>>& Decoders();

  
  
  bool HasOnlyIncompleteMedia();

#ifdef MOZ_EME
  nsresult SetCDMProxy(CDMProxy* aProxy);
#endif

#if defined(DEBUG)
  void Dump(const char* aPath) override;
#endif

private:
  friend class DecodersToInitialize;
  friend class MetadataRecipient;
  virtual ~TrackBuffer();

  
  
  
  
  
  already_AddRefed<SourceBufferDecoder> NewDecoder(TimeUnit aTimestampOffset);

  
  
  bool AppendDataToCurrentResource(MediaLargeByteBuffer* aData,
                                   uint32_t aDuration );
  
  void NotifyTimeRangesChanged();

  
  bool QueueInitializeDecoder(SourceBufferDecoder* aDecoder);

  
  
  void InitializeDecoder(SourceBufferDecoder* aDecoder);
  
  
  
  
  void CompleteInitializeDecoder(SourceBufferDecoder* aDecoder);

  
  
  
  
  bool RegisterDecoder(SourceBufferDecoder* aDecoder);

  
  
  bool ValidateTrackFormats(const MediaInfo& aInfo);

  
  
  
  
  void RemoveDecoder(SourceBufferDecoder* aDecoder);

  
  void RemoveEmptyDecoders(nsTArray<SourceBufferDecoder*>& aDecoders);

  void OnMetadataRead(MetadataHolder* aMetadata,
                      SourceBufferDecoder* aDecoder,
                      bool aWasEnded);

  void OnMetadataNotRead(ReadMetadataFailureReason aReason,
                         SourceBufferDecoder* aDecoder);

  nsAutoPtr<ContainerParser> mParser;
  nsRefPtr<MediaLargeByteBuffer> mInputBuffer;

  
  
  
  RefPtr<MediaTaskQueue> mTaskQueue;

  
  
  nsTArray<nsRefPtr<SourceBufferDecoder>> mDecoders;

  
  
  
  nsTArray<nsRefPtr<SourceBufferDecoder>> mShutdownDecoders;

  
  
  nsTArray<nsRefPtr<SourceBufferDecoder>> mInitializedDecoders;

  
  
  nsRefPtr<SourceBufferDecoder> mCurrentDecoder;

  nsRefPtr<MediaSourceDecoder> mParentDecoder;
  const nsCString mType;

  
  
  int64_t mLastStartTimestamp;
  Maybe<int64_t> mLastEndTimestamp;
  void AdjustDecodersTimestampOffset(TimeUnit aOffset);

  
  TimeUnit mLastTimestampOffset;
  TimeUnit mTimestampOffset;
  TimeUnit mAdjustedTimestamp;

  
  bool mIsWaitingOnCDM;

  
  
  MediaInfo mInfo;

  void ContinueShutdown();
  MediaPromiseHolder<ShutdownPromise> mShutdownPromise;
  bool mDecoderPerSegment;
  bool mShutdown;

  MediaPromiseHolder<AppendPromise> mInitializationPromise;
  
  MediaPromiseRequestHolder<MediaDecoderReader::MetadataPromise> mMetadataRequest;
};

} 
#endif 
