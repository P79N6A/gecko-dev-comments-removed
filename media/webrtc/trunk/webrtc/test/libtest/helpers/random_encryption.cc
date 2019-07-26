









#include "test/libtest/include/random_encryption.h"

#include <algorithm>
#include <cstdlib>
#include <cmath>

#include "video_engine/vie_defines.h"

static int Saturate(int value, int min, int max) {
  return std::min(std::max(value, min), max);
}

RandomEncryption::RandomEncryption(unsigned int rand_seed) {
  srand(rand_seed);
}


void RandomEncryption::GenerateRandomData(unsigned char* out_data, int bytes_in,
                                          int* bytes_out) {
  int out_length = MakeUpSimilarLength(bytes_in);
  for (int i = 0; i < out_length; i++) {
    
    
    out_data[i] = static_cast<unsigned char>(rand() % 256);
  }
  *bytes_out = out_length;
}



int RandomEncryption::MakeUpSimilarLength(int original_length) {
  int sign = rand() - RAND_MAX / 2;
  int length = original_length + sign * rand() % 50;

  return Saturate(length, 0, static_cast<int>(webrtc::kViEMaxMtu));
}
