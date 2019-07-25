





































#if !defined(nsOggReader_h_)
#define nsOggReader_h_

#include <ogg/ogg.h>
#include <theora/theoradec.h>
#include <vorbis/codec.h>
#include "nsBuiltinDecoderReader.h"
#include "nsOggCodecState.h"

class nsMediaDecoder;
class nsHTMLTimeRanges;

class nsOggReader : public nsBuiltinDecoderReader
{
public:
  nsOggReader(nsBuiltinDecoder* aDecoder);
  ~nsOggReader();

  virtual nsresult Init();
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
  virtual nsresult Seek(PRInt64 aTime, PRInt64 aStartTime, PRInt64 aEndTime);

  
  virtual nsresult GetBuffered(nsHTMLTimeRanges* aBuffered);

private:
  
  
  nsresult DecodeVorbis(nsTArray<SoundData*>& aChunks,
                        ogg_packet* aPacket);

  
  nsresult DecodeTheora(nsTArray<VideoData*>& aFrames,
                        ogg_packet* aPacket);

  
  
  PRInt64 ReadOggPage(ogg_page* aPage);

  
  
  PRBool ReadOggPacket(nsOggCodecState* aCodecState, ogg_packet* aPacket);

  
  
  
  
  
  
  nsresult SeekBisection(PRInt64 aTarget,
                         const ByteRange& aRange,
                         PRUint32 aFuzz);

private:
  
  nsClassHashtable<nsUint32HashKey, nsOggCodecState> mCodecStates;

  
  nsTheoraState* mTheoraState;

  
  nsVorbisState* mVorbisState;

  
  ogg_sync_state mOggState;

  
  
  PRInt64 mPageOffset;

  
  PRInt64 mTheoraGranulepos;

  
  PRInt64 mVorbisGranulepos;
};

#endif
