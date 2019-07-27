









#include "webrtc/common_audio/blocker.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace {


class SimpleBlockerCallback : public webrtc::BlockerCallback {
 public:
  virtual void ProcessBlock(const float* const* input,
                            int num_frames,
                            int num_input_channels,
                            int num_output_channels,
                            float* const* output) OVERRIDE {
    for (int i = 0; i < num_output_channels; ++i) {
      for (int j = 0; j < num_frames; ++j) {
        output[i][j] = input[i][j] + 3;
      }
    }
  }
};

}  

namespace webrtc {




class BlockerTest : public ::testing::Test {
 protected:
  void RunTest(Blocker* blocker,
               int chunk_size,
               int num_frames,
               const float* const* input,
               float* const* input_chunk,
               float* const* output,
               float* const* output_chunk,
               int num_input_channels,
               int num_output_channels) {
    int start = 0;
    int end = chunk_size - 1;
    while (end < num_frames) {
      CopyTo(input_chunk, 0, start, num_input_channels, chunk_size, input);
      blocker->ProcessChunk(input_chunk,
                            chunk_size,
                            num_input_channels,
                            num_output_channels,
                            output_chunk);
      CopyTo(output, start, 0, num_output_channels, chunk_size, output_chunk);

      start = start + chunk_size;
      end = end + chunk_size;
    }
  }

  void ValidateSignalEquality(const float* const* expected,
                              const float* const* actual,
                              int num_channels,
                              int num_frames) {
    for (int i = 0; i < num_channels; ++i) {
      for (int j = 0; j < num_frames; ++j) {
        EXPECT_FLOAT_EQ(expected[i][j], actual[i][j]);
      }
    }
  }

