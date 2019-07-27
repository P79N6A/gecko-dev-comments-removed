




#include <stdint.h>
#include <assert.h>
#include <mozilla/NullPtr.h>
#include "AudioBufferUtils.h"

const uint32_t FRAMES = 256;
const uint32_t CHANNELS = 2;
const uint32_t SAMPLES = CHANNELS * FRAMES;

int main() {
  mozilla::AudioCallbackBufferWrapper<float, CHANNELS> mBuffer;
  mozilla::SpillBuffer<float, 128, CHANNELS> b;
  float fromCallback[SAMPLES];
  float other[SAMPLES];

  for (uint32_t i = 0; i < SAMPLES; i++) {
    other[i] = 1.0;
    fromCallback[i] = 0.0;
  }

  
  mBuffer.SetBuffer(fromCallback, FRAMES);

  
  assert(b.Fill(other, 15) == 15);
  assert(b.Fill(other, 17) == 17);
  for (uint32_t i = 0; i < 32 * CHANNELS; i++) {
    other[i] = 0.0;
  }

  
  assert(b.Empty(mBuffer) == 32);

  
  assert(mBuffer.Available() == FRAMES - 32);

  
  mBuffer.WriteFrames(other + 32 * CHANNELS, FRAMES - 32);

  
  assert(mBuffer.Available() == 0);

  for (uint32_t i = 0 ; i < SAMPLES; i++) {
    if (fromCallback[i] != 1.0) {
      fprintf(stderr, "Difference at %d (%f != %f)\n", i, fromCallback[i], 1.0);
      assert(false);
    }
  }

  assert(b.Fill(other, FRAMES) == 128);
  assert(b.Fill(other, FRAMES) == 0);
  assert(b.Empty(mBuffer) == 0);

  return 0;
}
