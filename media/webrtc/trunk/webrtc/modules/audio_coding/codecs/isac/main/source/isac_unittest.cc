








#include <string>

#include "testing/gtest/include/gtest/gtest.h"
#include "webrtc/modules/audio_coding/codecs/isac/main/interface/isac.h"
#include "webrtc/test/testsupport/fileutils.h"

struct WebRtcISACStruct;

namespace webrtc {


const int kIsacNumberOfSamples = 320 * 6;

const size_t kMaxBytes = 1000;

class IsacTest : public ::testing::Test {
 protected:
  IsacTest();
  virtual void SetUp();

  WebRtcISACStruct* isac_codec_;

  int16_t speech_data_[kIsacNumberOfSamples];
  int16_t output_data_[kIsacNumberOfSamples];
  int16_t bitstream_[kMaxBytes / 2];
  uint8_t bitstream_small_[7];  
};

IsacTest::IsacTest()
    : isac_codec_(NULL) {
}

void IsacTest::SetUp() {
  
  FILE* input_file;
  const std::string file_name =
        webrtc::test::ResourcePath("audio_coding/testfile32kHz", "pcm");
  input_file = fopen(file_name.c_str(), "rb");
  ASSERT_TRUE(input_file != NULL);
  ASSERT_EQ(kIsacNumberOfSamples,
            static_cast<int32_t>(fread(speech_data_, sizeof(int16_t),
                                       kIsacNumberOfSamples, input_file)));
  fclose(input_file);
  input_file = NULL;
}


TEST_F(IsacTest, IsacCreateFail) {
  
  EXPECT_EQ(-1, WebRtcIsac_Create(NULL));
}


TEST_F(IsacTest, IsacFreeFail) {
  
  EXPECT_EQ(0, WebRtcIsac_Free(NULL));
}


TEST_F(IsacTest, IsacCreateFree) {
  EXPECT_EQ(0, WebRtcIsac_Create(&isac_codec_));
  EXPECT_TRUE(isac_codec_ != NULL);
  EXPECT_EQ(0, WebRtcIsac_Free(isac_codec_));}

TEST_F(IsacTest, IsacUpdateBWE) {
  
  EXPECT_EQ(0, WebRtcIsac_Create(&isac_codec_));

  
  WebRtcIsac_EncoderInit(isac_codec_, 0);
  WebRtcIsac_DecoderInit(isac_codec_);

  
  int16_t encoded_bytes;
  uint16_t* coded = reinterpret_cast<uint16_t*>(bitstream_);
  uint16_t* coded_small = reinterpret_cast<uint16_t*>(bitstream_small_);

  
  EXPECT_EQ(-1, WebRtcIsac_UpdateBwEstimate(isac_codec_, coded_small, 7, 1,
                                            12345, 56789));

  
  encoded_bytes =  WebRtcIsac_Encode(isac_codec_, speech_data_, bitstream_);
  EXPECT_EQ(0, encoded_bytes);
  encoded_bytes =  WebRtcIsac_Encode(isac_codec_, speech_data_, bitstream_);
  EXPECT_EQ(0, encoded_bytes);
  encoded_bytes =  WebRtcIsac_Encode(isac_codec_, speech_data_, bitstream_);
  EXPECT_EQ(0, encoded_bytes);
  encoded_bytes =  WebRtcIsac_Encode(isac_codec_, speech_data_, bitstream_);
  EXPECT_EQ(0, encoded_bytes);
  encoded_bytes =  WebRtcIsac_Encode(isac_codec_, speech_data_, bitstream_);
  EXPECT_EQ(0, encoded_bytes);
  encoded_bytes =  WebRtcIsac_Encode(isac_codec_, speech_data_, bitstream_);

  
  EXPECT_EQ(0, WebRtcIsac_UpdateBwEstimate(isac_codec_, coded, encoded_bytes, 1,
                                           12345, 56789));

  
  EXPECT_EQ(0, WebRtcIsac_Free(isac_codec_));
}

}  