  static void CopyTo(float* const* dst,
                     int start_index_dst,
                     int start_index_src,
                     int num_channels,
                     int num_frames,
                     const float* const* src) {
    for (int i = 0; i < num_channels; ++i) {
      memcpy(&dst[i][start_index_dst],
             &src[i][start_index_src],
             num_frames * sizeof(float));
    }
  }
};

TEST_F(BlockerTest, TestBlockerMutuallyPrimeChunkandBlockSize) {
  const int kNumInputChannels = 3;
  const int kNumOutputChannels = 2;
  const int kNumFrames = 10;
  const int kBlockSize = 4;
  const int kChunkSize = 5;
  const int kShiftAmount = 2;

  const float kInput[kNumInputChannels][kNumFrames] = {
      {1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
      {2, 2, 2, 2, 2, 2, 2, 2, 2, 2},
      {3, 3, 3, 3, 3, 3, 3, 3, 3, 3}};
  const ChannelBuffer<float> input_cb(kInput[0], kNumFrames, kNumInputChannels);

  const float kExpectedOutput[kNumInputChannels][kNumFrames] = {
      {6, 6, 12, 12, 20, 20, 20, 20, 20, 20},
      {6, 6, 12, 12, 28, 28, 28, 28, 28, 28}};
  const ChannelBuffer<float> expected_output_cb(
      kExpectedOutput[0], kNumFrames, kNumInputChannels);

  const float kWindow[kBlockSize] = {2.f, 2.f, 2.f, 2.f};

  ChannelBuffer<float> actual_output_cb(kNumFrames, kNumOutputChannels);
  ChannelBuffer<float> input_chunk_cb(kChunkSize, kNumInputChannels);
  ChannelBuffer<float> output_chunk_cb(kChunkSize, kNumOutputChannels);

  SimpleBlockerCallback callback;
  Blocker blocker(kChunkSize,
                  kBlockSize,
                  kNumInputChannels,
                  kNumOutputChannels,
                  kWindow,
                  kShiftAmount,
                  &callback);

  RunTest(&blocker,
          kChunkSize,
          kNumFrames,
          input_cb.channels(),
          input_chunk_cb.channels(),
          actual_output_cb.channels(),
          output_chunk_cb.channels(),
          kNumInputChannels,
          kNumOutputChannels);

  ValidateSignalEquality(expected_output_cb.channels(),
                         actual_output_cb.channels(),
                         kNumOutputChannels,
                         kNumFrames);
}

TEST_F(BlockerTest, TestBlockerMutuallyPrimeShiftAndBlockSize) {
  const int kNumInputChannels = 3;
  const int kNumOutputChannels = 2;
  const int kNumFrames = 12;
  const int kBlockSize = 4;
  const int kChunkSize = 6;
  const int kShiftAmount = 3;

  const float kInput[kNumInputChannels][kNumFrames] = {
      {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
      {2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2},
      {3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3}};
  const ChannelBuffer<float> input_cb(kInput[0], kNumFrames, kNumInputChannels);

  const float kExpectedOutput[kNumInputChannels][kNumFrames] = {
      {6, 6, 6, 12, 10, 10, 20, 10, 10, 20, 10, 10},
      {6, 6, 6, 12, 14, 14, 28, 14, 14, 28, 14, 14}};
  const ChannelBuffer<float> expected_output_cb(
      kExpectedOutput[0], kNumFrames, kNumInputChannels);

  const float kWindow[kBlockSize] = {2.f, 2.f, 2.f, 2.f};

  ChannelBuffer<float> actual_output_cb(kNumFrames, kNumOutputChannels);
  ChannelBuffer<float> input_chunk_cb(kChunkSize, kNumInputChannels);
  ChannelBuffer<float> output_chunk_cb(kChunkSize, kNumOutputChannels);

  SimpleBlockerCallback callback;
  Blocker blocker(kChunkSize,
                  kBlockSize,
                  kNumInputChannels,
                  kNumOutputChannels,
                  kWindow,
                  kShiftAmount,
                  &callback);

  RunTest(&blocker,
          kChunkSize,
          kNumFrames,
          input_cb.channels(),
          input_chunk_cb.channels(),
          actual_output_cb.channels(),
          output_chunk_cb.channels(),
          kNumInputChannels,
          kNumOutputChannels);

  ValidateSignalEquality(expected_output_cb.channels(),
                         actual_output_cb.channels(),
                         kNumOutputChannels,
                         kNumFrames);
}

TEST_F(BlockerTest, TestBlockerNoOverlap) {
  const int kNumInputChannels = 3;
  const int kNumOutputChannels = 2;
  const int kNumFrames = 12;
  const int kBlockSize = 4;
  const int kChunkSize = 4;
  const int kShiftAmount = 4;

  const float kInput[kNumInputChannels][kNumFrames] = {
      {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
      {2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2},
      {3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3}};
  const ChannelBuffer<float> input_cb(kInput[0], kNumFrames, kNumInputChannels);

  const float kExpectedOutput[kNumInputChannels][kNumFrames] = {
      {6, 6, 6, 6, 10, 10, 10, 10, 10, 10, 10, 10},
      {6, 6, 6, 6, 14, 14, 14, 14, 14, 14, 14, 14}};
  const ChannelBuffer<float> expected_output_cb(
      kExpectedOutput[0], kNumFrames, kNumInputChannels);

  const float kWindow[kBlockSize] = {2.f, 2.f, 2.f, 2.f};

  ChannelBuffer<float> actual_output_cb(kNumFrames, kNumOutputChannels);
  ChannelBuffer<float> input_chunk_cb(kChunkSize, kNumInputChannels);
  ChannelBuffer<float> output_chunk_cb(kChunkSize, kNumOutputChannels);

  SimpleBlockerCallback callback;
  Blocker blocker(kChunkSize,
                  kBlockSize,
                  kNumInputChannels,
                  kNumOutputChannels,
                  kWindow,
                  kShiftAmount,
                  &callback);

  RunTest(&blocker,
          kChunkSize,
          kNumFrames,
          input_cb.channels(),
          input_chunk_cb.channels(),
          actual_output_cb.channels(),
          output_chunk_cb.channels(),
          kNumInputChannels,
          kNumOutputChannels);

  ValidateSignalEquality(expected_output_cb.channels(),
                         actual_output_cb.channels(),
                         kNumOutputChannels,
                         kNumFrames);
}

}  
