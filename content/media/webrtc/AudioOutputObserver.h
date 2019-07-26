



#ifndef AUDIOOUTPUTOBSERVER_H_
#define AUDIOOUTPUTOBSERVER_H_

#include "mozilla/StaticPtr.h"
#include "AudioMixer.h"

namespace webrtc {
class SingleRwFifo;
}

namespace mozilla {

typedef struct FarEndAudioChunk_ {
  uint16_t mSamples;
  bool mOverrun;
  int16_t mData[1]; 
} FarEndAudioChunk;


class AudioOutputObserver : public MixerCallbackReceiver
{
public:
  AudioOutputObserver();
  virtual ~AudioOutputObserver();

  void MixerCallback(AudioDataValue* aMixedBuffer,
                     AudioSampleFormat aFormat,
                     uint32_t aChannels,
                     uint32_t aFrames,
                     uint32_t aSampleRate) MOZ_OVERRIDE;

  void Clear();
  void InsertFarEnd(const AudioDataValue *aBuffer, uint32_t aSamples, bool aOverran,
                    int aFreq, int aChannels, AudioSampleFormat aFormat);
  uint32_t PlayoutFrequency() { return mPlayoutFreq; }
  uint32_t PlayoutChannels() { return mPlayoutChannels; }

  FarEndAudioChunk *Pop();
  uint32_t Size();

private:
  uint32_t mPlayoutFreq;
  uint32_t mPlayoutChannels;

  nsAutoPtr<webrtc::SingleRwFifo> mPlayoutFifo;
  uint32_t mChunkSize;

  
  nsAutoPtr<FarEndAudioChunk> mSaved;
  uint32_t mSamplesSaved;
};


extern StaticAutoPtr<AudioOutputObserver> gFarendObserver;

}

#endif
