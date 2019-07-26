




#ifndef MOZILLA_AUDIONODEEXTERNALINPUTSTREAM_H_
#define MOZILLA_AUDIONODEEXTERNALINPUTSTREAM_H_

#include "MediaStreamGraph.h"
#include "AudioNodeStream.h"


typedef struct SpeexResamplerState_ SpeexResamplerState;

namespace mozilla {







class AudioNodeExternalInputStream : public AudioNodeStream {
public:
  AudioNodeExternalInputStream(AudioNodeEngine* aEngine, TrackRate aSampleRate);
  ~AudioNodeExternalInputStream();

  virtual void ProduceOutput(GraphTime aFrom, GraphTime aTo) MOZ_OVERRIDE;

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

  



  uint32_t GetTrackMapEntry(const StreamBuffer::Track& aTrack,
                            GraphTime aFrom);
};

}

#endif 
