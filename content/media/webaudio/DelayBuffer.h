





#ifndef DelayBuffer_h_
#define DelayBuffer_h_

#include "nsTArray.h"
#include "AudioSegment.h"
#include "mozilla/dom/AudioNodeBinding.h" 

namespace mozilla {

class DelayBuffer {
  typedef dom::ChannelInterpretation ChannelInterpretation;

public:
  
  
  DelayBuffer(int aMaxDelayTicks, double aSmoothingRate)
    : mSmoothingRate(aSmoothingRate)
    , mCurrentDelay(-1.0)
    , mMaxDelayTicks(aMaxDelayTicks)
    , mCurrentChunk(0)
    
  {
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
  }

  void Reset() {
    mChunks.Clear();
    mCurrentDelay = -1.0;
  };

  int MaxDelayTicks() const { return mMaxDelayTicks; }

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
};

} 

#endif 
