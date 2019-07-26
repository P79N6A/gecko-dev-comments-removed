




#include "AudioMixer.h"
#include <assert.h>

using mozilla::AudioDataValue;
using mozilla::AudioSampleFormat;



void MixingDone(AudioDataValue* aData, AudioSampleFormat aFormat, uint32_t aChannels, uint32_t aFrames, uint32_t aSampleRate)
{
  bool silent = true;
  for (uint32_t i = 0; i < aChannels * aFrames; i++) {
    if (aData[i] != 0.0) {
      if (aFormat == mozilla::AUDIO_FORMAT_S16) {
        fprintf(stderr, "Sample at %d is not silent: %d\n", i, (short)aData[i]);
      } else {
        fprintf(stderr, "Sample at %d is not silent: %f\n", i, (float)aData[i]);
      }
      silent = false;
    }
  }
  if (!silent) {
    MOZ_CRASH();
  }
}



template<typename T>
T GetLowValue();

template<typename T>
T GetHighValue();

template<>
float GetLowValue<float>() {
  return -1.0;
}

template<>
short GetLowValue<short>() {
  return -INT16_MAX;
}

template<>
float GetHighValue<float>() {
  return 1.0;
}

template<>
short GetHighValue<short>() {
  return INT16_MAX;
}

void FillBuffer(AudioDataValue* aBuffer, uint32_t aLength, AudioDataValue aValue)
{
  AudioDataValue* end = aBuffer + aLength;
  while (aBuffer != end) {
    *aBuffer++ = aValue;
  }
}

int main(int argc, char* argv[]) {
  const uint32_t CHANNEL_LENGTH = 256;
  const uint32_t AUDIO_RATE = 44100;
  AudioDataValue a[CHANNEL_LENGTH * 2];
  AudioDataValue b[CHANNEL_LENGTH * 2];
  FillBuffer(a, CHANNEL_LENGTH, GetLowValue<AudioDataValue>());
  FillBuffer(a + CHANNEL_LENGTH, CHANNEL_LENGTH, GetHighValue<AudioDataValue>());
  FillBuffer(b, CHANNEL_LENGTH, GetHighValue<AudioDataValue>());
  FillBuffer(b + CHANNEL_LENGTH, CHANNEL_LENGTH, GetLowValue<AudioDataValue>());

  {
    int iterations = 2;
    mozilla::AudioMixer mixer(MixingDone);

    fprintf(stderr, "Test AudioMixer constant buffer length.\n");

    while (iterations--) {
      mixer.Mix(a, 2, CHANNEL_LENGTH, AUDIO_RATE);
      mixer.Mix(b, 2, CHANNEL_LENGTH, AUDIO_RATE);
      mixer.FinishMixing();
    }
  }

  {
    mozilla::AudioMixer mixer(MixingDone);

    fprintf(stderr, "Test AudioMixer variable buffer length.\n");

    FillBuffer(a, CHANNEL_LENGTH / 2, GetLowValue<AudioDataValue>());
    FillBuffer(a + CHANNEL_LENGTH / 2, CHANNEL_LENGTH / 2, GetLowValue<AudioDataValue>());
    FillBuffer(b, CHANNEL_LENGTH / 2, GetHighValue<AudioDataValue>());
    FillBuffer(b + CHANNEL_LENGTH / 2, CHANNEL_LENGTH / 2, GetHighValue<AudioDataValue>());
    mixer.Mix(a, 2, CHANNEL_LENGTH / 2, AUDIO_RATE);
    mixer.Mix(b, 2, CHANNEL_LENGTH / 2, AUDIO_RATE);
    mixer.FinishMixing();
    FillBuffer(a, CHANNEL_LENGTH, GetLowValue<AudioDataValue>());
    FillBuffer(a + CHANNEL_LENGTH, CHANNEL_LENGTH, GetHighValue<AudioDataValue>());
    FillBuffer(b, CHANNEL_LENGTH, GetHighValue<AudioDataValue>());
    FillBuffer(b + CHANNEL_LENGTH, CHANNEL_LENGTH, GetLowValue<AudioDataValue>());
    mixer.Mix(a, 2, CHANNEL_LENGTH, AUDIO_RATE);
    mixer.Mix(b, 2, CHANNEL_LENGTH, AUDIO_RATE);
    mixer.FinishMixing();
    FillBuffer(a, CHANNEL_LENGTH / 2, GetLowValue<AudioDataValue>());
    FillBuffer(a + CHANNEL_LENGTH / 2, CHANNEL_LENGTH / 2, GetLowValue<AudioDataValue>());
    FillBuffer(b, CHANNEL_LENGTH / 2, GetHighValue<AudioDataValue>());
    FillBuffer(b + CHANNEL_LENGTH / 2, CHANNEL_LENGTH / 2, GetHighValue<AudioDataValue>());
    mixer.Mix(a, 2, CHANNEL_LENGTH / 2, AUDIO_RATE);
    mixer.Mix(b, 2, CHANNEL_LENGTH / 2, AUDIO_RATE);
    mixer.FinishMixing();
  }

  FillBuffer(a, CHANNEL_LENGTH, GetLowValue<AudioDataValue>());
  FillBuffer(b, CHANNEL_LENGTH, GetHighValue<AudioDataValue>());

  {
    mozilla::AudioMixer mixer(MixingDone);
    fprintf(stderr, "Test AudioMixer variable channel count.\n");

    mixer.Mix(a, 1, CHANNEL_LENGTH, AUDIO_RATE);
    mixer.Mix(b, 1, CHANNEL_LENGTH, AUDIO_RATE);
    mixer.FinishMixing();
    mixer.Mix(a, 1, CHANNEL_LENGTH, AUDIO_RATE);
    mixer.Mix(b, 1, CHANNEL_LENGTH, AUDIO_RATE);
    mixer.FinishMixing();
    mixer.Mix(a, 1, CHANNEL_LENGTH, AUDIO_RATE);
    mixer.Mix(b, 1, CHANNEL_LENGTH, AUDIO_RATE);
    mixer.FinishMixing();
  }

  {
    mozilla::AudioMixer mixer(MixingDone);
    fprintf(stderr, "Test AudioMixer variable stream count.\n");

    mixer.Mix(a, 2, CHANNEL_LENGTH, AUDIO_RATE);
    mixer.Mix(b, 2, CHANNEL_LENGTH, AUDIO_RATE);
    mixer.FinishMixing();
    mixer.Mix(a, 2, CHANNEL_LENGTH, AUDIO_RATE);
    mixer.Mix(b, 2, CHANNEL_LENGTH, AUDIO_RATE);
    mixer.Mix(a, 2, CHANNEL_LENGTH, AUDIO_RATE);
    mixer.Mix(b, 2, CHANNEL_LENGTH, AUDIO_RATE);
    mixer.FinishMixing();
    mixer.Mix(a, 2, CHANNEL_LENGTH, AUDIO_RATE);
    mixer.Mix(b, 2, CHANNEL_LENGTH, AUDIO_RATE);
    mixer.FinishMixing();
  }

  return 0;
}

