




#ifndef TrackEncoder_h_
#define TrackEncoder_h_

#include "mozilla/ReentrantMonitor.h"

#include "AudioSegment.h"
#include "EncodedFrameContainer.h"
#include "StreamBuffer.h"
#include "TrackMetadataBase.h"
#include "VideoSegment.h"
#include "MediaStreamGraph.h"

namespace mozilla {











class TrackEncoder
{
public:
  TrackEncoder();

  virtual ~TrackEncoder() {}

  



  virtual void NotifyQueuedTrackChanges(MediaStreamGraph* aGraph, TrackID aID,
                                        StreamTime aTrackOffset,
                                        uint32_t aTrackEvents,
                                        const MediaSegment& aQueuedMedia) = 0;

  



  void NotifyEvent(MediaStreamGraph* aGraph,
                   MediaStreamListener::MediaStreamGraphEvent event)
  {
    if (event == MediaStreamListener::MediaStreamGraphEvent::EVENT_REMOVED) {
      NotifyEndOfStream();
    }
  }

  



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

  
  uint32_t mAudioInitCounter;
  uint32_t mVideoInitCounter;
};

class AudioTrackEncoder : public TrackEncoder
{
public:
  AudioTrackEncoder()
    : TrackEncoder()
    , mChannels(0)
    , mSamplingRate(0)
  {}

  virtual void NotifyQueuedTrackChanges(MediaStreamGraph* aGraph, TrackID aID,
                                        StreamTime aTrackOffset,
                                        uint32_t aTrackEvents,
                                        const MediaSegment& aQueuedMedia) override;

  





  static void InterleaveTrackData(AudioChunk& aChunk, int32_t aDuration,
                                  uint32_t aOutputChannels,
                                  AudioDataValue* aOutput);

  



  static void DeInterleaveTrackData(AudioDataValue* aInput, int32_t aDuration,
                                    int32_t aChannels, AudioDataValue* aOutput);
  


  size_t SizeOfExcludingThis(mozilla::MallocSizeOf aMallocSizeOf) const;

protected:
  




  virtual int GetPacketDuration() { return 0; }

  






  virtual nsresult Init(int aChannels, int aSamplingRate) = 0;

  





  nsresult AppendAudioSegment(const AudioSegment& aSegment);

  



  virtual void NotifyEndOfStream() override;

  





  int mChannels;

  


  int mSamplingRate;

  


  AudioSegment mRawSegment;
};

class VideoTrackEncoder : public TrackEncoder
{
public:
  VideoTrackEncoder()
    : TrackEncoder()
    , mFrameWidth(0)
    , mFrameHeight(0)
    , mDisplayWidth(0)
    , mDisplayHeight(0)
    , mTrackRate(0)
    , mTotalFrameDuration(0)
  {}

  



  virtual void NotifyQueuedTrackChanges(MediaStreamGraph* aGraph, TrackID aID,
                                        StreamTime aTrackOffset,
                                        uint32_t aTrackEvents,
                                        const MediaSegment& aQueuedMedia) override;
  


  size_t SizeOfExcludingThis(mozilla::MallocSizeOf aMallocSizeOf) const;

protected:
  






  virtual nsresult Init(int aWidth, int aHeight, int aDisplayWidth,
                        int aDisplayHeight, TrackRate aTrackRate) = 0;

  



  nsresult AppendVideoSegment(const VideoSegment& aSegment);

  




  virtual void NotifyEndOfStream() override;

  


  int mFrameWidth;

  


  int mFrameHeight;

  


  int mDisplayWidth;

  


  int mDisplayHeight;

  


  TrackRate mTrackRate;

  



  StreamTime mTotalFrameDuration;

  



  VideoFrame mLastFrame;

  


  VideoSegment mRawSegment;
};

} 

#endif
