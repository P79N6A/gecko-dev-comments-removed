




#if !defined(nsOggReader_h_)
#define nsOggReader_h_

#include <ogg/ogg.h>
#include <theora/theoradec.h>
#ifdef MOZ_TREMOR
#include <tremor/ivorbiscodec.h>
#else
#include <vorbis/codec.h>
#endif
#include "nsBuiltinDecoderReader.h"
#include "nsOggCodecState.h"
#include "VideoUtils.h"

using namespace mozilla;

class nsMediaDecoder;
class nsTimeRanges;

class nsOggReader : public nsBuiltinDecoderReader
{
public:
  nsOggReader(nsBuiltinDecoder* aDecoder);
  ~nsOggReader();

  virtual nsresult Init(nsBuiltinDecoderReader* aCloneDonor);
  virtual nsresult ResetDecode();
  virtual bool DecodeAudioData();

  
  
  
  virtual bool DecodeVideoFrame(bool &aKeyframeSkip,
                                  PRInt64 aTimeThreshold);

  virtual bool HasAudio() {
    return (mVorbisState != 0 && mVorbisState->mActive) ||
           (mOpusState != 0 && mOpusState->mActive);
  }

  virtual bool HasVideo() {
    return mTheoraState != 0 && mTheoraState->mActive;
  }

  virtual nsresult ReadMetadata(nsVideoInfo* aInfo,
                                nsHTMLMediaElement::MetadataTags** aTags);
  virtual nsresult Seek(PRInt64 aTime, PRInt64 aStartTime, PRInt64 aEndTime, PRInt64 aCurrentTime);
  virtual nsresult GetBuffered(nsTimeRanges* aBuffered, PRInt64 aStartTime);

  
  virtual bool IsSeekableInBufferedRanges() {
    return true;
  }

private:

  
  
  nsresult ResetDecode(bool start);

  bool HasSkeleton() {
    return mSkeletonState != 0 && mSkeletonState->mActive;
  }

  
  
  enum IndexedSeekResult {
    SEEK_OK,          
    SEEK_INDEX_FAIL,  
    SEEK_FATAL_ERROR  
  };
  IndexedSeekResult SeekToKeyframeUsingIndex(PRInt64 aTarget);

  
  IndexedSeekResult RollbackIndexedSeek(PRInt64 aOffset);

  
  
  
  
  class SeekRange {
  public:
    SeekRange()
      : mOffsetStart(0),
        mOffsetEnd(0),
        mTimeStart(0),
        mTimeEnd(0)
    {}

    SeekRange(PRInt64 aOffsetStart,
              PRInt64 aOffsetEnd,
              PRInt64 aTimeStart,
              PRInt64 aTimeEnd)
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

    PRInt64 mOffsetStart, mOffsetEnd; 
    PRInt64 mTimeStart, mTimeEnd; 
  };

  
  
  
  
  
  
  nsresult SeekInBufferedRange(PRInt64 aTarget,
                               PRInt64 aAdjustedTarget,
                               PRInt64 aStartTime,
                               PRInt64 aEndTime,
                               const nsTArray<SeekRange>& aRanges,
                               const SeekRange& aRange);

  
  
  
  
  
  
  nsresult SeekInUnbuffered(PRInt64 aTarget,
                            PRInt64 aStartTime,
                            PRInt64 aEndTime,
                            const nsTArray<SeekRange>& aRanges);

  
  
  PRInt64 RangeEndTime(PRInt64 aEndOffset);

  
  
  
  
  
  
  
  PRInt64 RangeEndTime(PRInt64 aStartOffset,
                       PRInt64 aEndOffset,
                       bool aCachedDataOnly);

  
  
  
  PRInt64 RangeStartTime(PRInt64 aOffset);

  
  
  
  
  
  
  nsresult SeekBisection(PRInt64 aTarget,
                         const SeekRange& aRange,
                         PRUint32 aFuzz);

  
  
  bool IsKnownStream(PRUint32 aSerial);

  
  
  
  
  nsresult GetSeekRanges(nsTArray<SeekRange>& aRanges);

  
  
  
  
  
  
  
  
  SeekRange SelectSeekRange(const nsTArray<SeekRange>& aRanges,
                            PRInt64 aTarget,
                            PRInt64 aStartTime,
                            PRInt64 aEndTime,
                            bool aExact);
private:

  
  
  nsresult DecodeVorbis(ogg_packet* aPacket);

  
  
  nsresult DecodeOpus(ogg_packet* aPacket);

  
  
  
  
  
  nsresult DecodeTheora(ogg_packet* aPacket, PRInt64 aTimeThreshold);

  
  
  PRInt64 ReadOggPage(ogg_page* aPage);

  
  
  
  
  bool ReadHeaders(nsOggCodecState* aState);

  
  
  
  ogg_packet* NextOggPacket(nsOggCodecState* aCodecState);

  
  
  void BuildSerialList(nsTArray<PRUint32>& aTracks);

  
  nsClassHashtable<nsUint32HashKey, nsOggCodecState> mCodecStates;

  
  
  
  
  nsAutoTArray<PRUint32,4> mKnownStreams;

  
  nsTheoraState* mTheoraState;

  
  nsVorbisState* mVorbisState;

  
  nsOpusState *mOpusState;

  
  
  
  bool mOpusEnabled;

  
  nsSkeletonState* mSkeletonState;

  
  ogg_sync_state mOggState;

  
  
  
  
  
  
  PRUint32 mVorbisSerial;
  PRUint32 mOpusSerial;
  PRUint32 mTheoraSerial;
  vorbis_info mVorbisInfo;
  int mOpusPreSkip;
  th_info mTheoraInfo;

  
  
  PRInt64 mPageOffset;

  
  
  nsIntRect mPicture;
};

#endif
