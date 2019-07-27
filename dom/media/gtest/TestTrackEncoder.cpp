




#include "gtest/gtest.h"
#include "OpusTrackEncoder.h"

using namespace mozilla;

class TestOpusTrackEncoder : public OpusTrackEncoder
{
public:
  
  bool TestOpusCreation(int aChannels, int aSamplingRate)
  {
    if (Init(aChannels, aSamplingRate) == NS_OK) {
      if (GetPacketDuration()) {
        return true;
      }
    }
    return false;
  }

  
  
  
  int TestGetOutputSampleRate()
  {
    return mInitialized ? GetOutputSampleRate() : 0;
  }
};

static bool
TestOpusInit(int aChannels, int aSamplingRate)
{
  TestOpusTrackEncoder encoder;
  return encoder.TestOpusCreation(aChannels, aSamplingRate);
}

static int
TestOpusResampler(int aChannels, int aSamplingRate)
{
  TestOpusTrackEncoder encoder;
  EXPECT_TRUE(encoder.TestOpusCreation(aChannels, aSamplingRate));
  return encoder.TestGetOutputSampleRate();
}

TEST(Media, OpusEncoder_Init)
{
  
  EXPECT_FALSE(TestOpusInit(0, 16000));
  EXPECT_FALSE(TestOpusInit(-1, 16000));

  
  
  
  
  EXPECT_FALSE(TestOpusInit(8 + 1, 16000));

  
  for (int i = 1; i <= 8; i++) {
    EXPECT_TRUE(TestOpusInit(i, 16000));
  }

  
  EXPECT_FALSE(TestOpusInit(1, 0));
  EXPECT_FALSE(TestOpusInit(1, -1));

  
  EXPECT_FALSE(TestOpusInit(2, 2000));
  EXPECT_FALSE(TestOpusInit(2, 4000));
  EXPECT_FALSE(TestOpusInit(2, 7999));
  EXPECT_TRUE(TestOpusInit(2, 8000));
  EXPECT_TRUE(TestOpusInit(2, 192000));
  EXPECT_FALSE(TestOpusInit(2, 192001));
  EXPECT_FALSE(TestOpusInit(2, 200000));
}

TEST(Media, OpusEncoder_Resample)
{
  
  
  
  EXPECT_TRUE(TestOpusResampler(1, 8000) == 8000);
  EXPECT_TRUE(TestOpusResampler(1, 12000) == 12000);
  EXPECT_TRUE(TestOpusResampler(1, 16000) == 16000);
  EXPECT_TRUE(TestOpusResampler(1, 24000) == 24000);
  EXPECT_TRUE(TestOpusResampler(1, 48000) == 48000);

  
  EXPECT_FALSE(TestOpusResampler(1, 9600) == 9600);
  EXPECT_FALSE(TestOpusResampler(1, 44100) == 44100);
}
