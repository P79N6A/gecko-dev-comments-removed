




#if !defined(OggCodecState_h_)
#define OggCodecState_h_

#include <ogg/ogg.h>
#include <theora/theoradec.h>
#ifdef MOZ_TREMOR
#include <tremor/ivorbiscodec.h>
#else
#include <vorbis/codec.h>
#endif
#ifdef MOZ_OPUS
#include <opus/opus.h>
#include "opus/opus_multistream.h"

#include "mozilla/dom/HTMLMediaElement.h"
#include "MediaDecoderStateMachine.h"
#include "MediaDecoderReader.h"
#endif
#include <nsAutoRef.h>
#include <nsDeque.h>
#include <nsTArray.h>
#include <nsClassHashtable.h>
#include "VideoUtils.h"

#include <stdint.h>



#define VALIDATE_VORBIS_SAMPLE_CALCULATION
#ifdef  VALIDATE_VORBIS_SAMPLE_CALCULATION
#include <map>
#endif

#include "OpusParser.h"

namespace mozilla {


class OggPacketDeallocator : public nsDequeFunctor {
  virtual void* operator() (void* aPacket) {
    ogg_packet* p = static_cast<ogg_packet*>(aPacket);
    delete [] p->packet;
    delete p;
    return nullptr;
  }
};











class OggPacketQueue : private nsDeque {
public:
  OggPacketQueue() : nsDeque(new OggPacketDeallocator()) {}
  ~OggPacketQueue() { Erase(); }
  bool IsEmpty() { return nsDeque::GetSize() == 0; }
  void Append(ogg_packet* aPacket);
  ogg_packet* PopFront() { return static_cast<ogg_packet*>(nsDeque::PopFront()); }
  ogg_packet* PeekFront() { return static_cast<ogg_packet*>(nsDeque::PeekFront()); }
  void PushFront(ogg_packet* aPacket) { nsDeque::PushFront(aPacket); }
  void Erase() { nsDeque::Erase(); }
};



class OggCodecState {
public:
  typedef mozilla::MetadataTags MetadataTags;
  
  enum CodecType {
    TYPE_VORBIS=0,
    TYPE_THEORA=1,
    TYPE_OPUS=2,
    TYPE_SKELETON=3,
    TYPE_UNKNOWN=4
  };

  virtual ~OggCodecState();
  
  
  
  static OggCodecState* Create(ogg_page* aPage);
  
  virtual CodecType GetType() { return TYPE_UNKNOWN; }

  
  
  
  
  
  virtual bool DecodeHeader(ogg_packet* aPacket) {
    return (mDoneReadingHeaders = true);
  }

  
  virtual MetadataTags* GetTags() {
    return nullptr;
  }

  
  virtual int64_t Time(int64_t granulepos) { return -1; }

  
  virtual int64_t StartTime(int64_t granulepos) { return -1; }

  
  virtual bool Init();

  
  
  bool DoneReadingHeaders() { return mDoneReadingHeaders; }

  
  
  void Deactivate() {
    mActive = false;
    mDoneReadingHeaders = true;
    Reset();
  }

  
  virtual nsresult Reset();

  
  
  
  
  
  
  virtual bool IsHeader(ogg_packet* aPacket) { return false; }

  
  
  
  
  
  ogg_packet* PacketOut();

  
  
  static void ReleasePacket(ogg_packet* aPacket);

  
  
  
  
  
  
  virtual nsresult PageIn(ogg_page* aPage);

  
  uint64_t mPacketCount;

  
  uint32_t mSerial;

  
  ogg_stream_state mState;

  
  
  OggPacketQueue mPackets;

  
  bool mActive;
  
  
  bool mDoneReadingHeaders;

protected:
  
  
  
  OggCodecState(ogg_page* aBosPage, bool aActive);

  
  void ClearUnstamped();

  
  
  
  
  
  
  
  nsresult PacketOutUntilGranulepos(bool& aFoundGranulepos);

  
  
  nsTArray<ogg_packet*> mUnstamped;

  
  static bool IsValidVorbisTagName(nsCString& aName);

  
  
  
  static bool AddVorbisComment(MetadataTags* aTags,
                        const char* aComment,
                        uint32_t aLength);
};

class VorbisState : public OggCodecState {
public:
  explicit VorbisState(ogg_page* aBosPage);
  virtual ~VorbisState();

  CodecType GetType() { return TYPE_VORBIS; }
  bool DecodeHeader(ogg_packet* aPacket);
  int64_t Time(int64_t granulepos);
  bool Init();
  nsresult Reset();
  bool IsHeader(ogg_packet* aPacket);
  nsresult PageIn(ogg_page* aPage); 

  
  MetadataTags* GetTags();

  
  static int64_t Time(vorbis_info* aInfo, int64_t aGranulePos); 

  vorbis_info mInfo;
  vorbis_comment mComment;
  vorbis_dsp_state mDsp;
  vorbis_block mBlock;

private:

  
  
  nsresult ReconstructVorbisGranulepos();

  
  
  
  
  long mPrevVorbisBlockSize;

  
  
  
  int64_t mGranulepos;

#ifdef VALIDATE_VORBIS_SAMPLE_CALCULATION
  
  
  
  std::map<ogg_packet*, long> mVorbisPacketSamples;
#endif

  
  
  
  void RecordVorbisPacketSamples(ogg_packet* aPacket, long aSamples);

  
  
  
  void AssertHasRecordedPacketSamples(ogg_packet* aPacket);

public:
  
  
  
