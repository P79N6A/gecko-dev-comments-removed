





#include "AudioNodeEngine.h"

namespace mozilla {

void
AllocateAudioBlock(uint32_t aChannelCount, AudioChunk* aChunk)
{
  
  
  nsRefPtr<SharedBuffer> buffer =
    SharedBuffer::Create(WEBAUDIO_BLOCK_SIZE*aChannelCount*sizeof(float));
  aChunk->mDuration = WEBAUDIO_BLOCK_SIZE;
  aChunk->mChannelData.SetLength(aChannelCount);
  float* data = static_cast<float*>(buffer->Data());
  for (uint32_t i = 0; i < aChannelCount; ++i) {
    aChunk->mChannelData[i] = data + i*WEBAUDIO_BLOCK_SIZE;
  }
  aChunk->mBuffer = buffer.forget();
  aChunk->mVolume = 1.0f;
  aChunk->mBufferFormat = AUDIO_FORMAT_FLOAT32;
}

void
WriteZeroesToAudioBlock(AudioChunk* aChunk, uint32_t aStart, uint32_t aLength)
{
  MOZ_ASSERT(aStart + aLength <= WEBAUDIO_BLOCK_SIZE);
  if (aLength == 0)
    return;
  for (uint32_t i = 0; i < aChunk->mChannelData.Length(); ++i) {
    memset(static_cast<float*>(const_cast<void*>(aChunk->mChannelData[i])) + aStart,
           0, aLength*sizeof(float));
  }
}

void
AudioBlockAddChannelWithScale(const float aInput[WEBAUDIO_BLOCK_SIZE],
                              float aScale,
                              float aOutput[WEBAUDIO_BLOCK_SIZE])
{
  if (aScale == 1.0f) {
    for (uint32_t i = 0; i < WEBAUDIO_BLOCK_SIZE; ++i) {
      aOutput[i] += aInput[i];
    }
  } else {
    for (uint32_t i = 0; i < WEBAUDIO_BLOCK_SIZE; ++i) {
      aOutput[i] += aInput[i]*aScale;
    }
  }
}

void
AudioBlockCopyChannelWithScale(const float aInput[WEBAUDIO_BLOCK_SIZE],
                               float aScale,
                               float aOutput[WEBAUDIO_BLOCK_SIZE])
{
  if (aScale == 1.0f) {
    memcpy(aOutput, aInput, WEBAUDIO_BLOCK_SIZE*sizeof(float));
  } else {
    for (uint32_t i = 0; i < WEBAUDIO_BLOCK_SIZE; ++i) {
      aOutput[i] = aInput[i]*aScale;
    }
  }
}

void
AudioBlockCopyChannelWithScale(const float aInput[WEBAUDIO_BLOCK_SIZE],
                               const float aScale[WEBAUDIO_BLOCK_SIZE],
                               float aOutput[WEBAUDIO_BLOCK_SIZE])
{
  for (uint32_t i = 0; i < WEBAUDIO_BLOCK_SIZE; ++i) {
    aOutput[i] = aInput[i]*aScale[i];
  }
}

}
