



#ifndef MP3_TRACK_DEMUXER_H_
#define MP3_TRACK_DEMUXER_H_

#include "mozilla/Attributes.h"
#include "demuxer/TrackDemuxer.h"

namespace mp4_demuxer {





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



class MP3Demuxer : public mozilla::TrackDemuxer {
public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(MP3Demuxer);

  
  explicit MP3Demuxer(Stream* aSource);

  
  
  
  bool Init();

  
  int64_t StreamLength() const;

  
  
  int64_t Duration() const;

  
  
  int64_t Duration(int64_t aNumFrames) const;

#ifdef ENABLE_TESTS
  const FrameParser::Frame& LastFrame() const;
#endif
  const ID3Parser::ID3Header& ID3Header() const;
  const FrameParser::VBRHeader& VBRInfo() const;

  
  virtual void Seek(Microseconds aTime) override;
  virtual already_AddRefed<mozilla::MediaRawData> DemuxSample() override;
  virtual Microseconds GetNextKeyframeTime() override;

  void UpdateConfig(mozilla::AudioInfo& aConfig) {
    aConfig.mRate = mSamplesPerSecond;
    aConfig.mChannels = mChannels;
    aConfig.mBitDepth = 16;
    aConfig.mMimeType = "audio/mpeg";
  }

private:
  
  ~MP3Demuxer() {}

  
  void FastSeek(Microseconds aTime);

  
  void SlowSeek(Microseconds aTime);

  
  already_AddRefed<mozilla::MediaRawData> GetNext();

  
  
  uint32_t Read(uint8_t* aBuffer, uint32_t aOffset, uint32_t aSize);

  
  double AverageFrameLength() const;

  
  nsRefPtr<Stream> mSource;

  
  FrameParser mParser;

  
  uint64_t mOffset;

  
  uint64_t mFirstFrameOffset;

  
  int64_t mStreamLength;

  
  int64_t mNumParsedFrames;
  int64_t mFrameIndex;

  
  int64_t mTotalFrameLen;

  
  int32_t mSamplesPerFrame;

  
  int32_t mSamplesPerSecond;

  
  int32_t mChannels;
};

}

#endif