  void ValidateVorbisPacketSamples(ogg_packet* aPacket, long aSamples);

};



int TheoraVersion(th_info* info,
                  unsigned char maj,
                  unsigned char min,
                  unsigned char sub);

class TheoraState : public OggCodecState {
public:
  explicit TheoraState(ogg_page* aBosPage);
  virtual ~TheoraState();

  CodecType GetType() { return TYPE_THEORA; }
  bool DecodeHeader(ogg_packet* aPacket);
  int64_t Time(int64_t granulepos);
  int64_t StartTime(int64_t granulepos);
  bool Init();
  bool IsHeader(ogg_packet* aPacket);
  nsresult PageIn(ogg_page* aPage); 

  
  
  int64_t MaxKeyframeOffset();

  
  static int64_t Time(th_info* aInfo, int64_t aGranulePos); 

  th_info mInfo;
  th_comment mComment;
  th_setup_info *mSetup;
  th_dec_ctx* mCtx;

  float mPixelAspectRatio;

private:

  
  
  
  
  
  void ReconstructTheoraGranulepos();

};

class OpusState : public OggCodecState {
#ifdef MOZ_OPUS
public:
  explicit OpusState(ogg_page* aBosPage);
  virtual ~OpusState();

  CodecType GetType() { return TYPE_OPUS; }
  bool DecodeHeader(ogg_packet* aPacket);
  int64_t Time(int64_t aGranulepos);
  bool Init();
  nsresult Reset();
  nsresult Reset(bool aStart);
  bool IsHeader(ogg_packet* aPacket);
  nsresult PageIn(ogg_page* aPage);

  
  static int64_t Time(int aPreSkip, int64_t aGranulepos);

  
  int mRate;        
  int mChannels;    
  uint16_t mPreSkip; 
#ifdef MOZ_SAMPLE_TYPE_FLOAT32
  float mGain;      
#else
  int32_t mGain_Q16; 
#endif

  nsAutoPtr<OpusParser> mParser;
  OpusMSDecoder *mDecoder;

  int mSkip;        
  
  
  int64_t mPrevPacketGranulepos;

  
  MetadataTags* GetTags();

private:

  
  
  
  
  
  bool ReconstructOpusGranulepos();

  
  
  
  int64_t mPrevPageGranulepos;

#endif 
};



#define SKELETON_VERSION(major, minor) (((major)<<16)|(minor))

class SkeletonState : public OggCodecState {
public:
  explicit SkeletonState(ogg_page* aBosPage);
  ~SkeletonState();
  CodecType GetType() { return TYPE_SKELETON; }
  bool DecodeHeader(ogg_packet* aPacket);
  int64_t Time(int64_t granulepos) { return -1; }
  bool Init() { return true; }
  bool IsHeader(ogg_packet* aPacket) { return true; }

  
  
  bool IsPresentable(int64_t aTime) { return aTime >= mPresentationTime; }

  
  
  class nsKeyPoint {
  public:
    nsKeyPoint()
      : mOffset(INT64_MAX),
        mTime(INT64_MAX) {}

    nsKeyPoint(int64_t aOffset, int64_t aTime)
      : mOffset(aOffset),
        mTime(aTime) {}

    
    int64_t mOffset;

    
    int64_t mTime;

    bool IsNull() {
      return mOffset == INT64_MAX &&
             mTime == INT64_MAX;
    }
  };

  
  
  class nsSeekTarget {
  public:
    nsSeekTarget() : mSerial(0) {}
    nsKeyPoint mKeyPoint;
    uint32_t mSerial;
    bool IsNull() {
      return mKeyPoint.IsNull() &&
             mSerial == 0;
    }
  };

  
  
  
  nsresult IndexedSeekTarget(int64_t aTarget,
                             nsTArray<uint32_t>& aTracks,
                             nsSeekTarget& aResult);

  bool HasIndex() const {
    return mIndex.Count() > 0;
  }

  
  
  
  
  nsresult GetDuration(const nsTArray<uint32_t>& aTracks, int64_t& aDuration);

private:

  
  bool DecodeIndex(ogg_packet* aPacket);

  
  
  nsresult IndexedSeekTargetForTrack(uint32_t aSerialno,
                                     int64_t aTarget,
                                     nsKeyPoint& aResult);

  
  uint32_t mVersion;

  
  int64_t mPresentationTime;

  
  int64_t mLength;

  
  
  class nsKeyFrameIndex {
  public:

    nsKeyFrameIndex(int64_t aStartTime, int64_t aEndTime) 
      : mStartTime(aStartTime),
        mEndTime(aEndTime)
    {
      MOZ_COUNT_CTOR(nsKeyFrameIndex);
    }

    ~nsKeyFrameIndex() {
      MOZ_COUNT_DTOR(nsKeyFrameIndex);
    }

    void Add(int64_t aOffset, int64_t aTimeMs) {
      mKeyPoints.AppendElement(nsKeyPoint(aOffset, aTimeMs));
    }

    const nsKeyPoint& Get(uint32_t aIndex) const {
      return mKeyPoints[aIndex];
    }

    uint32_t Length() const {
      return mKeyPoints.Length();
    }

    
    const int64_t mStartTime;

    
    const int64_t mEndTime;

  private:
    nsTArray<nsKeyPoint> mKeyPoints;
  };

  
  nsClassHashtable<nsUint32HashKey, nsKeyFrameIndex> mIndex;
};

} 



template <>
class nsAutoRefTraits<ogg_packet> : public nsPointerRefTraits<ogg_packet>
{
public:
  static void Release(ogg_packet* aPacket) {
    mozilla::OggCodecState::ReleasePacket(aPacket);
  }
};


#endif
