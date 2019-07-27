




#if !defined(OggReader_h_)
#define OggReader_h_

#include <ogg/ogg.h>
#include <theora/theoradec.h>
#ifdef MOZ_TREMOR
#include <tremor/ivorbiscodec.h>
#else
#include <vorbis/codec.h>
#endif
#include "MediaDecoderReader.h"
#include "OggCodecState.h"
#include "VideoUtils.h"
#include "mozilla/Monitor.h"

namespace mozilla {
namespace dom {
class TimeRanges;
}
}

namespace mozilla {



class OggCodecStore
{
  public:
    OggCodecStore();
    void Add(uint32_t serial, OggCodecState* codecState);
    bool Contains(uint32_t serial);
    OggCodecState* Get(uint32_t serial);
    bool IsKnownStream(uint32_t aSerial);

  private:
    
    nsClassHashtable<nsUint32HashKey, OggCodecState> mCodecStates;

    
    Monitor mMonitor;
};

class OggReader MOZ_FINAL : public MediaDecoderReader
{
public:
  explicit OggReader(AbstractMediaDecoder* aDecoder);

protected:
  ~OggReader();

public:
  virtual nsresult Init(MediaDecoderReader* aCloneDonor);
  virtual nsresult ResetDecode();
  virtual bool DecodeAudioData();

  
  
  
  virtual bool DecodeVideoFrame(bool &aKeyframeSkip,
                                  int64_t aTimeThreshold);

  virtual bool HasAudio() {
    return (mVorbisState != 0 && mVorbisState->mActive)
#ifdef MOZ_OPUS
      || (mOpusState != 0 && mOpusState->mActive)
#endif 
      ;
  }

  virtual bool HasVideo() {
    return mTheoraState != 0 && mTheoraState->mActive;
  }

  virtual nsresult ReadMetadata(MediaInfo* aInfo,
                                MetadataTags** aTags);
  virtual nsresult Seek(int64_t aTime, int64_t aStartTime, int64_t aEndTime, int64_t aCurrentTime);
  virtual nsresult GetBuffered(dom::TimeRanges* aBuffered, int64_t aStartTime);

  virtual bool IsMediaSeekable() MOZ_OVERRIDE;

private:
  
  
  
  
  VideoData* FindStartTime(int64_t& aOutStartTime);
  AudioData* DecodeToFirstAudioData();

  
  ReentrantMonitor mMonitor;

  
  
  nsresult ResetDecode(bool start);

  bool HasSkeleton() {
    return mSkeletonState != 0 && mSkeletonState->mActive;
  }

  
  
  enum IndexedSeekResult {
    SEEK_OK,          
    SEEK_INDEX_FAIL,  
    SEEK_FATAL_ERROR  
  };
  IndexedSeekResult SeekToKeyframeUsingIndex(int64_t aTarget);

  
  IndexedSeekResult RollbackIndexedSeek(int64_t aOffset);

  
  
  
  
  class SeekRange {
  public:
    SeekRange()
      : mOffsetStart(0),
        mOffsetEnd(0),
        mTimeStart(0),
        mTimeEnd(0)
    {}

    SeekRange(int64_t aOffsetStart,
              int64_t aOffsetEnd,
              int64_t aTimeStart,
              int64_t aTimeEnd)
      : mOffsetStart(aOffsetStart),
        mOffsetEnd(aOffsetEnd),
        mTimeStart(aTimeStart),
        mTimeEnd(aTimeEnd)
    {}

    bool IsNull() const {
      return mOffsetStart == 0 &&
             mOffsetEnd == 0 &&
             mTimeStart == 0 &&
             mTimeEnd == 0;
    }

    int64_t mOffsetStart, mOffsetEnd; 
    int64_t mTimeStart, mTimeEnd; 
  };

  
  
  
  
  
  
  nsresult SeekInBufferedRange(int64_t aTarget,
                               int64_t aAdjustedTarget,
                               int64_t aStartTime,
                               int64_t aEndTime,
                               const nsTArray<SeekRange>& aRanges,
                               const SeekRange& aRange);

  
  
  
  
  
  
  nsresult SeekInUnbuffered(int64_t aTarget,
                            int64_t aStartTime,
                            int64_t aEndTime,
                            const nsTArray<SeekRange>& aRanges);

  
  
  int64_t RangeEndTime(int64_t aEndOffset);

  
  
  
  
  
  
  
  int64_t RangeEndTime(int64_t aStartOffset,
                       int64_t aEndOffset,
                       bool aCachedDataOnly);

  
  
  
  int64_t RangeStartTime(int64_t aOffset);

  
  
  
  
  
  
  nsresult SeekBisection(int64_t aTarget,
                         const SeekRange& aRange,
                         uint32_t aFuzz);

  
  
  bool IsKnownStream(uint32_t aSerial);

  
  
  
  
  nsresult GetSeekRanges(nsTArray<SeekRange>& aRanges);

  
  
  
  
  
  
  
  
  SeekRange SelectSeekRange(const nsTArray<SeekRange>& aRanges,
                            int64_t aTarget,
                            int64_t aStartTime,
                            int64_t aEndTime,
                            bool aExact);
private:

  
  
  nsresult DecodeVorbis(ogg_packet* aPacket);

  
  
  nsresult DecodeOpus(ogg_packet* aPacket);

  
  
  
  
  
  nsresult DecodeTheora(ogg_packet* aPacket, int64_t aTimeThreshold);

  
  
  bool ReadOggPage(ogg_page* aPage);

  
  
  
  
  bool ReadHeaders(OggCodecState* aState);

  
  bool ReadOggChain();

  
  
  void SetChained(bool aIsChained);

  
  
  
  ogg_packet* NextOggPacket(OggCodecState* aCodecState);

  
  
  void BuildSerialList(nsTArray<uint32_t>& aTracks);

  
  void SetupTargetTheora(TheoraState* aTheoraState);
  void SetupTargetVorbis(VorbisState* aVorbisState);
  void SetupTargetOpus(OpusState* aOpusState);
  void SetupTargetSkeleton(SkeletonState* aSkeletonState);
  void SetupMediaTracksInfo(const nsTArray<uint32_t>& aSerials);

  OggCodecStore mCodecStore;

  
  TheoraState* mTheoraState;

  
  VorbisState* mVorbisState;

#ifdef MOZ_OPUS
  
  OpusState *mOpusState;

  
  
  
  bool mOpusEnabled;
#endif 

  
  SkeletonState* mSkeletonState;

  
  ogg_sync_state mOggState;

  
  
  
  
  
  
  uint32_t mVorbisSerial;
  uint32_t mOpusSerial;
  uint32_t mTheoraSerial;
  vorbis_info mVorbisInfo;
  int mOpusPreSkip;
  th_info mTheoraInfo;

  
  
  nsIntRect mPicture;

  
  
  bool mIsChained;

  
  int64_t mDecodedAudioFrames;
};

} 

#endif
