




#include "gtest/gtest.h"
#include "VorbisTrackEncoder.h"
#include "VP8TrackEncoder.h"
#include "WebMWriter.h"

using namespace mozilla;

class WebMVorbisTrackEncoder : public VorbisTrackEncoder
{
public:
  bool TestVorbisCreation(int aChannels, int aSamplingRate)
  {
    if (NS_SUCCEEDED(Init(aChannels, aSamplingRate))) {
      return true;
    }
    return false;
  }
};

class WebMVP8TrackEncoder: public VP8TrackEncoder
{
public:
  bool TestVP8Creation(int32_t aWidth, int32_t aHeight, int32_t aDisplayWidth,
                       int32_t aDisplayHeight, TrackRate aTrackRate)
  {
    if (NS_SUCCEEDED(Init(aWidth, aHeight, aDisplayWidth, aDisplayHeight,
                          aTrackRate))) {
      return true;
    }
    return false;
  }
};

const uint64_t FIXED_DURATION = 1000000;
const uint32_t FIXED_FRAMESIZE = 500;

class TestWebMWriter: public WebMWriter
{
public:
  TestWebMWriter(int aTrackTypes)
  : WebMWriter(aTrackTypes),
    mTimestamp(0)
  {}

  void SetVorbisMetadata(int aChannels, int aSampleRate) {
    WebMVorbisTrackEncoder vorbisEncoder;
    EXPECT_TRUE(vorbisEncoder.TestVorbisCreation(aChannels, aSampleRate));
    nsRefPtr<TrackMetadataBase> vorbisMeta = vorbisEncoder.GetMetadata();
    SetMetadata(vorbisMeta);
  }
  void SetVP8Metadata(int32_t aWidth, int32_t aHeight, int32_t aDisplayWidth,
                      int32_t aDisplayHeight,TrackRate aTrackRate) {
    WebMVP8TrackEncoder vp8Encoder;
    EXPECT_TRUE(vp8Encoder.TestVP8Creation(aWidth, aHeight, aDisplayWidth,
                                           aDisplayHeight, aTrackRate));
    nsRefPtr<TrackMetadataBase> vp8Meta = vp8Encoder.GetMetadata();
    SetMetadata(vp8Meta);
  }

  
  
  
  
  void AppendDummyFrame(EncodedFrame::FrameType aFrameType,
                        uint64_t aDuration) {
    EncodedFrameContainer encodedVideoData;
    nsTArray<uint8_t> frameData;
    nsRefPtr<EncodedFrame> videoData = new EncodedFrame();
    
    frameData.SetLength(FIXED_FRAMESIZE);
    videoData->SetFrameType(aFrameType);
    videoData->SetTimeStamp(mTimestamp);
    videoData->SetDuration(aDuration);
    videoData->SwapInFrameData(frameData);
    encodedVideoData.AppendEncodedFrame(videoData);
    WriteEncodedTrack(encodedVideoData, 0);
    mTimestamp += aDuration;
  }

  bool HaveValidCluster() {
    nsTArray<nsTArray<uint8_t> > encodedBuf;
    GetContainerData(&encodedBuf, 0);
    return (encodedBuf.Length() > 0) ? true : false;
  }

  
  
  uint64_t mTimestamp;
};

TEST(WebMWriter, Metadata)
{
  TestWebMWriter writer(ContainerWriter::CREATE_AUDIO_TRACK |
                        ContainerWriter::CREATE_VIDEO_TRACK);

  
  nsTArray<nsTArray<uint8_t> > encodedBuf;
  writer.GetContainerData(&encodedBuf, ContainerWriter::GET_HEADER);
  EXPECT_TRUE(encodedBuf.Length() == 0);
  writer.GetContainerData(&encodedBuf, ContainerWriter::FLUSH_NEEDED);
  EXPECT_TRUE(encodedBuf.Length() == 0);

  
  int channel = 1;
  int sampleRate = 44100;
  writer.SetVorbisMetadata(channel, sampleRate);

  
  
  writer.GetContainerData(&encodedBuf, ContainerWriter::GET_HEADER);
  EXPECT_TRUE(encodedBuf.Length() == 0);
  writer.GetContainerData(&encodedBuf, ContainerWriter::FLUSH_NEEDED);
  EXPECT_TRUE(encodedBuf.Length() == 0);

  
  int32_t width = 640;
  int32_t height = 480;
  int32_t displayWidth = 640;
  int32_t displayHeight = 480;
  TrackRate aTrackRate = 90000;
  writer.SetVP8Metadata(width, height, displayWidth,
                        displayHeight, aTrackRate);

  writer.GetContainerData(&encodedBuf, ContainerWriter::GET_HEADER);
  EXPECT_TRUE(encodedBuf.Length() > 0);
}

