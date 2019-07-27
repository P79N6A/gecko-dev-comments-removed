





#include "DelayBuffer.h"

#include "mozilla/PodOperations.h"
#include "AudioChannelFormat.h"
#include "AudioNodeEngine.h"

namespace mozilla {

size_t
DelayBuffer::SizeOfExcludingThis(MallocSizeOf aMallocSizeOf) const
{
  size_t amount = 0;
  amount += mChunks.SizeOfExcludingThis(aMallocSizeOf);
  for (size_t i = 0; i < mChunks.Length(); i++) {
    amount += mChunks[i].SizeOfExcludingThis(aMallocSizeOf, false);
  }

  amount += mUpmixChannels.SizeOfExcludingThis(aMallocSizeOf);
  return amount;
}

void
DelayBuffer::Write(const AudioChunk& aInputChunk)
{
  
  MOZ_ASSERT(aInputChunk.IsNull() == !aInputChunk.mChannelData.Length());
#ifdef DEBUG
  MOZ_ASSERT(!mHaveWrittenBlock);
  mHaveWrittenBlock = true;
#endif

  if (!EnsureBuffer()) {
    return;
  }

  if (mCurrentChunk == mLastReadChunk) {
    mLastReadChunk = -1; 
  }
  mChunks[mCurrentChunk] = aInputChunk;
}

void
DelayBuffer::Read(const double aPerFrameDelays[WEBAUDIO_BLOCK_SIZE],
                  AudioChunk* aOutputChunk,
                  ChannelInterpretation aChannelInterpretation)
{
  int chunkCount = mChunks.Length();
  if (!chunkCount) {
    aOutputChunk->SetNull(WEBAUDIO_BLOCK_SIZE);
    return;
  }

  
  
  
  
  
  
  
  double minDelay = aPerFrameDelays[0];
  double maxDelay = minDelay;
  for (unsigned i = 1; i < WEBAUDIO_BLOCK_SIZE; ++i) {
    minDelay = std::min(minDelay, aPerFrameDelays[i] - i);
    maxDelay = std::max(maxDelay, aPerFrameDelays[i] - i);
  }

  
  int oldestChunk = ChunkForDelay(int(maxDelay) + 1);
  int youngestChunk = ChunkForDelay(minDelay);

  uint32_t channelCount = 0;
  for (int i = oldestChunk; true; i = (i + 1) % chunkCount) {
    channelCount = GetAudioChannelsSuperset(channelCount,
                                            mChunks[i].ChannelCount());
    if (i == youngestChunk) {
      break;
    }
  }

  if (channelCount) {
    AllocateAudioBlock(channelCount, aOutputChunk);
    ReadChannels(aPerFrameDelays, aOutputChunk,
                 0, channelCount, aChannelInterpretation);
  } else {
    aOutputChunk->SetNull(WEBAUDIO_BLOCK_SIZE);
  }

  
  mCurrentDelay = aPerFrameDelays[WEBAUDIO_BLOCK_SIZE - 1];
}

void
DelayBuffer::ReadChannel(const double aPerFrameDelays[WEBAUDIO_BLOCK_SIZE],
                         const AudioChunk* aOutputChunk, uint32_t aChannel,
                         ChannelInterpretation aChannelInterpretation)
{
  if (!mChunks.Length()) {
    float* outputChannel = static_cast<float*>
      (const_cast<void*>(aOutputChunk->mChannelData[aChannel]));
    PodZero(outputChannel, WEBAUDIO_BLOCK_SIZE);
    return;
  }

  ReadChannels(aPerFrameDelays, aOutputChunk,
               aChannel, 1, aChannelInterpretation);
}

void
DelayBuffer::ReadChannels(const double aPerFrameDelays[WEBAUDIO_BLOCK_SIZE],
                          const AudioChunk* aOutputChunk,
                          uint32_t aFirstChannel, uint32_t aNumChannelsToRead,
                          ChannelInterpretation aChannelInterpretation)
{
  uint32_t totalChannelCount = aOutputChunk->mChannelData.Length();
  uint32_t readChannelsEnd = aFirstChannel + aNumChannelsToRead;
  MOZ_ASSERT(readChannelsEnd <= totalChannelCount);

  if (mUpmixChannels.Length() != totalChannelCount) {
    mLastReadChunk = -1; 
  }

  float* const* outputChannels = reinterpret_cast<float* const*>
    (const_cast<void* const*>(aOutputChunk->mChannelData.Elements()));
  for (uint32_t channel = aFirstChannel;
       channel < readChannelsEnd; ++channel) {
    PodZero(outputChannels[channel], WEBAUDIO_BLOCK_SIZE);
  }

  for (unsigned i = 0; i < WEBAUDIO_BLOCK_SIZE; ++i) {
    double currentDelay = aPerFrameDelays[i];
    MOZ_ASSERT(currentDelay >= 0.0);
    MOZ_ASSERT(currentDelay <= (mChunks.Length() - 1) * WEBAUDIO_BLOCK_SIZE);

    
    
    
    
    int floorDelay = int(currentDelay);
    double interpolationFactor = currentDelay - floorDelay;
    int positions[2];
    positions[1] = PositionForDelay(floorDelay) + i;
    positions[0] = positions[1] - 1;

    for (unsigned tick = 0; tick < ArrayLength(positions); ++tick) {
      int readChunk = ChunkForPosition(positions[tick]);
      
      
      if (!mChunks[readChunk].IsNull()) {
        int readOffset = OffsetForPosition(positions[tick]);
        UpdateUpmixChannels(readChunk, totalChannelCount,
                            aChannelInterpretation);
        double multiplier = interpolationFactor * mChunks[readChunk].mVolume;
        for (uint32_t channel = aFirstChannel;
             channel < readChannelsEnd; ++channel) {
          outputChannels[channel][i] += multiplier *
            static_cast<const float*>(mUpmixChannels[channel])[readOffset];
        }
      }

      interpolationFactor = 1.0 - interpolationFactor;
    }
  }
}

void
DelayBuffer::Read(double aDelayTicks, AudioChunk* aOutputChunk,
                  ChannelInterpretation aChannelInterpretation)
{
  const bool firstTime = mCurrentDelay < 0.0;
  double currentDelay = firstTime ? aDelayTicks : mCurrentDelay;

  double computedDelay[WEBAUDIO_BLOCK_SIZE];

  for (unsigned i = 0; i < WEBAUDIO_BLOCK_SIZE; ++i) {
    
    currentDelay += (aDelayTicks - currentDelay) * mSmoothingRate;
    computedDelay[i] = currentDelay;
  }

  Read(computedDelay, aOutputChunk, aChannelInterpretation);
}

bool
DelayBuffer::EnsureBuffer()
{
  if (mChunks.Length() == 0) {
    
    
    
    
    const int chunkCount = (mMaxDelayTicks + 2 * WEBAUDIO_BLOCK_SIZE - 1) >>
                                         WEBAUDIO_BLOCK_SIZE_BITS;
    if (!mChunks.SetLength(chunkCount)) {
      return false;
    }

    mLastReadChunk = -1;
  }
  return true;
}

int
DelayBuffer::PositionForDelay(int aDelay) {
  
  
  return ((mCurrentChunk + mChunks.Length()) * WEBAUDIO_BLOCK_SIZE) - aDelay;
}

int
DelayBuffer::ChunkForPosition(int aPosition)
{
  MOZ_ASSERT(aPosition >= 0);
  return (aPosition >> WEBAUDIO_BLOCK_SIZE_BITS) % mChunks.Length();
}

int
DelayBuffer::OffsetForPosition(int aPosition)
{
  MOZ_ASSERT(aPosition >= 0);
  return aPosition & (WEBAUDIO_BLOCK_SIZE - 1);
}

int
DelayBuffer::ChunkForDelay(int aDelay)
{
  return ChunkForPosition(PositionForDelay(aDelay));
}

void
DelayBuffer::UpdateUpmixChannels(int aNewReadChunk, uint32_t aChannelCount,
                                 ChannelInterpretation aChannelInterpretation)
{
  if (aNewReadChunk == mLastReadChunk) {
    MOZ_ASSERT(mUpmixChannels.Length() == aChannelCount);
    return;
  }

  static const float silenceChannel[WEBAUDIO_BLOCK_SIZE] = {};

  NS_WARN_IF_FALSE(mHaveWrittenBlock || aNewReadChunk != mCurrentChunk,
                   "Smoothing is making feedback delay too small.");

  mLastReadChunk = aNewReadChunk;
  mUpmixChannels = mChunks[aNewReadChunk].mChannelData;
  MOZ_ASSERT(mUpmixChannels.Length() <= aChannelCount);
  if (mUpmixChannels.Length() < aChannelCount) {
    if (aChannelInterpretation == ChannelInterpretation::Speakers) {
      AudioChannelsUpMix(&mUpmixChannels, aChannelCount, silenceChannel);
      MOZ_ASSERT(mUpmixChannels.Length() == aChannelCount,
                 "We called GetAudioChannelsSuperset to avoid this");
    } else {
      
      for (uint32_t channel = mUpmixChannels.Length();
           channel < aChannelCount; ++channel) {
        mUpmixChannels.AppendElement(silenceChannel);
      }
    }
  }
}

} 
