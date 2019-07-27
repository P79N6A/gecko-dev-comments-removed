











#include "webrtc/modules/audio_coding/neteq/normal.h"

#include <vector>

#include "testing/gtest/include/gtest/gtest.h"
#include "webrtc/common_audio/signal_processing/include/signal_processing_library.h"
#include "webrtc/modules/audio_coding/neteq/audio_multi_vector.h"
#include "webrtc/modules/audio_coding/neteq/background_noise.h"
#include "webrtc/modules/audio_coding/neteq/expand.h"
#include "webrtc/modules/audio_coding/neteq/mock/mock_decoder_database.h"
#include "webrtc/modules/audio_coding/neteq/mock/mock_expand.h"
#include "webrtc/modules/audio_coding/neteq/random_vector.h"
#include "webrtc/modules/audio_coding/neteq/sync_buffer.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"

using ::testing::_;

namespace webrtc {

TEST(Normal, CreateAndDestroy) {
  MockDecoderDatabase db;
  int fs = 8000;
  size_t channels = 1;
  BackgroundNoise bgn(channels);
  SyncBuffer sync_buffer(1, 1000);
  RandomVector random_vector;
  Expand expand(&bgn, &sync_buffer, &random_vector, fs, channels);
  Normal normal(fs, &db, bgn, &expand);
  EXPECT_CALL(db, Die());  
}

TEST(Normal, AvoidDivideByZero) {
  WebRtcSpl_Init();
  MockDecoderDatabase db;
  int fs = 8000;
  size_t channels = 1;
  BackgroundNoise bgn(channels);
  SyncBuffer sync_buffer(1, 1000);
  RandomVector random_vector;
  MockExpand expand(&bgn, &sync_buffer, &random_vector, fs, channels);
  Normal normal(fs, &db, bgn, &expand);

  int16_t input[1000] = {0};
  scoped_ptr<int16_t[]> mute_factor_array(new int16_t[channels]);
  for (size_t i = 0; i < channels; ++i) {
    mute_factor_array[i] = 16384;
  }
  AudioMultiVector output(channels);

  
  EXPECT_EQ(
      0,
      normal.Process(input, 0, kModeExpand, mute_factor_array.get(), &output));
  EXPECT_EQ(0u, output.Size());

  
  EXPECT_CALL(expand, SetParametersForNormalAfterExpand());
  EXPECT_CALL(expand, Process(_));
  EXPECT_CALL(expand, Reset());
  
  
  
  
  int input_size_samples = 63;
  EXPECT_EQ(input_size_samples,
            normal.Process(input,
                           input_size_samples,
                           kModeExpand,
                           mute_factor_array.get(),
                           &output));

  EXPECT_CALL(db, Die());      
  EXPECT_CALL(expand, Die());  
}

TEST(Normal, InputLengthAndChannelsDoNotMatch) {
  WebRtcSpl_Init();
  MockDecoderDatabase db;
  int fs = 8000;
  size_t channels = 2;
  BackgroundNoise bgn(channels);
  SyncBuffer sync_buffer(channels, 1000);
  RandomVector random_vector;
  MockExpand expand(&bgn, &sync_buffer, &random_vector, fs, channels);
  Normal normal(fs, &db, bgn, &expand);

  int16_t input[1000] = {0};
  scoped_ptr<int16_t[]> mute_factor_array(new int16_t[channels]);
  for (size_t i = 0; i < channels; ++i) {
    mute_factor_array[i] = 16384;
  }
  AudioMultiVector output(channels);

  
  size_t input_len = 80 * channels - 1;
  EXPECT_EQ(
      0,
      normal.Process(
          input, input_len, kModeExpand, mute_factor_array.get(), &output));
  EXPECT_EQ(0u, output.Size());

  EXPECT_CALL(db, Die());      
  EXPECT_CALL(expand, Die());  
}



}  
