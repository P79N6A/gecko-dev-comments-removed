




#ifndef MOZILLA_AUDIONODEEXTERNALINPUTSTREAM_H_
#define MOZILLA_AUDIONODEEXTERNALINPUTSTREAM_H_

#include "MediaStreamGraph.h"
#include "AudioNodeStream.h"
#include "mozilla/Atomics.h"


typedef struct SpeexResamplerState_ SpeexResamplerState;

namespace mozilla {







class AudioNodeExternalInputStream : public AudioNodeStream {
public:
  AudioNodeExternalInputStream(AudioNodeEngine* aEngine, TrackRate aSampleRate);
protected:
  ~AudioNodeExternalInputStream();

public:
  virtual void ProcessInput(GraphTime aFrom, GraphTime aTo, uint32_t aFlags) MOZ_OVERRIDE;

private:
  
  
  struct TrackMapEntry {
    ~TrackMapEntry();

    



    void ResampleInputData(AudioSegment* aSegment);
    



    void ResampleChannels(const nsTArray<const void*>& aBuffers,
                          uint32_t aInputDuration,
                          AudioSampleFormat aFormat,
                          float aVolume);

    
    
    TrackTicks mEndOfConsumedInputTicks;
    
    
    
    StreamTime mEndOfLastInputIntervalInInputStream;
    
    
    
    StreamTime mEndOfLastInputIntervalInOutputStream;
    


    TrackTicks mSamplesPassedToResampler;
    


    SpeexResamplerState* mResampler;
    






    AudioSegment mResampledData;
    


    uint32_t mResamplerChannelCount;
    


    TrackID mTrackID;
  };

  nsTArray<TrackMapEntry> mTrackMap;
  
  TrackTicks mCurrentOutputPosition;

  



  size_t GetTrackMapEntry(const StreamBuffer::Track& aTrack,
                          GraphTime aFrom);

  




  bool IsEnabled();
};

}

#endif 
