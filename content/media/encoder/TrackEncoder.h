




#ifndef TrackEncoder_h_
#define TrackEncoder_h_

#include "mozilla/ReentrantMonitor.h"

#include "AudioSegment.h"
#include "EncodedFrameContainer.h"
#include "StreamBuffer.h"
#include "TrackMetadataBase.h"

namespace mozilla {

class MediaStreamGraph;











class TrackEncoder
{
public:
  TrackEncoder()
    : mReentrantMonitor("media.TrackEncoder")
    , mEncodingComplete(false)
    , mEosSetInEncoder(false)
    , mInitialized(false)
    , mEndOfStream(false)
    , mCanceled(false)
  {}

  virtual ~TrackEncoder() {}

  



  virtual void NotifyQueuedTrackChanges(MediaStreamGraph* aGraph, TrackID aID,
                                        TrackRate aTrackRate,
                                        TrackTicks aTrackOffset,
                                        uint32_t aTrackEvents,
                                        const MediaSegment& aQueuedMedia) = 0;

  



  void NotifyRemoved(MediaStreamGraph* aGraph) { NotifyEndOfStream(); }

  



  virtual already_AddRefed<TrackMetadataBase> GetMetadata() = 0;

  



  virtual nsresult GetEncodedTrack(EncodedFrameContainer& aData) = 0;

  



  bool IsEncodingComplete() { return mEncodingComplete; }

  



  void NotifyCancel()
  {
    ReentrantMonitorAutoEnter mon(mReentrantMonitor);
    mCanceled = true;
    mReentrantMonitor.NotifyAll();
  }

protected:
  



  virtual void NotifyEndOfStream() = 0;

  





  ReentrantMonitor mReentrantMonitor;

  


  bool mEncodingComplete;

  



  bool mEosSetInEncoder;

  



  bool mInitialized;

  




  bool mEndOfStream;

  



  bool mCanceled;
};

class AudioTrackEncoder : public TrackEncoder
{
public:
  AudioTrackEncoder()
    : TrackEncoder()
    , mChannels(0)
    , mSamplingRate(0)
  {}

  void NotifyQueuedTrackChanges(MediaStreamGraph* aGraph, TrackID aID,
                                TrackRate aTrackRate,
                                TrackTicks aTrackOffset,
                                uint32_t aTrackEvents,
                                const MediaSegment& aQueuedMedia) MOZ_OVERRIDE;

protected:
  




  virtual int GetPacketDuration() { return 0; }

  






  virtual nsresult Init(int aChannels, int aSamplingRate) = 0;

  





  nsresult AppendAudioSegment(const AudioSegment& aSegment);

  



  virtual void NotifyEndOfStream() MOZ_OVERRIDE;

  





  void InterleaveTrackData(AudioChunk& aChunk, int32_t aDuration,
                           uint32_t aOutputChannels, AudioDataValue* aOutput);

  





  int mChannels;

  


  int mSamplingRate;

  


  AudioSegment mRawSegment;
};

class VideoTrackEncoder : public TrackEncoder
{

};

}
#endif
