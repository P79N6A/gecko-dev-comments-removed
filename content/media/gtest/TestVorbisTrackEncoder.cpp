




#include "gtest/gtest.h"
#include "VorbisTrackEncoder.h"
#include "WebMWriter.h"
#include "MediaStreamGraph.h"

using namespace mozilla;

class TestVorbisTrackEncoder : public VorbisTrackEncoder
{
public:
  
  bool TestVorbisCreation(int aChannels, int aSamplingRate)
  {
    if (Init(aChannels, aSamplingRate) == NS_OK) {
      return true;
    }
    return false;
  }
};

static bool
TestVorbisInit(int aChannels, int aSamplingRate)
{
  TestVorbisTrackEncoder encoder;
  return encoder.TestVorbisCreation(aChannels, aSamplingRate);
}

static int
ReadLacing(const uint8_t* aInput, uint32_t aInputLength, uint32_t& aReadBytes)
{
  aReadBytes = 0;

  int packetSize = 0;
  while (aReadBytes < aInputLength) {
    if (aInput[aReadBytes] == 255) {
      packetSize += 255;
      aReadBytes++;
    } else { 
      packetSize += aInput[aReadBytes];
      aReadBytes++;
      break;
    }
  }

  return packetSize;
}

static bool
parseVorbisMetadata(nsTArray<uint8_t>& aData, int aChannels, int aRate)
{
  uint32_t offset = 0;
  
  if (aData.ElementAt(0) != 2) {
    return false;
  }
  offset = 1;

  
  uint32_t readbytes;
  ogg_packet header;
  ogg_packet header_comm;
  ogg_packet header_code;
  int header_length;
  int header_comm_length;
  int header_code_length;
  EXPECT_TRUE(offset < aData.Length());
  header_length = ReadLacing(aData.Elements()+offset, aData.Length()-offset,
                             readbytes);
  offset += readbytes;
  EXPECT_TRUE(offset < aData.Length());
  header_comm_length = ReadLacing(aData.Elements()+offset,
                                  aData.Length()-offset, readbytes);
  offset += readbytes;
  EXPECT_TRUE(offset < aData.Length());
  
  header_code_length = aData.Length() - offset - header_length
                       - header_comm_length;
  EXPECT_TRUE(header_code_length >= 32);

  
  header.packet = aData.Elements() + offset;
  header.bytes = header_length;
  offset += header_length;
  header_comm.packet = aData.Elements() + offset;
  header_comm.bytes = header_comm_length;
  offset += header_comm_length;
  header_code.packet = aData.Elements() + offset;
  header_code.bytes = header_code_length;

  vorbis_info vi;
  vorbis_comment vc;
  vorbis_info_init(&vi);
  vorbis_comment_init(&vc);
  EXPECT_TRUE(0 == vorbis_synthesis_headerin(&vi,&vc,&header));

  EXPECT_TRUE(0 == vorbis_synthesis_headerin(&vi,&vc,&header_comm));

  EXPECT_TRUE(0 == vorbis_synthesis_headerin(&vi,&vc,&header_code));

  EXPECT_TRUE(vi.channels == aChannels);
  EXPECT_TRUE(vi.rate == aRate);

  return true;
}


TEST(VorbisTrackEncoder, Init)
{
  
  
  EXPECT_FALSE(TestVorbisInit(0, 16000));
  EXPECT_FALSE(TestVorbisInit(-1, 16000));
  EXPECT_FALSE(TestVorbisInit(8 + 1, 16000));

  
  for (int i = 1; i <= 8; i++) {
    EXPECT_FALSE(TestVorbisInit(i, -1));
    EXPECT_TRUE(TestVorbisInit(i, 8000));
    EXPECT_TRUE(TestVorbisInit(i, 11000));
    EXPECT_TRUE(TestVorbisInit(i, 16000));
    EXPECT_TRUE(TestVorbisInit(i, 22050));
    EXPECT_TRUE(TestVorbisInit(i, 32000));
    EXPECT_TRUE(TestVorbisInit(i, 44100));
    EXPECT_TRUE(TestVorbisInit(i, 48000));
    EXPECT_TRUE(TestVorbisInit(i, 96000));
    EXPECT_FALSE(TestVorbisInit(i, 200000 + 1));
  }
}


TEST(VorbisTrackEncoder, Metadata)
{
  
  TestVorbisTrackEncoder encoder;
  int channels = 1;
  int rate = 44100;
  encoder.TestVorbisCreation(channels, rate);

  nsRefPtr<TrackMetadataBase> meta = encoder.GetMetadata();
  nsRefPtr<VorbisMetadata> vorbisMetadata(static_cast<VorbisMetadata*>(meta.get()));

  
  
  EXPECT_TRUE(vorbisMetadata->mChannels == channels);
  EXPECT_TRUE(vorbisMetadata->mSamplingFrequency == rate);
  EXPECT_TRUE(parseVorbisMetadata(vorbisMetadata->mData, channels, rate));
}


TEST(VorbisTrackEncoder, EncodedFrame)
{
  
  TestVorbisTrackEncoder encoder;
  int channels = 1;
  int rate = 44100;
  encoder.TestVorbisCreation(channels, rate);

  
  
  nsRefPtr<mozilla::SharedBuffer> samples =
    mozilla::SharedBuffer::Create(rate * sizeof(AudioDataValue));
  AudioDataValue* data = static_cast<AudioDataValue*>(samples->Data());
  for (int i = 0; i < rate; i++) {
    data[i] = ((i%8)*4000) - (7*4000)/2;
  }
  nsAutoTArray<const AudioDataValue*,1> channelData;
  channelData.AppendElement(data);
  AudioSegment segment;
  segment.AppendFrames(samples.forget(), channelData, 44100);

  
  encoder.NotifyQueuedTrackChanges(nullptr, 0, 0, 0, 0, segment);

  
  EncodedFrameContainer container;
  EXPECT_TRUE(NS_SUCCEEDED(encoder.GetEncodedTrack(container)));
  
  EXPECT_TRUE(container.GetEncodedFrames().Length() > 0);
  EXPECT_TRUE(container.GetEncodedFrames().ElementAt(0)->GetFrameData().Length()
              > 0);
  EXPECT_TRUE(container.GetEncodedFrames().ElementAt(0)->GetFrameType() ==
              EncodedFrame::FrameType::VORBIS_AUDIO_FRAME);
  
  EXPECT_TRUE(container.GetEncodedFrames().ElementAt(0)->GetDuration() == 0);
  EXPECT_TRUE(container.GetEncodedFrames().ElementAt(0)->GetTimeStamp() == 0);
}


TEST(VorbisTrackEncoder, EncodeComplete)
{
  
  TestVorbisTrackEncoder encoder;
  int channels = 1;
  int rate = 44100;
  encoder.TestVorbisCreation(channels, rate);

  
  AudioSegment segment;
  encoder.NotifyQueuedTrackChanges(nullptr, 0, 0, 0,
                                   MediaStreamListener::TRACK_EVENT_ENDED,
                                   segment);

  
  
  
  EncodedFrameContainer container;
  EXPECT_TRUE(NS_SUCCEEDED(encoder.GetEncodedTrack(container)));
}