TEST(WebMWriter, Cluster)
{
  TestWebMWriter writer(ContainerWriter::CREATE_AUDIO_TRACK |
                        ContainerWriter::CREATE_VIDEO_TRACK);
  
  int channel = 1;
  int sampleRate = 48000;
  writer.SetVorbisMetadata(channel, sampleRate);
  
  int32_t width = 320;
  int32_t height = 240;
  int32_t displayWidth = 320;
  int32_t displayHeight = 240;
  TrackRate aTrackRate = 90000;
  writer.SetVP8Metadata(width, height, displayWidth,
                        displayHeight, aTrackRate);

  nsTArray<nsTArray<uint8_t> > encodedBuf;
  writer.GetContainerData(&encodedBuf, ContainerWriter::GET_HEADER);
  EXPECT_TRUE(encodedBuf.Length() > 0);
  encodedBuf.Clear();

  uint64_t timestamp = 0;
  
  writer.AppendDummyFrame(EncodedFrame::VP8_I_FRAME, FIXED_DURATION);
  
  EXPECT_FALSE(writer.HaveValidCluster());

  
  writer.AppendDummyFrame(EncodedFrame::VP8_I_FRAME, FIXED_DURATION);
  
  EXPECT_TRUE(writer.HaveValidCluster());

  
  writer.AppendDummyFrame(EncodedFrame::VP8_P_FRAME, FIXED_DURATION);
  
  EXPECT_FALSE(writer.HaveValidCluster());

  
  writer.AppendDummyFrame(EncodedFrame::VP8_I_FRAME, FIXED_DURATION);
  
  EXPECT_TRUE(writer.HaveValidCluster());
}

TEST(WebMWriter, FLUSH_NEEDED)
{
  TestWebMWriter writer(ContainerWriter::CREATE_AUDIO_TRACK |
                        ContainerWriter::CREATE_VIDEO_TRACK);
  
  int channel = 2;
  int sampleRate = 44100;
  writer.SetVorbisMetadata(channel, sampleRate);
  
  int32_t width = 176;
  int32_t height = 352;
  int32_t displayWidth = 176;
  int32_t displayHeight = 352;
  TrackRate aTrackRate = 100000;
  writer.SetVP8Metadata(width, height, displayWidth,
                        displayHeight, aTrackRate);

  uint64_t timestamp = 0;
  
  writer.AppendDummyFrame(EncodedFrame::VP8_I_FRAME, FIXED_DURATION);

  
  writer.AppendDummyFrame(EncodedFrame::VP8_P_FRAME, FIXED_DURATION);
  
  EXPECT_TRUE(writer.HaveValidCluster());
  
  
  EXPECT_FALSE(writer.HaveValidCluster());

  nsTArray<nsTArray<uint8_t> > encodedBuf;
  
  writer.GetContainerData(&encodedBuf, ContainerWriter::FLUSH_NEEDED);
  EXPECT_TRUE(encodedBuf.Length() > 0);
  encodedBuf.Clear();

  
  writer.AppendDummyFrame(EncodedFrame::VP8_P_FRAME, FIXED_DURATION);
  
  
  EXPECT_FALSE(writer.HaveValidCluster());

  
  writer.AppendDummyFrame(EncodedFrame::VP8_I_FRAME, FIXED_DURATION);
  
  
  EXPECT_FALSE(writer.HaveValidCluster());

  
  writer.AppendDummyFrame(EncodedFrame::VP8_I_FRAME, FIXED_DURATION);
  
  EXPECT_TRUE(writer.HaveValidCluster());
}
