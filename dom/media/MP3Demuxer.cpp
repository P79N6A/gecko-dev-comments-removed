





#include "MP3Demuxer.h"

#include <inttypes.h>
#include <algorithm>

#include "mozilla/Assertions.h"
#include "mozilla/Endian.h"
#include "VideoUtils.h"
#include "TimeUnits.h"

using media::TimeUnit;
using media::TimeIntervals;

namespace mozilla {
namespace mp3 {



MP3Demuxer::MP3Demuxer(MediaResource* aSource)
  : mSource(aSource)
{}

bool
MP3Demuxer::InitInternal() {
  if (!mTrackDemuxer) {
    mTrackDemuxer = new MP3TrackDemuxer(mSource);
  }
  return mTrackDemuxer->Init();
}

nsRefPtr<MP3Demuxer::InitPromise>
MP3Demuxer::Init() {
  if (!InitInternal()) {
    return InitPromise::CreateAndReject(
      DemuxerFailureReason::WAITING_FOR_DATA, __func__);
  }

  return InitPromise::CreateAndResolve(NS_OK, __func__);
}

already_AddRefed<MediaDataDemuxer>
MP3Demuxer::Clone() const {
  nsRefPtr<MP3Demuxer> demuxer = new MP3Demuxer(mSource);
  if (!demuxer->InitInternal()) {
    NS_WARNING("Couldn't recreate MP3Demuxer");
    return nullptr;
  }
  return demuxer.forget();
}

bool
MP3Demuxer::HasTrackType(TrackInfo::TrackType aType) const {
  return aType == TrackInfo::kAudioTrack;
}

uint32_t
MP3Demuxer::GetNumberTracks(TrackInfo::TrackType aType) const {
  return aType == TrackInfo::kAudioTrack ? 1u : 0u;
}

already_AddRefed<MediaTrackDemuxer>
MP3Demuxer::GetTrackDemuxer(TrackInfo::TrackType aType, uint32_t aTrackNumber) {
  if (!mTrackDemuxer) {
    return nullptr;
  }
  return nsRefPtr<MP3TrackDemuxer>(mTrackDemuxer).forget();
}

bool
MP3Demuxer::IsSeekable() const {
  return true;
}

void
MP3Demuxer::NotifyDataArrived(uint32_t aLength, int64_t aOffset) {
  
  NS_WARNING("Unimplemented function NotifyDataArrived");
}

void
MP3Demuxer::NotifyDataRemoved() {
  
  NS_WARNING("Unimplemented function NotifyDataRemoved");
}




MP3TrackDemuxer::MP3TrackDemuxer(MediaResource* aSource)
  : mSource(aSource),
    mOffset(0),
    mFirstFrameOffset(0),
    mNumParsedFrames(0),
    mFrameIndex(0),
    mTotalFrameLen(0),
    mSamplesPerFrame(0),
    mSamplesPerSecond(0),
    mChannels(0)
{
}

bool
MP3TrackDemuxer::Init() {
  FastSeek(TimeUnit());
  
  nsRefPtr<MediaRawData> frame(GetNextFrame(FindNextFrame()));
  if (!frame) {
    return false;
  }

  
  FastSeek(TimeUnit());

  if (!mInfo) {
    mInfo = MakeUnique<AudioInfo>();
  }

  mInfo->mRate = mSamplesPerSecond;
  mInfo->mChannels = mChannels;
  mInfo->mBitDepth = 16;
  mInfo->mMimeType = "audio/mpeg";
  mInfo->mDuration = Duration().ToMicroseconds();

  return mSamplesPerSecond && mChannels;
}

#ifdef ENABLE_TESTS
const FrameParser::Frame&
MP3TrackDemuxer::LastFrame() const {
  return mParser.PrevFrame();
}

nsRefPtr<MediaRawData>
MP3TrackDemuxer::DemuxSample() {
  return GetNextFrame(FindNextFrame());
}
#endif

const ID3Parser::ID3Header&
MP3TrackDemuxer::ID3Header() const {
  return mParser.ID3Header();
}

const FrameParser::VBRHeader&
MP3TrackDemuxer::VBRInfo() const {
  return mParser.VBRInfo();
}

UniquePtr<TrackInfo>
MP3TrackDemuxer::GetInfo() const {
  return mInfo->Clone();
}

nsRefPtr<MP3TrackDemuxer::SeekPromise>
MP3TrackDemuxer::Seek(TimeUnit aTime) {
  const TimeUnit seekTime = ScanUntil(aTime);

  return SeekPromise::CreateAndResolve(seekTime, __func__);
}

TimeUnit
MP3TrackDemuxer::FastSeek(TimeUnit aTime) {
  if (!aTime.ToMicroseconds()) {
    
    mOffset = mFirstFrameOffset;
    mFrameIndex = 0;
    mParser.FinishParsing();
    return TimeUnit();
  }

  if (!mSamplesPerFrame || !mNumParsedFrames) {
    return TimeUnit::FromMicroseconds(-1);
  }

  const int64_t numFrames = aTime.ToSeconds() *
                            mSamplesPerSecond / mSamplesPerFrame;
  mOffset = mFirstFrameOffset + numFrames * AverageFrameLength();
  mFrameIndex = numFrames;
  mParser.FinishParsing();

  return Duration(mFrameIndex);
}

TimeUnit
MP3TrackDemuxer::ScanUntil(TimeUnit aTime) {
  if (!aTime.ToMicroseconds()) {
    return FastSeek(aTime);
  }

  if (Duration(mFrameIndex) > aTime) {
    FastSeek(aTime);
  }

  MediaByteRange nextRange = FindNextFrame();
  while (SkipNextFrame(nextRange) && Duration(mFrameIndex + 1) < aTime) {
    nextRange = FindNextFrame();
  }

  return Duration(mFrameIndex);
}

nsRefPtr<MP3TrackDemuxer::SamplesPromise>
MP3TrackDemuxer::GetSamples(int32_t aNumSamples) {
  if (!aNumSamples) {
    return SamplesPromise::CreateAndReject(
        DemuxerFailureReason::DEMUXER_ERROR, __func__);
  }

  nsRefPtr<SamplesHolder> frames = new SamplesHolder();

  while (aNumSamples--) {
    nsRefPtr<MediaRawData> frame(GetNextFrame(FindNextFrame()));
    if (!frame) {
      break;
    }

    frames->mSamples.AppendElement(frame);
  }

  if (frames->mSamples.IsEmpty()) {
    return SamplesPromise::CreateAndReject(
        DemuxerFailureReason::END_OF_STREAM, __func__);
  }
  return SamplesPromise::CreateAndResolve(frames, __func__);
}

void
MP3TrackDemuxer::Reset() {
  FastSeek(TimeUnit());
}

nsRefPtr<MP3TrackDemuxer::SkipAccessPointPromise>
MP3TrackDemuxer::SkipToNextRandomAccessPoint(TimeUnit aTimeThreshold) {
  
  return SkipAccessPointPromise::CreateAndReject(
    SkipFailureHolder(DemuxerFailureReason::DEMUXER_ERROR, 0), __func__);
}

int64_t
MP3TrackDemuxer::GetResourceOffset() const {
  return mOffset;
}

TimeIntervals
MP3TrackDemuxer::GetBuffered() {
  
  NS_WARNING("Unimplemented function GetBuffered");
  return TimeIntervals();
}

int64_t
MP3TrackDemuxer::GetEvictionOffset(TimeUnit aTime) {
  return 0;
}

int64_t
MP3TrackDemuxer::StreamLength() const {
  return mSource->GetLength();
}

TimeUnit
MP3TrackDemuxer::Duration() const {
  if (!mNumParsedFrames) {
    return TimeUnit::FromMicroseconds(-1);
  }

  const int64_t streamLen = StreamLength();
  
  int64_t numFrames = mParser.VBRInfo().NumFrames();
  if (numFrames < 0) {
    if (streamLen < 0) {
      
      return TimeUnit::FromMicroseconds(-1);
    }
    numFrames = (streamLen - mFirstFrameOffset) / AverageFrameLength();
  }
  return Duration(numFrames);
}

TimeUnit
MP3TrackDemuxer::Duration(int64_t aNumFrames) const {
  if (!mSamplesPerSecond) {
    return TimeUnit::FromMicroseconds(-1);
  }

  const double usPerFrame = USECS_PER_S * mSamplesPerFrame / mSamplesPerSecond;
  return TimeUnit::FromMicroseconds(aNumFrames * usPerFrame);
}

MediaByteRange
MP3TrackDemuxer::FindNextFrame() {
  static const int BUFFER_SIZE = 4096;

  uint8_t buffer[BUFFER_SIZE];
  int32_t read = 0;
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
    return { 0, 0 };
  }

