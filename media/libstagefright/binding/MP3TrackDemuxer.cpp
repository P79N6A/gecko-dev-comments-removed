





#include "mp4_demuxer/MP3TrackDemuxer.h"
#include "mozilla/Assertions.h"
#include "mozilla/Endian.h"
#include "VideoUtils.h"

namespace mp4_demuxer {



MP3Demuxer::MP3Demuxer(Stream* aSource)
  : mSource(aSource),
    mOffset(0),
    mFirstFrameOffset(0),
    mStreamLength(-1),
    mNumParsedFrames(0),
    mFrameIndex(0),
    mTotalFrameLen(0),
    mSamplesPerFrame(0),
    mSamplesPerSecond(0),
    mChannels(0)
{
}

bool
MP3Demuxer::Init() {
  if (!mSource->Length(&mStreamLength)) {
    
    mStreamLength = -1;
  }
  return true;
}

#ifdef ENABLE_TESTS
const FrameParser::Frame&
MP3Demuxer::LastFrame() const {
  return mParser.PrevFrame();
}
#endif

const ID3Parser::ID3Header&
MP3Demuxer::ID3Header() const {
  return mParser.ID3Header();
}

const FrameParser::VBRHeader&
MP3Demuxer::VBRInfo() const {
  return mParser.VBRInfo();
}

void
MP3Demuxer::Seek(Microseconds aTime) {
  SlowSeek(aTime);
}

void
MP3Demuxer::FastSeek(Microseconds aTime) {
  if (!aTime) {
    
    mOffset = mFirstFrameOffset;
    mFrameIndex = 0;
    mParser.FinishParsing();
    return;
  }

  if (!mSamplesPerFrame || !mNumParsedFrames) {
    return;
  }

  const int64_t numFrames = static_cast<double>(aTime) / USECS_PER_S *
                            mSamplesPerSecond / mSamplesPerFrame;
  mOffset = mFirstFrameOffset + numFrames * AverageFrameLength();
  mFrameIndex = numFrames;
  mParser.FinishParsing();
}

void
MP3Demuxer::SlowSeek(Microseconds aTime) {
  if (!aTime) {
    FastSeek(aTime);
    return;
  }

  if (Duration(mFrameIndex) > aTime) {
    FastSeek(aTime);
  }

  nsRefPtr<MediaRawData> frameData(GetNext());
  while (frameData && Duration(mFrameIndex + 1) < aTime) {
    frameData = GetNext();
  }
}


already_AddRefed<MediaRawData>
MP3Demuxer::DemuxSample() {
  nsRefPtr<MediaRawData> sample(GetNext());
  if (!sample) {
    return nullptr;
  }
  return sample.forget();
}

Microseconds
MP3Demuxer::GetNextKeyframeTime() {
  return -1;
}

int64_t
MP3Demuxer::StreamLength() const {
  return mStreamLength;
}

int64_t
MP3Demuxer::Duration() const {
  if (!mNumParsedFrames) {
    return -1;
  }

  
  int64_t numFrames = mParser.VBRInfo().NumFrames();
  if (numFrames < 0) {
    if (mStreamLength < 0) {
      
      return -1;
    }
    numFrames = (mStreamLength - mFirstFrameOffset) / AverageFrameLength();
  }
  return Duration(numFrames);
}

int64_t
MP3Demuxer::Duration(int64_t aNumFrames) const {
  if (!mSamplesPerSecond) {
    return -1;
  }

  const double usPerFrame = USECS_PER_S * mSamplesPerFrame / mSamplesPerSecond;
  return aNumFrames * usPerFrame;
}

already_AddRefed<mozilla::MediaRawData>
MP3Demuxer::GetNext() {
  static const int BUFFER_SIZE = 4096;

  uint8_t buffer[BUFFER_SIZE];
  uint32_t read = 0;
  const uint8_t* frameBeg = nullptr;
  const uint8_t* bufferEnd = nullptr;

  while (frameBeg == bufferEnd &&
         (read = Read(buffer, mOffset, BUFFER_SIZE)) > 0) {
    MOZ_ASSERT(mOffset + read > mOffset);
    mOffset += read;
    bufferEnd = buffer + read;
    frameBeg = mParser.Parse(buffer, bufferEnd);
  }

  if (frameBeg == bufferEnd || !mParser.CurrentFrame().Length()) {
    return nullptr;
  }

  
  const int32_t frameLen = mParser.CurrentFrame().Length();
  nsRefPtr<MediaRawData> frame = new MediaRawData();
  frame->mOffset = mOffset - (bufferEnd - frameBeg) + 1;

  nsAutoPtr<MediaRawDataWriter> frameWriter(frame->CreateWriter());
  if (!frameWriter->SetSize(frameLen)) {
    return nullptr;
  }

  read = Read(frameWriter->mData, frame->mOffset, frame->mSize);

  if (read != frame->mSize) {
    return nullptr;
  }

  
  if (mTotalFrameLen + frameLen < 0) {
    
    
    mTotalFrameLen /= 2;
    mNumParsedFrames /= 2;
  }

  
  mOffset = frame->mOffset + frame->mSize;
  MOZ_ASSERT(mOffset > frame->mOffset);

  mTotalFrameLen += frameLen;
  mSamplesPerFrame = mParser.CurrentFrame().Header().SamplesPerFrame();
  mSamplesPerSecond = mParser.CurrentFrame().Header().SampleRate();
  mChannels = mParser.CurrentFrame().Header().Channels();
  ++mNumParsedFrames;
  ++mFrameIndex;
  MOZ_ASSERT(mFrameIndex > 0);

  frame->mTime = Duration(mFrameIndex - 1);
  frame->mDuration = Duration(1);

  if (mNumParsedFrames == 1) {
    
    
    mParser.ParseVBRHeader(frame->mData, frame->mData + frame->mSize);
    mFirstFrameOffset = frame->mOffset;
  }

  
  mParser.FinishParsing();
  return frame.forget();
}

uint32_t
MP3Demuxer::Read(uint8_t* aBuffer, uint32_t aOffset, uint32_t aSize) {
  size_t read = 0;
  if (!mSource->ReadAt(aOffset, aBuffer, aSize, &read)) {
    read = 0;
  }
  return read;
}

double
MP3Demuxer::AverageFrameLength() const {
  if (!mNumParsedFrames) {
    return 0.0;
  }
  return static_cast<double>(mTotalFrameLen) / mNumParsedFrames;
}



namespace frame_header {

static const int SYNC1 = 0;
static const int SYNC2_VERSION_LAYER_PROTECTION = 1;
static const int BITRATE_SAMPLERATE_PADDING_PRIVATE = 2;
static const int CHANNELMODE_MODEEXT_COPY_ORIG_EMPH = 3;
}

FrameParser::FrameParser()
{
}

void
FrameParser::Reset() {
  mID3Parser.Reset();
  mFirstFrame.Reset();
  mFrame.Reset();
}

void
FrameParser::FinishParsing() {
  if (!mID3Parser.Header().IsValid()) {
    
    mID3Parser.Reset();
  }
#ifdef ENABLE_TESTS
  mPrevFrame = mFrame;
#endif
  mFrame.Reset();
}

const FrameParser::Frame&
FrameParser::CurrentFrame() const {
  return mFrame;
}

#ifdef ENABLE_TESTS
const FrameParser::Frame&
FrameParser::PrevFrame() const {
  return mPrevFrame;
}
#endif

const FrameParser::Frame&
FrameParser::FirstFrame() const {
  return mFirstFrame;
}

const ID3Parser::ID3Header&
FrameParser::ID3Header() const {
  return mID3Parser.Header();
}

const FrameParser::VBRHeader&
FrameParser::VBRInfo() const {
  return mVBRHeader;
}

const uint8_t*
FrameParser::Parse(const uint8_t* aBeg, const uint8_t* aEnd) {
  if (!aBeg || !aEnd || aBeg >= aEnd) {
    return aEnd;
  }

  if (!mID3Parser.Header().Size() && !mFirstFrame.Length()) {
    
    
    
    const uint8_t* id3Beg = mID3Parser.Parse(aBeg, aEnd);
    if (id3Beg != aEnd) {
      
      aBeg = id3Beg + ID3Parser::ID3Header::SIZE + mID3Parser.Header().Size();
    }
  }

  while (aBeg < aEnd && !mFrame.ParseNext(*aBeg)) {
    ++aBeg;
  }

  if (mFrame.Length()) {
    
    if (!mFirstFrame.Length()) {
      mFirstFrame = mFrame;
    }
    
    aBeg -= FrameHeader::SIZE;
    return aBeg;
  }
  return aEnd;
}



FrameParser::FrameHeader::FrameHeader()
{
  Reset();
}

uint8_t
FrameParser::FrameHeader::Sync1() const {
  return mRaw[frame_header::SYNC1];
}

uint8_t
FrameParser::FrameHeader::Sync2() const {
  return 0x7 & mRaw[frame_header::SYNC2_VERSION_LAYER_PROTECTION] >> 5;
}

uint8_t
FrameParser::FrameHeader::RawVersion() const {
  return 0x3 & mRaw[frame_header::SYNC2_VERSION_LAYER_PROTECTION] >> 3;
}

uint8_t
FrameParser::FrameHeader::RawLayer() const {
  return 0x3 & mRaw[frame_header::SYNC2_VERSION_LAYER_PROTECTION] >> 1;
}

uint8_t
FrameParser::FrameHeader::RawProtection() const {
  return 0x1 & mRaw[frame_header::SYNC2_VERSION_LAYER_PROTECTION] >> 6;
}

uint8_t
FrameParser::FrameHeader::RawBitrate() const {
  return 0xF & mRaw[frame_header::BITRATE_SAMPLERATE_PADDING_PRIVATE] >> 4;
}

uint8_t
FrameParser::FrameHeader::RawSampleRate() const {
  return 0x3 & mRaw[frame_header::BITRATE_SAMPLERATE_PADDING_PRIVATE] >> 2;
}

uint8_t
FrameParser::FrameHeader::Padding() const {
  return 0x1 & mRaw[frame_header::BITRATE_SAMPLERATE_PADDING_PRIVATE] >> 1;
}

uint8_t
FrameParser::FrameHeader::Private() const {
  return 0x1 & mRaw[frame_header::BITRATE_SAMPLERATE_PADDING_PRIVATE];
}

uint8_t
FrameParser::FrameHeader::RawChannelMode() const {
  return 0xF & mRaw[frame_header::CHANNELMODE_MODEEXT_COPY_ORIG_EMPH] >> 4;
}

int32_t
FrameParser::FrameHeader::Layer() const {
  static const uint8_t LAYERS[4] = { 0, 3, 2, 1 };

  return LAYERS[RawLayer()];
}

int32_t
FrameParser::FrameHeader::SampleRate() const {
  
  static const uint16_t SAMPLE_RATE[4][4] = {
    { 11025, 12000,  8000, 0 }, 
    {     0,     0,     0, 0 }, 
    { 22050, 24000, 16000, 0 }, 
    { 44100, 48000, 32000, 0 }  
  };

  return SAMPLE_RATE[RawVersion()][RawSampleRate()];
}

int32_t
FrameParser::FrameHeader::Channels() const {
  
  
  return RawChannelMode() == 3 ? 1 : 2;
}

int32_t
FrameParser::FrameHeader::SamplesPerFrame() const {
  
  static const uint16_t FRAME_SAMPLE[4][4] = {
    
    {      0,  576, 1152,  384 }, 
    {      0,    0,    0,    0 }, 
    {      0,  576, 1152,  384 }, 
    {      0, 1152, 1152,  384 }  
  };

  return FRAME_SAMPLE[RawVersion()][RawLayer()];
}

int32_t
FrameParser::FrameHeader::Bitrate() const {
  
  static const uint16_t BITRATE[4][4][16] = {
    { 
      { 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 0 }, 
      { 0,   8,  16,  24,  32,  40,  48,  56,  64,  80,  96, 112, 128, 144, 160, 0 }, 
      { 0,   8,  16,  24,  32,  40,  48,  56,  64,  80,  96, 112, 128, 144, 160, 0 }, 
      { 0,  32,  48,  56,  64,  80,  96, 112, 128, 144, 160, 176, 192, 224, 256, 0 }  
    },
    { 
      { 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 0 }, 
      { 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 0 }, 
      { 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 0 }, 
      { 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 0 }  
    },
    { 
      { 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 0 }, 
      { 0,   8,  16,  24,  32,  40,  48,  56,  64,  80,  96, 112, 128, 144, 160, 0 }, 
      { 0,   8,  16,  24,  32,  40,  48,  56,  64,  80,  96, 112, 128, 144, 160, 0 }, 
      { 0,  32,  48,  56,  64,  80,  96, 112, 128, 144, 160, 176, 192, 224, 256, 0 }  
    },
    { 
      { 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 0 }, 
      { 0,  32,  40,  48,  56,  64,  80,  96, 112, 128, 160, 192, 224, 256, 320, 0 }, 
      { 0,  32,  48,  56,  64,  80,  96, 112, 128, 160, 192, 224, 256, 320, 384, 0 }, 
      { 0,  32,  64,  96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448, 0 }, 
    }
  };

  return 1000 * BITRATE[RawVersion()][RawLayer()][RawBitrate()];
}

int32_t
FrameParser::FrameHeader::SlotSize() const {
  
  static const uint8_t SLOT_SIZE[4] = { 0, 1, 1, 4 }; 

  return SLOT_SIZE[RawLayer()];
}

bool
FrameParser::FrameHeader::ParseNext(uint8_t c) {
  if (!Update(c)) {
    Reset();
    if (!Update(c)) {
      Reset();
    }
  }
  return IsValid();
}

bool
FrameParser::FrameHeader::IsValid(int aPos) const {
  if (IsValid()) {
    return true;
  }
  if (aPos == frame_header::SYNC1) {
    return Sync1() == 0xFF;
  }
  if (aPos == frame_header::SYNC2_VERSION_LAYER_PROTECTION) {
    return Sync2() == 7 &&
           RawVersion() != 1 &&
           RawLayer() != 0;
  }
  if (aPos == frame_header::BITRATE_SAMPLERATE_PADDING_PRIVATE) {
    return RawBitrate() != 0xF;
  }
  return true;
}

bool
FrameParser::FrameHeader::IsValid() const {
  return mPos >= SIZE;
}

void
FrameParser::FrameHeader::Reset() {
  mPos = 0;
}

bool
FrameParser::FrameHeader::Update(uint8_t c) {
  if (mPos < SIZE) {
    mRaw[mPos] = c;
  }
  return IsValid(mPos++);
}



FrameParser::VBRHeader::VBRHeader()
  : mNumFrames(-1),
    mType(NONE)
{
}

FrameParser::VBRHeader::VBRHeaderType
FrameParser::VBRHeader::Type() const {
  return mType;
}

int64_t
FrameParser::VBRHeader::NumFrames() const {
  return mNumFrames;
}

bool
FrameParser::VBRHeader::ParseXing(const uint8_t* aBeg, const uint8_t* aEnd) {
  static const uint32_t TAG = BigEndian::readUint32("Xing");
  static const uint32_t FRAME_COUNT_OFFSET = 8;

  enum Flags {
    NUM_FRAMES = 0x01,
    NUM_BYTES = 0x02,
    TOC = 0x04,
    VBR_SCALE = 0x08
  };

  if (!aBeg || !aEnd || aBeg >= aEnd) {
    return false;
  }

  
  for (; aBeg + sizeof(TAG) < aEnd; ++aBeg) {
    if (BigEndian::readUint32(aBeg) != TAG) {
      continue;
    }

    const uint32_t flags = BigEndian::readUint32(aBeg + sizeof(TAG));
    if (flags & NUM_FRAMES && aBeg + FRAME_COUNT_OFFSET < aEnd) {
      mNumFrames = BigEndian::readUint32(aBeg + FRAME_COUNT_OFFSET);
    }
    mType = XING;
    return true;
  }
  return false;
}

bool
FrameParser::VBRHeader::ParseVBRI(const uint8_t* aBeg, const uint8_t* aEnd) {
  static const uint32_t TAG = BigEndian::readUint32("VBRI");
  static const uint32_t OFFSET = 32 - FrameParser::FrameHeader::SIZE;
  static const uint32_t FRAME_COUNT_OFFSET = OFFSET + 14;
  static const uint32_t MIN_FRAME_SIZE = OFFSET + 26;

  if (!aBeg || !aEnd || aBeg >= aEnd) {
    return false;
  }

  const int64_t frameLen = aEnd - aBeg;
  
  if (frameLen > MIN_FRAME_SIZE &&
      BigEndian::readUint32(aBeg + OFFSET) == TAG) {
    mNumFrames = BigEndian::readUint32(aBeg + FRAME_COUNT_OFFSET);
    mType = VBRI;
    return true;
  }
  return false;
}

bool
FrameParser::VBRHeader::Parse(const uint8_t* aBeg, const uint8_t* aEnd) {
  return ParseVBRI(aBeg, aEnd) || ParseXing(aBeg, aEnd);
}



void
FrameParser::Frame::Reset() {
  mHeader.Reset();
}

int32_t
FrameParser::Frame::Length() const {
  if (!mHeader.IsValid() || !mHeader.SampleRate()) {
    return 0;
  }

  const float bitsPerSample = mHeader.SamplesPerFrame() / 8.0f;
  const int32_t frameLen = bitsPerSample * mHeader.Bitrate() /
                           mHeader.SampleRate() +
                           mHeader.Padding() * mHeader.SlotSize();
  return frameLen;
}

bool
FrameParser::Frame::ParseNext(uint8_t c) {
  return mHeader.ParseNext(c);
}

const FrameParser::FrameHeader&
FrameParser::Frame::Header() const {
  return mHeader;
}

bool
FrameParser::ParseVBRHeader(const uint8_t* aBeg, const uint8_t* aEnd) {
  return mVBRHeader.Parse(aBeg, aEnd);
}




namespace id3_header {
static const int ID_LEN = 3;
static const int VERSION_LEN = 2;
static const int FLAGS_LEN = 1;
static const int SIZE_LEN = 4;

static const int ID_END = ID_LEN;
static const int VERSION_END = ID_END + VERSION_LEN;
static const int FLAGS_END = VERSION_END + FLAGS_LEN;
static const int SIZE_END = FLAGS_END + SIZE_LEN;

static const uint8_t ID[ID_LEN] = {'I', 'D', '3'};
}

const uint8_t*
ID3Parser::Parse(const uint8_t* aBeg, const uint8_t* aEnd) {
  if (!aBeg || !aEnd || aBeg >= aEnd) {
    return aEnd;
  }

  while (aBeg < aEnd && !mHeader.ParseNext(*aBeg)) {
    ++aBeg;
  }

  if (aBeg < aEnd) {
    
    aBeg -= ID3Header::SIZE - 1;
  }
  return aBeg;
}

void
ID3Parser::Reset() {
  mHeader.Reset();
}

const ID3Parser::ID3Header&
ID3Parser::Header() const {
  return mHeader;
}



ID3Parser::ID3Header::ID3Header()
{
  Reset();
}

void
ID3Parser::ID3Header::Reset() {
  mSize = 0;
  mPos = 0;
}

uint8_t
ID3Parser::ID3Header::MajorVersion() const {
  return mRaw[id3_header::ID_END];
}

uint8_t
ID3Parser::ID3Header::MinorVersion() const {
  return mRaw[id3_header::ID_END + 1];
}

uint8_t
ID3Parser::ID3Header::Flags() const {
  return mRaw[id3_header::FLAGS_END - id3_header::FLAGS_LEN];
}

uint32_t
ID3Parser::ID3Header::Size() const {
  return mSize;
}

bool
ID3Parser::ID3Header::ParseNext(uint8_t c) {
  if (!Update(c)) {
    Reset();
    if (!Update(c)) {
      Reset();
    }
  }
  return IsValid();
}

bool
ID3Parser::ID3Header::IsValid(int aPos) const {
  if (IsValid()) {
    return true;
  }
  const uint8_t c = mRaw[aPos];
  if (aPos < id3_header::ID_END) {
    return id3_header::ID[aPos] == c;
  }
  if (aPos < id3_header::VERSION_END) {
    return c < 0xFF;
  }
  if (aPos < id3_header::FLAGS_END) {
    return true;
  }
  if (aPos < id3_header::SIZE_END) {
    return c < 0x80;
  }
  return true;
}

bool
ID3Parser::ID3Header::IsValid() const {
  return mPos >= SIZE;
}

bool
ID3Parser::ID3Header::Update(uint8_t c) {
  if (mPos >= id3_header::SIZE_END - id3_header::SIZE_LEN &&
      mPos < id3_header::SIZE_END) {
    mSize <<= 7;
    mSize |= c;
  }
  if (mPos < SIZE) {
    mRaw[mPos] = c;
  }
  return IsValid(mPos++);
}

}  
