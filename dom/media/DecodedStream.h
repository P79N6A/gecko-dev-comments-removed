





#ifndef DecodedStream_h_
#define DecodedStream_h_

#include "nsRefPtr.h"
#include "nsTArray.h"
#include "mozilla/UniquePtr.h"
#include "mozilla/gfx/Point.h"

namespace mozilla {

class MediaInputPort;
class SourceMediaStream;
class ProcessedMediaStream;
class DecodedStream;
class DecodedStreamGraphListener;
class OutputStreamListener;
class ReentrantMonitor;
class MediaStreamGraph;

namespace layers {
class Image;
}









class DecodedStreamData {
public:
  explicit DecodedStreamData(SourceMediaStream* aStream);
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
public:
  DecodedStream();
  DecodedStreamData* GetData() const;
  void DestroyData();
  void RecreateData(MediaStreamGraph* aGraph);
  void Connect(ProcessedMediaStream* aStream, bool aFinishWhenEnded);
  void Remove(MediaStream* aStream);
  void SetPlaying(bool aPlaying);

private:
  ReentrantMonitor& GetReentrantMonitor() const;
  void Connect(OutputStreamData* aStream);
  nsTArray<OutputStreamData>& OutputStreams();

  UniquePtr<DecodedStreamData> mData;
  
  nsTArray<OutputStreamData> mOutputStreams;

  
  
  
  
  
  
  
  mutable ReentrantMonitor mMonitor;
};

} 

#endif 