  const int64_t nextBeg = mOffset - (bufferEnd - frameBeg) + 1;
  return { nextBeg, nextBeg + mParser.CurrentFrame().Length() };
}

bool
MP3TrackDemuxer::SkipNextFrame(const MediaByteRange& aRange) {
  if (!mNumParsedFrames || !aRange.Length()) {
    
    nsRefPtr<MediaRawData> frame(GetNextFrame(aRange));
    return frame;
  }

  UpdateState(aRange);

  return true;
}

already_AddRefed<MediaRawData>
MP3TrackDemuxer::GetNextFrame(const MediaByteRange& aRange) {
  if (!aRange.Length()) {
    return nullptr;
  }

  nsRefPtr<MediaRawData> frame = new MediaRawData();
  frame->mOffset = aRange.mStart;

  nsAutoPtr<MediaRawDataWriter> frameWriter(frame->CreateWriter());
  if (!frameWriter->SetSize(aRange.Length())) {
    return nullptr;
  }

  const uint32_t read = Read(frameWriter->mData, frame->mOffset, frame->mSize);

  if (read != aRange.Length()) {
    return nullptr;
  }

  UpdateState(aRange);

  frame->mTime = Duration(mFrameIndex - 1).ToMicroseconds();
  frame->mDuration = Duration(1).ToMicroseconds();

  MOZ_ASSERT(frame->mTime >= 0);
  MOZ_ASSERT(frame->mDuration > 0);

  if (mNumParsedFrames == 1) {
    
    
    mParser.ParseVBRHeader(frame->mData, frame->mData + frame->mSize);
    mFirstFrameOffset = frame->mOffset;
  }

  return frame.forget();
}

void
MP3TrackDemuxer::UpdateState(const MediaByteRange& aRange) {
  
  if (mTotalFrameLen + aRange.Length() < mTotalFrameLen) {
    
    
    mTotalFrameLen /= 2;
    mNumParsedFrames /= 2;
  }

  
  mOffset = aRange.mEnd;

  mTotalFrameLen += aRange.Length();
  mSamplesPerFrame = mParser.CurrentFrame().Header().SamplesPerFrame();
  mSamplesPerSecond = mParser.CurrentFrame().Header().SampleRate();
  mChannels = mParser.CurrentFrame().Header().Channels();
  ++mNumParsedFrames;
  ++mFrameIndex;
  MOZ_ASSERT(mFrameIndex > 0);

  
  mParser.FinishParsing();
}

int32_t
MP3TrackDemuxer::Read(uint8_t* aBuffer, int64_t aOffset, int32_t aSize) {
  aSize = std::min<int64_t>(aSize, StreamLength() - aOffset);
  if (aSize <= 0) {
    return 0;
  }

  uint32_t read = 0;
  const nsresult rv = mSource->ReadAt(aOffset, reinterpret_cast<char*>(aBuffer),
                                      static_cast<uint32_t>(aSize), &read);
  NS_ENSURE_SUCCESS(rv, 0);
  return static_cast<int32_t>(read);
}

double
MP3TrackDemuxer::AverageFrameLength() const {
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
}  
