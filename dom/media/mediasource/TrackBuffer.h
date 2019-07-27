





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
#include <map>

namespace mozilla {

class ContainerParser;
class MediaSourceDecoder;
class MediaByteBuffer;

class TrackBuffer final : public SourceBufferContentManager {
public:
  TrackBuffer(MediaSourceDecoder* aParentDecoder, const nsACString& aType);

  nsRefPtr<ShutdownPromise> Shutdown();

  bool AppendData(MediaByteBuffer* aData, TimeUnit aTimestampOffset) override;

  
  
  
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

  
  media::TimeIntervals GetBuffered(SourceBufferDecoder* aDecoder);

#ifdef MOZ_EME
  nsresult SetCDMProxy(CDMProxy* aProxy);
#endif

#if defined(DEBUG)
  void Dump(const char* aPath) override;
#endif

  typedef std::map<SourceBufferDecoder*, media::TimeIntervals> DecoderBufferedMap;

private:
  friend class DecodersToInitialize;
  friend class MetadataRecipient;
  virtual ~TrackBuffer();

  
  
  
  
  
  already_AddRefed<SourceBufferDecoder> NewDecoder(media::TimeUnit aTimestampOffset);

  
  
  int64_t AppendDataToCurrentResource(MediaByteBuffer* aData,
                                   uint32_t aDuration );
  
  void NotifyTimeRangesChanged();
  
  void NotifyReaderDataRemoved(MediaDecoderReader* aReader);

  typedef MediaPromise<bool, nsresult,  true> BufferedRangesUpdatedPromise;
  nsRefPtr<BufferedRangesUpdatedPromise> UpdateBufferedRanges(Interval<int64_t> aByteRange, bool aNotifyParent);

  
  bool QueueInitializeDecoder(SourceBufferDecoder* aDecoder);

  
  
  void InitializeDecoder(SourceBufferDecoder* aDecoder);
  
  
  
  
  void CompleteInitializeDecoder(SourceBufferDecoder* aDecoder);

  
  
  
  
  bool RegisterDecoder(SourceBufferDecoder* aDecoder);

  
  
  bool ValidateTrackFormats(const MediaInfo& aInfo);

  
  
  
  
  void RemoveDecoder(SourceBufferDecoder* aDecoder);

  
  void RemoveEmptyDecoders(const nsTArray<nsRefPtr<SourceBufferDecoder>>& aDecoders);

  void OnMetadataRead(MetadataHolder* aMetadata,
                      SourceBufferDecoder* aDecoder,
                      bool aWasEnded);

  void OnMetadataNotRead(ReadMetadataFailureReason aReason,
                         SourceBufferDecoder* aDecoder);

  nsAutoPtr<ContainerParser> mParser;
  nsRefPtr<MediaByteBuffer> mInputBuffer;

  
  
  
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

  
  media::TimeUnit mLastTimestampOffset;
  media::TimeUnit mTimestampOffset;
  media::TimeUnit mAdjustedTimestamp;

  
  bool mIsWaitingOnCDM;

  
  
  MediaInfo mInfo;

  void ContinueShutdown();
  MediaPromiseHolder<ShutdownPromise> mShutdownPromise;
  bool mDecoderPerSegment;
  bool mShutdown;

  MediaPromiseHolder<AppendPromise> mInitializationPromise;
  
  MediaPromiseRequestHolder<MediaDecoderReader::MetadataPromise> mMetadataRequest;

  MediaPromiseHolder<RangeRemovalPromise> mRangeRemovalPromise;

  Interval<int64_t> mLastAppendRange;

  
  media::TimeIntervals mBufferedRanges;

  DecoderBufferedMap mReadersBuffered;
};

} 
#endif 
