









#include "webrtc/modules/audio_coding/neteq/audio_classifier.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>

#include "testing/gtest/include/gtest/gtest.h"
#include "webrtc/test/testsupport/fileutils.h"

namespace webrtc {

static const size_t kFrameSize = 960;

TEST(AudioClassifierTest, AllZeroInput) {
  int16_t in_mono[kFrameSize] = {0};

  
  
  AudioClassifier zero_classifier;
  for (int i = 0; i < 100; ++i) {
    zero_classifier.Analysis(in_mono, kFrameSize, 1);
  }
  EXPECT_TRUE(zero_classifier.is_music());
}

void RunAnalysisTest(const std::string& audio_filename,
                     const std::string& data_filename,
                     size_t channels) {
  AudioClassifier classifier;
  scoped_ptr<int16_t[]> in(new int16_t[channels * kFrameSize]);
  bool is_music_ref;

  FILE* audio_file = fopen(audio_filename.c_str(), "rb");
  ASSERT_TRUE(audio_file != NULL) << "Failed to open file " << audio_filename
                                  << std::endl;
  FILE* data_file = fopen(data_filename.c_str(), "rb");
  ASSERT_TRUE(audio_file != NULL) << "Failed to open file " << audio_filename
                                  << std::endl;
  while (fread(in.get(), sizeof(int16_t), channels * kFrameSize, audio_file) ==
         channels * kFrameSize) {
    bool is_music =
        classifier.Analysis(in.get(), channels * kFrameSize, channels);
    EXPECT_EQ(is_music, classifier.is_music());
    ASSERT_EQ(1u, fread(&is_music_ref, sizeof(is_music_ref), 1, data_file));
    EXPECT_EQ(is_music_ref, is_music);
  }
  fclose(audio_file);
  fclose(data_file);
}

TEST(AudioClassifierTest, DoAnalysisMono) {
  RunAnalysisTest(test::ResourcePath("short_mixed_mono_48", "pcm"),
                  test::ResourcePath("short_mixed_mono_48", "dat"),
                  1);
}

TEST(AudioClassifierTest, DoAnalysisStereo) {
  RunAnalysisTest(test::ResourcePath("short_mixed_stereo_48", "pcm"),
                  test::ResourcePath("short_mixed_stereo_48", "dat"),
                  2);
}

}  
