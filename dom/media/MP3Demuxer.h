



#ifndef MP3_DEMUXER_H_
#define MP3_DEMUXER_H_

#include "mozilla/Attributes.h"
#include "MediaDataDemuxer.h"
#include "MediaResource.h"

namespace mozilla {
namespace mp3 {

class MP3TrackDemuxer;

class MP3Demuxer : public MediaDataDemuxer {
public:
  
  explicit MP3Demuxer(MediaResource* aSource);
  nsRefPtr<InitPromise> Init() override;
  already_AddRefed<MediaDataDemuxer> Clone() const override;
  bool HasTrackType(TrackInfo::TrackType aType) const override;
  uint32_t GetNumberTracks(TrackInfo::TrackType aType) const override;
  already_AddRefed<MediaTrackDemuxer> GetTrackDemuxer(
      TrackInfo::TrackType aType, uint32_t aTrackNumber) override;
  bool IsSeekable() const override;
  void NotifyDataArrived(uint32_t aLength, int64_t aOffset) override;
  void NotifyDataRemoved() override;

private:
  
  bool InitInternal();

  nsRefPtr<MediaResource> mSource;
  nsRefPtr<MP3TrackDemuxer> mTrackDemuxer;
};





class ID3Parser {
public:
  
  class ID3Header {
  public:
    
    static const int SIZE = 10;

    
    ID3Header();

    
    void Reset();

    
    uint8_t MajorVersion() const;
    uint8_t MinorVersion() const;

    
    uint8_t Flags() const;

    
    uint32_t Size() const;

    
    
    bool IsValid(int aPos) const;

    
    bool IsValid() const;

    
    
    bool ParseNext(uint8_t c);

  private:
    
    
    bool Update(uint8_t c);

    
    uint8_t mRaw[SIZE];

    
    
    
    uint32_t mSize;

    
    
    int mPos;
  };

  
  const ID3Header& Header() const;

  
  
  const uint8_t* Parse(const uint8_t* aBeg, const uint8_t* aEnd);

  
  void Reset();

private:
  
  ID3Header mHeader;
};


















class FrameParser {
public:
  
  class FrameHeader {
  public:
    
    static const int SIZE = 4;

    
    FrameHeader();

    
    uint8_t Sync1() const;
    uint8_t Sync2() const;
    uint8_t RawVersion() const;
    uint8_t RawLayer() const;
    uint8_t RawProtection() const;
    uint8_t RawBitrate() const;
    uint8_t RawSampleRate() const;
    uint8_t Padding() const;
    uint8_t Private() const;
    uint8_t RawChannelMode() const;

    
    int32_t SampleRate() const;

    
    int32_t Channels() const;

    
    int32_t SamplesPerFrame() const;

    
    int32_t SlotSize() const;

    
    int32_t Bitrate() const;

    
    int32_t Layer() const;

    
    
    bool IsValid(const int aPos) const;

    
    bool IsValid() const;

    
    void Reset();

    
    
    bool ParseNext(const uint8_t c);

  private:
    
    
    bool Update(const uint8_t c);

    
    uint8_t mRaw[SIZE];

    
    
    int mPos;
  };

  
  
  class VBRHeader {
  public:
    enum VBRHeaderType {
      NONE,
      XING,
      VBRI
    };

    
    VBRHeader();

    
    VBRHeaderType Type() const;

    
    int64_t NumFrames() const;

    
    
    bool Parse(const uint8_t* aBeg, const uint8_t* aEnd);

  private:
    
    
    bool ParseXing(const uint8_t* aBeg, const uint8_t* aEnd);

    
    
    bool ParseVBRI(const uint8_t* aBeg, const uint8_t* aEnd);

    
    int64_t mNumFrames;

    
    VBRHeaderType mType;
  };

  
  class Frame {
  public:
    
    int32_t Length() const;

    
    const FrameHeader& Header() const;

    
    void Reset();

    
    
    bool ParseNext(uint8_t c);

  private:
    
    FrameHeader mHeader;
  };

  
  FrameParser();

  
  const Frame& CurrentFrame() const;

#ifdef ENABLE_TESTS
  
  const Frame& PrevFrame() const;
#endif

  
  const Frame& FirstFrame() const;

  
  const ID3Parser::ID3Header& ID3Header() const;

  
  const VBRHeader& VBRInfo() const;

  
  void Reset();

  
  
  
  
  void FinishParsing();

  
  
  const uint8_t* Parse(const uint8_t* aBeg, const uint8_t* aEnd);

  
  
  bool ParseVBRHeader(const uint8_t* aBeg, const uint8_t* aEnd);

private:
  
  ID3Parser mID3Parser;

  
  VBRHeader mVBRHeader;

  
  
  Frame mFirstFrame;
  Frame mFrame;
#ifdef ENABLE_TESTS
  Frame mPrevFrame;
#endif
};



class MP3TrackDemuxer : public MediaTrackDemuxer {
public:
  
  explicit MP3TrackDemuxer(MediaResource* aSource);

  
  
  bool Init();

  
  int64_t StreamLength() const;

  
  media::TimeUnit Duration() const;

  
  
  media::TimeUnit Duration(int64_t aNumFrames) const;

#ifdef ENABLE_TESTS
  const FrameParser::Frame& LastFrame() const;
  nsRefPtr<MediaRawData> DemuxSample();
#endif

  const ID3Parser::ID3Header& ID3Header() const;
  const FrameParser::VBRHeader& VBRInfo() const;

  
  UniquePtr<TrackInfo> GetInfo() const override;
  nsRefPtr<SeekPromise> Seek(media::TimeUnit aTime) override;
  nsRefPtr<SamplesPromise> GetSamples(int32_t aNumSamples = 1) override;
  void Reset() override;
  nsRefPtr<SkipAccessPointPromise> SkipToNextRandomAccessPoint(
    media::TimeUnit aTimeThreshold) override;
  int64_t GetResourceOffset() const override;
  media::TimeIntervals GetBuffered() override;
  int64_t GetEvictionOffset(media::TimeUnit aTime) override;

private:
  
  ~MP3TrackDemuxer() {}

  
  media::TimeUnit FastSeek(media::TimeUnit aTime);

  
  media::TimeUnit ScanUntil(media::TimeUnit aTime);

  
  MediaByteRange FindNextFrame();

  
  bool SkipNextFrame(const MediaByteRange& aRange);

  
  already_AddRefed<MediaRawData> GetNextFrame(const MediaByteRange& aRange);

  
  void UpdateState(const MediaByteRange& aRange);

  
  
  int32_t Read(uint8_t* aBuffer, int64_t aOffset, int32_t aSize);

  
  double AverageFrameLength() const;

  
  nsRefPtr<MediaResource> mSource;

  
  FrameParser mParser;

  
  int64_t mOffset;

  
  int64_t mFirstFrameOffset;

  
  uint64_t mNumParsedFrames;

  
  int64_t mFrameIndex;

  
  uint64_t mTotalFrameLen;

  
  int32_t mSamplesPerFrame;

  
  int32_t mSamplesPerSecond;

  
  int32_t mChannels;

  
  UniquePtr<AudioInfo> mInfo;
};

}  
}  

#endif
