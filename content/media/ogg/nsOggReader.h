





































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
  virtual PRBool DecodeAudioData();

  
  
  
  virtual PRBool DecodeVideoFrame(PRBool &aKeyframeSkip,
                                  PRInt64 aTimeThreshold);

  virtual VideoData* FindStartTime(PRInt64 aOffset,
                                   PRInt64& aOutStartTime);

  
  
  virtual PRInt64 FindEndTime(PRInt64 aEndOffset);

  virtual PRBool HasAudio()
  {
    mozilla::MonitorAutoEnter mon(mMonitor);
    return mVorbisState != 0 && mVorbisState->mActive;
  }

  virtual PRBool HasVideo()
  {
    mozilla::MonitorAutoEnter mon(mMonitor);
    return mTheoraState != 0 && mTheoraState->mActive;
  }

  virtual nsresult ReadMetadata();
  virtual nsresult Seek(PRInt64 aTime, PRInt64 aStartTime, PRInt64 aEndTime, PRInt64 aCurrentTime);
  virtual nsresult GetBuffered(nsTimeRanges* aBuffered, PRInt64 aStartTime);

private:

  PRBool HasSkeleton()
  {
    MonitorAutoEnter mon(mMonitor);
    return mSkeletonState != 0 && mSkeletonState->mActive;
  }

  
  
  
  
  PRBool CanDecodeToTarget(PRInt64 aTarget,
                           PRInt64 aCurrentTime);

  
  
  enum IndexedSeekResult {
    SEEK_OK,          
    SEEK_INDEX_FAIL,  
    SEEK_FATAL_ERROR  
  };
  IndexedSeekResult SeekToKeyframeUsingIndex(PRInt64 aTarget);

  
  IndexedSeekResult RollbackIndexedSeek(PRInt64 aOffset);

  
  
  
  
  
  nsresult SeekInBufferedRange(PRInt64 aTarget,
                               PRInt64 aStartTime,
                               PRInt64 aEndTime,
                               const nsTArray<ByteRange>& aRanges,
                               const ByteRange& aRange);

  
  
  
  
  
  
  nsresult SeekInUnbuffered(PRInt64 aTarget,
                            PRInt64 aStartTime,
                            PRInt64 aEndTime,
                            const nsTArray<ByteRange>& aRanges);

  
  
  
  
  
  
  
  PRInt64 FindEndTime(PRInt64 aEndOffset,
                      PRBool aCachedDataOnly,
                      ogg_sync_state* aState);

  
  
  nsresult DecodeVorbis(nsTArray<nsAutoPtr<SoundData> >& aChunks,
                        ogg_packet* aPacket);

  
  nsresult DecodeTheora(nsTArray<nsAutoPtr<VideoData> >& aFrames,
                        ogg_packet* aPacket);

  
  
  PRInt64 ReadOggPage(ogg_page* aPage);

  
  
  PRBool ReadOggPacket(nsOggCodecState* aCodecState, ogg_packet* aPacket);

  
  
  
  
  
  
  nsresult SeekBisection(PRInt64 aTarget,
                         const ByteRange& aRange,
                         PRUint32 aFuzz);

  
  
  PRBool IsKnownStream(PRUint32 aSerial);

private:
  
  nsClassHashtable<nsUint32HashKey, nsOggCodecState> mCodecStates;

  
  
  
  
  nsAutoTArray<PRUint32,4> mKnownStreams;

  
  nsTheoraState* mTheoraState;

  
  nsVorbisState* mVorbisState;

  
  nsSkeletonState* mSkeletonState;

  
  ogg_sync_state mOggState;

  
  
  
  
  
  PRUint32 mVorbisSerial;
  PRUint32 mTheoraSerial;
  vorbis_info mVorbisInfo;
  th_info mTheoraInfo;

  
  
  PRInt64 mPageOffset;

  
  PRInt64 mTheoraGranulepos;

  
  PRInt64 mVorbisGranulepos;
};

#endif
