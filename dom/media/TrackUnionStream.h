




#ifndef MOZILLA_TRACKUNIONSTREAM_H_
#define MOZILLA_TRACKUNIONSTREAM_H_

#include "MediaStreamGraph.h"
#include <algorithm>

namespace mozilla {




class TrackUnionStream : public ProcessedMediaStream {
public:
  explicit TrackUnionStream(DOMMediaStream* aWrapper);

  virtual void RemoveInput(MediaInputPort* aPort) MOZ_OVERRIDE;
  virtual void ProcessInput(GraphTime aFrom, GraphTime aTo, uint32_t aFlags) MOZ_OVERRIDE;

  
  
  typedef bool (*TrackIDFilterCallback)(StreamBuffer::Track*);

  void SetTrackIDFilter(TrackIDFilterCallback aCallback);

  
  
  virtual void ForwardTrackEnabled(TrackID aOutputID, bool aEnabled) MOZ_OVERRIDE;

protected:
  TrackIDFilterCallback mFilterCallback;

  
  struct TrackMapEntry {
    
    
    StreamTime mEndOfConsumedInputTicks;
    
    
    
    StreamTime mEndOfLastInputIntervalInInputStream;
    
    
    
    StreamTime mEndOfLastInputIntervalInOutputStream;
    MediaInputPort* mInputPort;
    
    
    
    
    
    TrackID mInputTrackID;
    TrackID mOutputTrackID;
    nsAutoPtr<MediaSegment> mSegment;
  };

  uint32_t AddTrack(MediaInputPort* aPort, StreamBuffer::Track* aTrack,
                    GraphTime aFrom);
  void EndTrack(uint32_t aIndex);
  void CopyTrackData(StreamBuffer::Track* aInputTrack,
                     uint32_t aMapIndex, GraphTime aFrom, GraphTime aTo,
                     bool* aOutputTrackFinished);

  nsTArray<TrackMapEntry> mTrackMap;
};

}

#endif 
