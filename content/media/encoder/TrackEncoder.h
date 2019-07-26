




#ifndef TrackEncoder_h_
#define TrackEncoder_h_

#include "mozilla/ReentrantMonitor.h"

#include "AudioSegment.h"
#include "EncodedFrameContainer.h"
#include "StreamBuffer.h"
#include "TrackMetadataBase.h"
#include "VideoSegment.h"

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

  





  static void InterleaveTrackData(AudioChunk& aChunk, int32_t aDuration,
                                  uint32_t aOutputChannels,
                                  AudioDataValue* aOutput);

  



  static void DeInterleaveTrackData(AudioDataValue* aInput, int32_t aDuration,
                                    int32_t aChannels, AudioDataValue* aOutput);

protected:
  




  virtual int GetPacketDuration() { return 0; }

  






  virtual nsresult Init(int aChannels, int aSamplingRate) = 0;

  





  nsresult AppendAudioSegment(const AudioSegment& aSegment);

  



  virtual void NotifyEndOfStream() MOZ_OVERRIDE;

  





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

  



  void NotifyQueuedTrackChanges(MediaStreamGraph* aGraph, TrackID aID,
                                TrackRate aTrackRate,
                                TrackTicks aTrackOffset,
                                uint32_t aTrackEvents,
                                const MediaSegment& aQueuedMedia) MOZ_OVERRIDE;

protected:
  






  virtual nsresult Init(int aWidth, int aHeight, int aDisplayWidth,
                        int aDisplayHeight, TrackRate aTrackRate) = 0;

  



  nsresult AppendVideoSegment(const VideoSegment& aSegment);

  




  virtual void NotifyEndOfStream() MOZ_OVERRIDE;

  



  void CreateMutedFrame(nsTArray<uint8_t>* aOutputBuffer);

  


  int mFrameWidth;

  


  int mFrameHeight;

  


  int mDisplayWidth;

  


  int mDisplayHeight;

  


  TrackRate mTrackRate;

  



  TrackTicks mTotalFrameDuration;

  



  VideoFrame mLastFrame;

  


  VideoSegment mRawSegment;
};

}
#endif
