




#ifndef TrackEncoder_h_
#define TrackEncoder_h_

#include "mozilla/ReentrantMonitor.h"

#include "AudioSegment.h"
#include "StreamBuffer.h"
#include "TrackMetadataBase.h"
#include "EncodedFrameContainer.h"

namespace mozilla {

class MediaStreamGraph;











class TrackEncoder
{
public:
  TrackEncoder() {}
  virtual ~TrackEncoder() {}

  



  virtual void NotifyQueuedTrackChanges(MediaStreamGraph* aGraph, TrackID aID,
                                        TrackRate aTrackRate,
                                        TrackTicks aTrackOffset,
                                        uint32_t aTrackEvents,
                                        const MediaSegment& aQueuedMedia) = 0;

  



  virtual void NotifyRemoved(MediaStreamGraph* aGraph) = 0;

  


  virtual nsRefPtr<TrackMetadataBase> GetMetadata() = 0;

  


  virtual nsresult GetEncodedTrack(EncodedFrameContainer& aData) = 0;
};

class AudioTrackEncoder : public TrackEncoder
{
public:
  AudioTrackEncoder()
    : TrackEncoder()
    , mChannels(0)
    , mSamplingRate(0)
    , mInitialized(false)
    , mDoneEncoding(false)
    , mReentrantMonitor("media.AudioEncoder")
    , mRawSegment(new AudioSegment())
    , mEndOfStream(false)
    , mCanceled(false)
    , mSilentDuration(0)
  {}

  void NotifyQueuedTrackChanges(MediaStreamGraph* aGraph, TrackID aID,
                                TrackRate aTrackRate,
                                TrackTicks aTrackOffset,
                                uint32_t aTrackEvents,
                                const MediaSegment& aQueuedMedia) MOZ_OVERRIDE;

  void NotifyRemoved(MediaStreamGraph* aGraph) MOZ_OVERRIDE;

  bool IsEncodingComplete()
  {
    return mDoneEncoding;
  }

  



  void NotifyCancel()
  {
    ReentrantMonitorAutoEnter mon(mReentrantMonitor);
    mCanceled = true;
    mReentrantMonitor.NotifyAll();
  }

protected:
  




  virtual int GetPacketDuration() = 0;

  






  virtual nsresult Init(int aChannels, int aSamplingRate) = 0;

  





  nsresult AppendAudioSegment(MediaSegment* aSegment);

  



  void NotifyEndOfStream();

  





  void InterleaveTrackData(AudioChunk& aChunk, int32_t aDuration,
                           uint32_t aOutputChannels, AudioDataValue* aOutput);

  



  int mChannels;
  int mSamplingRate;
  bool mInitialized;
  bool mDoneEncoding;

  


  ReentrantMonitor mReentrantMonitor;

  


  nsAutoPtr<AudioSegment> mRawSegment;

  




  bool mEndOfStream;

  



  bool mCanceled;

  



  TrackTicks mSilentDuration;
};

class VideoTrackEncoder : public TrackEncoder
{

};

}
#endif
