





#ifndef DecodedStream_h_
#define DecodedStream_h_

#include "nsRefPtr.h"
#include "nsTArray.h"
#include "MediaInfo.h"

#include "mozilla/UniquePtr.h"
#include "mozilla/gfx/Point.h"
#include "mozilla/CheckedInt.h"
#include "mozilla/ReentrantMonitor.h"
#include "mozilla/Maybe.h"

namespace mozilla {

class AudioData;
class VideoData;
class AudioSegment;
class MediaStream;
class MediaInputPort;
class SourceMediaStream;
class ProcessedMediaStream;
class DecodedStream;
class DecodedStreamGraphListener;
class OutputStreamListener;
class ReentrantMonitor;
class MediaStreamGraph;

template <class T> class MediaQueue;

namespace layers {
class Image;
} 









class DecodedStreamData {
public:
  DecodedStreamData(SourceMediaStream* aStream, bool aPlaying);
  ~DecodedStreamData();
  bool IsFinished() const;
  int64_t GetPosition() const;
  void SetPlaying(bool aPlaying);

  


  
  int64_t mAudioFramesWritten;
  
  
  
  int64_t mNextVideoTime; 
  int64_t mNextAudioTime; 
  
  
  nsRefPtr<layers::Image> mLastVideoImage;
  gfx::IntSize mLastVideoImageDisplaySize;
  
  
  bool mStreamInitialized;
  bool mHaveSentFinish;
  bool mHaveSentFinishAudio;
  bool mHaveSentFinishVideo;

  
  const nsRefPtr<SourceMediaStream> mStream;
  nsRefPtr<DecodedStreamGraphListener> mListener;
  bool mPlaying;
  
  
  bool mEOSVideoCompensation;
};

class OutputStreamData {
public:
  ~OutputStreamData();
  void Init(DecodedStream* aDecodedStream, ProcessedMediaStream* aStream);
  nsRefPtr<ProcessedMediaStream> mStream;
  
  nsRefPtr<MediaInputPort> mPort;
  nsRefPtr<OutputStreamListener> mListener;
};

class DecodedStream {
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(DecodedStream);
public:
  DecodedStream(MediaQueue<AudioData>& aAudioQueue,
                MediaQueue<VideoData>& aVideoQueue);

  
  
  void StartPlayback(int64_t aStartTime, const MediaInfo& aInfo);
  
  void StopPlayback();

  void DestroyData();
  void RecreateData();
  void Connect(ProcessedMediaStream* aStream, bool aFinishWhenEnded);
  void Remove(MediaStream* aStream);
  void SetPlaying(bool aPlaying);
  int64_t AudioEndTime() const;
  int64_t GetPosition() const;
  bool IsFinished() const;

  
  bool SendData(double aVolume, bool aIsSameOrigin);

protected:
  virtual ~DecodedStream() {}

private:
  ReentrantMonitor& GetReentrantMonitor() const;
  void RecreateData(MediaStreamGraph* aGraph);
  void Connect(OutputStreamData* aStream);
  nsTArray<OutputStreamData>& OutputStreams();
  void InitTracks();
  void AdvanceTracks();
  void SendAudio(double aVolume, bool aIsSameOrigin);
  void SendVideo(bool aIsSameOrigin);

  UniquePtr<DecodedStreamData> mData;
  
  nsTArray<OutputStreamData> mOutputStreams;

  
  
  
  
  
  
  
  mutable ReentrantMonitor mMonitor;

  bool mPlaying;
  Maybe<int64_t> mStartTime;
  MediaInfo mInfo;

  MediaQueue<AudioData>& mAudioQueue;
  MediaQueue<VideoData>& mVideoQueue;
};

} 

#endif 
