





#ifndef DelayBuffer_h_
#define DelayBuffer_h_

#include "nsTArray.h"
#include "AudioSegment.h"
#include "mozilla/dom/AudioNodeBinding.h" 

namespace mozilla {

class DelayBuffer final
{
  typedef dom::ChannelInterpretation ChannelInterpretation;

public:
  
  
  DelayBuffer(double aMaxDelayTicks, double aSmoothingRate)
    : mSmoothingRate(aSmoothingRate)
    , mCurrentDelay(-1.0)
    
    , mMaxDelayTicks(ceil(aMaxDelayTicks))
    , mCurrentChunk(0)
    
#ifdef DEBUG
    , mHaveWrittenBlock(false)
#endif
  {
    
    
    
    MOZ_ASSERT(aMaxDelayTicks <=
               std::numeric_limits<decltype(mMaxDelayTicks)>::max());
  }

  
  void Write(const AudioChunk& aInputChunk);

  
  
  void Read(const double aPerFrameDelays[WEBAUDIO_BLOCK_SIZE],
            AudioChunk* aOutputChunk,
            ChannelInterpretation aChannelInterpretation);
  
  
  void Read(double aDelayTicks, AudioChunk* aOutputChunk,
            ChannelInterpretation aChannelInterpretation);

  
  
  
  
  void ReadChannel(const double aPerFrameDelays[WEBAUDIO_BLOCK_SIZE],
                   const AudioChunk* aOutputChunk, uint32_t aChannel,
                   ChannelInterpretation aChannelInterpretation);

  
  void NextBlock()
  {
    mCurrentChunk = (mCurrentChunk + 1) % mChunks.Length();
#ifdef DEBUG
    MOZ_ASSERT(mHaveWrittenBlock);
    mHaveWrittenBlock = false;
#endif
  }

  void Reset() {
    mChunks.Clear();
    mCurrentDelay = -1.0;
  };

  int MaxDelayTicks() const { return mMaxDelayTicks; }

  size_t SizeOfExcludingThis(MallocSizeOf aMallocSizeOf) const;

private:
  void ReadChannels(const double aPerFrameDelays[WEBAUDIO_BLOCK_SIZE],
                    const AudioChunk* aOutputChunk,
                    uint32_t aFirstChannel, uint32_t aNumChannelsToRead,
                    ChannelInterpretation aChannelInterpretation);
  bool EnsureBuffer();
  int PositionForDelay(int aDelay);
  int ChunkForPosition(int aPosition);
  int OffsetForPosition(int aPosition);
  int ChunkForDelay(int aDelay);
  void UpdateUpmixChannels(int aNewReadChunk, uint32_t channelCount,
                           ChannelInterpretation aChannelInterpretation);

  
  FallibleTArray<AudioChunk> mChunks;
  
  nsAutoTArray<const void*,GUESS_AUDIO_CHANNELS> mUpmixChannels;
  double mSmoothingRate;
  
  double mCurrentDelay;
  
  int mMaxDelayTicks;
  
  
  int mCurrentChunk;
  
  int mLastReadChunk;
#ifdef DEBUG
  bool mHaveWrittenBlock;
#endif
};

} 

#endif 
